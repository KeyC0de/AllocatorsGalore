#pragma once

#include <cstddef>
#include <cassert>
#include <new>
#include <iostream>
#include "aligned_allocations.h"

///======================================================================
/// \class	Arena
///
/// \author	Nikos Lazaridis (KeyC0de)
/// \date	27-Sep-19
///
/// \brief	A memory buffer with linear or stack allocation policy. New allocations simply move an offset forward
///	`allocate()` throws exception if there is no more space in the arena
///	\details you can't use the same arena for different types (at least not different types that have different alignment requirements).
///======================================================================
template<std::size_t alignment = alignof(std::max_align_t)>
class Arena
{
	inline static constexpr std::size_t s_alignment = alignment;
	unsigned char* const m_pdata;
	std::size_t const m_maxSize;
	std::size_t m_offset;
public:
	Arena(std::size_t size)
		: m_pdata{static_cast<unsigned char*>(alignedMalloc(size, s_alignment))},
		m_maxSize{ size },
		m_offset{ 0 }
	{
		static_assert(isPowerOfTwo(alignment), "Arena alignment value must be a power of 2.");
#ifdef _DEBUG
		std::cout << "arena[" << this << "] of size " << size << " created." << '\n';
#endif // _DEBUG
	}

	~Arena() noexcept
	{
#ifdef _DEBUG
		std::cout << "arena[" << this << "] destroyed; final offset was: " << m_offset << '\n';
#endif // _DEBUG
		alignedFree(m_pdata);
	}

	Arena(const Arena& rhs) = delete;
	Arena& operator=(const Arena& rhs) = delete;

	Arena(Arena&& rhs) noexcept
		: m_pdata{rhs.m_pdata},
		m_maxSize{rhs.m_maxSize},
		m_offset{rhs.m_offset}
	{}
	Arena& operator=(Arena&& rhs) noexcept
	{
		if (this != &rhs)
		{
			std::swap(m_pdata, rhs.m_pdata);
			std::swap(m_maxSize, rhs.m_maxSize);
			std::swap(m_offset, rhs.m_offset);
		}
		return *this;
	}

	[[nodiscard]]
	void* allocate(std::size_t bytes)
	{
		m_offset = alignForward(m_offset, s_alignment);
		assert(isAligned(m_offset, s_alignment));
		std::size_t currentAllocationStartAddress = getCurrentAddress();

		// check if there is enough memory available
		if (m_offset + bytes > m_maxSize)
		{
			throw std::bad_alloc{};
		}
#ifdef _DEBUG
		std::cout << "arena[" << this << "] allocating " << bytes << " bytes at offset " << m_offset << '\n';
#endif // _DEBUG
		m_offset += bytes;

		return reinterpret_cast<void*>(currentAllocationStartAddress);
	}

	/// may called for temporaries, but nothing gets deallocated in an arena ever, but the arena itself!
	void deallocate(void*, std::size_t count)
	{
#ifdef _DEBUG
		std::cout << "arena[" << this << "] may deallocate " << count << " bytes." << '\n';
#endif // _DEBUG
	}

	/// HELPERS
	constexpr std::size_t getAvailableMemory() const noexcept {
		return m_maxSize - m_offset;
	}
	std::size_t getTotalMemory() const noexcept {
		return m_maxSize;
	}
	std::size_t getCurrentAddress() const noexcept {
		return reinterpret_cast<std::size_t>(const_cast<unsigned char*>(m_pdata)) + m_offset;
	}
	std::size_t getEndAddress() const noexcept {
		return reinterpret_cast<std::size_t>(const_cast<unsigned char*>(m_pdata)) + m_maxSize;
	}

	/// GETTERS
	char* getStartAddress() const noexcept {
		return m_pdata;
	}
	std::size_t getOffset() const noexcept {
		return m_offset;
	}
	const std::size_t getSize() const noexcept {
		return m_maxSize;
	}
	static constexpr std::size_t getAlignment() noexcept {
		return s_alignment;
	}
};

///---------------------------------------------------------------------------------------
/// C++11 compatible Linear Allocator
///
///	\author Nikos Lazaridis (KeyC0de)
/// \date	26/9/2019
///
///	allocates memory in a linear way. By simply adjusting an offset we make more memory available
/// the constructor allocates the initial memory chunk
/// the destructor clears the entire allocated memory chunk, "micro"-deallocations cannot be made
///	the allocator obtains its memory from an arena
///---------------------------------------------------------------------------------------
template<typename T, std::size_t alignment = alignof(std::max_align_t)>
class LinearAllocator
{
	using TArena = Arena<alignment>;

	inline static constexpr std::size_t s_alignment = alignment;
	TArena* m_localArena;
public:
	using value_type = T;
	using pointer = T*;

	//using propagate_on_container_copy_assignment = std::true_type;
	//using propagate_on_container_move_assignment = std::true_type;
	//using propagate_on_container_swap = std::true_type;

	explicit LinearAllocator(TArena* arena)
		: m_localArena{arena}
	{
		static_assert(isPowerOfTwo(alignment), "Alignment value must be a power of 2.");
		assert(arena->getAlignment() == getAlignment());
	}

	LinearAllocator(const LinearAllocator& rhs) noexcept
		: m_localArena{rhs.getArena()}
	{}
	template<typename Other, std::size_t OtherAlignment>
	LinearAllocator(const LinearAllocator<Other, OtherAlignment>& rhs) noexcept
		: m_localArena{ rhs.getArena() }
	{}
	LinearAllocator(const LinearAllocator&& rhs) noexcept
		: m_localArena{ rhs.getArena() }
	{}
	template<typename Other, std::size_t OtherAlignment>
	LinearAllocator(const LinearAllocator<Other, OtherAlignment>&& rhs) noexcept
		: m_localArena{ rhs.getArena() }
	{}

	static constexpr std::size_t getAlignment() noexcept {
		return s_alignment;
	}

	template<typename Other, std::size_t OtherAlignment = getAlignment()>
	struct rebind {
		using other = LinearAllocator<Other, OtherAlignment>;
	};

	[[nodiscard]]
	T* allocate(std::size_t count)
	{
		return static_cast<T*>(m_localArena->allocate(count * sizeof(T)));
	}

	void deallocate(T* p, std::size_t count) noexcept
	{
		m_localArena->deallocate(p, count * sizeof(T));
	}

	TArena* getArena() const noexcept {
		return m_localArena;
	}

	constexpr std::size_t getAvailableMemory() const noexcept {
		return m_localArena->getAvailableMemory();
	}
};

template<typename T, std::size_t TAlignment, typename Other, std::size_t OtherAlignment>
inline bool operator==(const LinearAllocator<T, TAlignment>& lhs, const LinearAllocator<Other, OtherAlignment>& rhs) noexcept
{
	return lhs.getArena() == rhs.getArena();
}

template<typename T, std::size_t TAlignment, typename Other, std::size_t OtherAlignment>
inline bool operator!=(const LinearAllocator<T, TAlignment>& lhs, const LinearAllocator<Other, OtherAlignment>& rhs) noexcept
{
	return lhs.getArena() != rhs.getArena();
}