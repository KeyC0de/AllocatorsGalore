#pragma once

#include <cassert>
#include <atomic>
#include <memory>
#include "aligned_allocations.h"

template<std::size_t tm_alignment = alignof(std::max_align_t)>
class SArena
{
	static inline constexpr std::size_t s_alignment = tm_alignment;

	char* m_pdata;
	std::atomic<char*> m_poffset;
	std::size_t m_maxSize;

	struct Header final {
		std::size_t allocationAddress;
	};
public:
	static constexpr std::size_t getAlignment() noexcept {
		return s_alignment;
	}

	// defctor
	//SArena()
	//	: m_pdata{ nullptr },
	//	m_poffset{ nullptr },
	//	m_maxSize{ 0 }
	//{
	//}

	/// \attention! DO NOT malloc inside the initializer list.
	SArena(std::size_t size)
		: m_maxSize(size)
	{
		assert(m_maxSize > 0);
		m_pdata = (char*)alignedMalloc(m_maxSize, s_alignment);

		m_poffset.store(m_pdata);
		if (m_pdata == nullptr)
		{
			throw std::runtime_error("alignedMalloc() failed");
		}
	}

	~SArena()
	{
		if (m_pdata != nullptr)
		{
			alignedFree(m_pdata);
		}
	}

	SArena(const SArena& rhs) = delete;
	SArena& operator=(const SArena& rhs) = delete;

	SArena(SArena&& rhs)
	{// delegate work to the maop
		*this = std::move(rhs);
	}
	SArena& operator=(SArena&& rhs)
	{
		if (this != &rhs)
		{
			if (m_pdata != nullptr)
			{
				alignedFree(m_pdata);
			}

			m_pdata = std::move(rhs.m_pdata);
			m_maxSize = rhs.m_maxSize;
			m_poffset.store(rhs.m_poffset.load(std::memory_order_relaxed));

			// destroy the other one
			rhs.m_pdata = nullptr;
			rhs.m_maxSize = 0;
			rhs.m_poffset.store(nullptr);
		}
		return *this;
	}


	/// \param	the `bytes` to allocate - more will be allocated due to alignment padding and Header size
	///	\return	The address to the start of this allocated memory region, or throw an exception if you can't
	[[nodiscard]]
	void* allocate(std::size_t bytes)
	{
		assert(m_pdata != nullptr);

		std::size_t currentAllocationStartAddress = alignForward((std::size_t) m_poffset.load(std::memory_order_relaxed) + sizeof(Header), s_alignment);

#ifdef _DEBUG
		std::cout << "allocating " << bytes << " bytes starting at address " << currentAllocationStartAddress << '\n';
#endif // _DEBUG

		// set the new offset
		m_poffset.store((char*)(currentAllocationStartAddress + bytes));
		// check that we haven't run out of memory
		if (m_poffset.load(std::memory_order_relaxed) <= m_pdata + m_maxSize)
		{
			((Header*)(currentAllocationStartAddress - sizeof(Header)))->allocationAddress = currentAllocationStartAddress;
		}
		else
			throw std::bad_alloc{};
		return reinterpret_cast<void*>(currentAllocationStartAddress);
	}

	void deallocate(void* plastAllocationAddress, [[maybe_unused]] std::size_t count = 0) noexcept
	{
		//assert(m_pdata != nullptr);
		//
		//std::size_t headerAddress = (std::size_t) plastAllocationAddress - sizeof(Header);
		//assert(headerAddress >= (std::size_t) m_pdata && headerAddress < getEndAddress());
		//
		//// get the previous allocation address
		//std::size_t allocationSize = ((Header*)headerAddress)->allocationAddress;
		//
		//char* expectedOffset = (char*)(headerAddress + allocationSize);
		//char* desiredOffset = (char*)headerAddress;
		//
		//bool succeed;
		//if (m_poffset == expectedOffset) {
		//	m_poffset = desiredOffset;
		//	std::cout << "desired offset set" << '\n';
		//	succeed = true;
		//}
		//else {
		//	expectedOffset = m_poffset;
		//	succeed = false;
		//}
		// or:
		//m_poffset.compare_exchange_strong(expectedOffset, desiredOffset);

		// or:
		*m_poffset = *(char*)((std::size_t)plastAllocationAddress - sizeof(Header));
		return;
	}

	/// reset the arena. All existing allocated memory will be lost
	void reset()
	{
		assert(m_pdata != nullptr);
		m_poffset.store(m_pdata);
	}

	std::size_t getAvailableMemory() const noexcept {
		return getEndAddress() - reinterpret_cast<std::size_t>(m_poffset.load(std::memory_order_relaxed));
	}

	/// GETTERS
	char* getStartAddress() const noexcept {
		return m_pdata;
	}
	std::size_t getCurrentAddress() const noexcept {
		return (std::size_t) m_pdata + (std::size_t) m_poffset.load(std::memory_order_relaxed);
	}
	std::size_t getEndAddress() const noexcept {
		return (std::size_t) m_pdata + m_maxSize;
	}
	std::atomic<char*> getOffset() const noexcept {
		return m_poffset;
	}
	std::size_t getMaxSize() const noexcept {
		return m_maxSize;
	}
};

///======================================================================
/// \class	StackAllocatorTS
///
/// \author	Nikos Lazaridis (KeyC0de)
/// \date	01-Oct-19
///
/// \tparam T : the type
/// \tparam tm_alignment : alignment value in bytes
///
/// \brief	Thread Safe version of the Stack Allocator
///	\brief	What makes it thread safe?
///			1. making the top of the stack pool m_poffset atomic
///			2. performing any write operations on it atomically
///			3. having a shared_ptr instead of a raw pointer to the Arena
///======================================================================
template<typename T, std::size_t tm_alignment = alignof(std::max_align_t)>
class StackAllocatorTS
{
	static_assert(isPowerOfTwo(tm_alignment), "Stack Allocator's alignment value must be a power of 2.");

	template<typename U, std::size_t otherAlignment>
	friend class StackAllocatorTS;

	using TSArena = SArena<tm_alignment>;

	std::shared_ptr<TSArena> m_localArena;
public:
	using value_type = T;
	using size_type = std::size_t;
	using difference_type = ptrdiff_t;
	using pointer = T * ;
	using const_pointer = T * const;
	using reference = T & ;
	using const_reference = const T&;

	// defctor
	StackAllocatorTS() noexcept
	{}
	// ctor
	StackAllocatorTS(std::size_t bytes) noexcept
	{
		m_localArena = std::make_shared<TSArena>(bytes);
	}
	// dtor
	~StackAllocatorTS()
	{}

	// cctors
	StackAllocatorTS(const StackAllocatorTS& rhs) noexcept
	{// delegate to caop
		*this = rhs;
	}
	template<typename U>
	StackAllocatorTS(const StackAllocatorTS<U, tm_alignment>& rhs) noexcept
	{
		*this = rhs;
	}

	// caops
	StackAllocatorTS& operator=(const StackAllocatorTS& rhs)
	{
		m_localArena = rhs.m_localArena;
		return *this;
	}
	template<typename U>
	StackAllocatorTS& operator=(const StackAllocatorTS<U, tm_alignment>& rhs)
	{
		m_localArena = rhs.m_localArena;
		return *this;
	}

	// mctors
	StackAllocatorTS(const StackAllocatorTS&& rhs) noexcept
		: m_localArena{ rhs.m_localArena }
	{}
	template<typename Other>
	StackAllocatorTS(const StackAllocatorTS<Other>&& rhs) noexcept
		: m_localArena{ rhs.m_localArena }
	{}

	// rebinding ctor to another allocator of type U
	template<typename U>
	struct rebind
	{
		using other = StackAllocatorTS<U, tm_alignment>;
	};

	/// get the address of a reference
	T* address(T& x) const noexcept
	{
		return &x;
	}
	T* const address(const T& x) const noexcept
	{
		return &x;
	}

	/// allocates count * sizeof(T) bytes
	[[nodiscard]]
	T* allocate(std::size_t count, [[maybe_unused]] const void* hint = nullptr)
	{
		assert(m_localArena != nullptr);

		void* ret = m_localArena->allocate(count * sizeof(T));
		if (ret == nullptr)
		{
			throw std::bad_alloc{};
		}

		return (T*)ret;
	}

	void deallocate(void* plastAllocationAddress, [[maybe_unused]] std::size_t count) noexcept
	{
		assert(m_localArena != nullptr);
		m_localArena->deallocate(plastAllocationAddress);
	}

	template<typename... Args>
	void construct(T* p, Args&&... args) const
	{
		new(p) T(std::forward<Args>(args)...);
	}
	template<typename J, typename... Args>
	void construct(J* p, Args&&... args) const
	{
		new(p) J(std::forward<Args>(args)...);
	}

	void destroy(T* p) const noexcept
	{
		p->~T();
	}
	template<typename J>
	void destroy(J* p) const noexcept
	{
		p->~J();
	}

	/// returns the largest possible value in Bytes that can meaningfully be passed to `allocate`
	std::size_t max_size() const noexcept
	{
		assert(m_localArena != nullptr);
		return m_localArena->getMaxSize();
	}

	void reset()
	{
		assert(m_localArena != nullptr);
		m_localArena->reset();
	}

	/// get the memory pool used by this allocator
	const TSArena& getArena() const noexcept
	{
		return *m_localArena;
	}

	std::size_t getAvailableMemory() const noexcept {
		return m_localArena->getAvailableMemory();
	}

	constexpr std::size_t getAlignment() const noexcept {
		return m_localArena->getAlignment();
	}

	std::size_t getSize() const noexcept {
		return m_localArena->getMaxSize();
	}
};


/// void specialization such that one particular allocator instance may allocate and construct different 
///			types of objects (see amap and mapstring example)
/// it is usable, but you're advised to abstain and use concrete types in all cases
template<std::size_t tm_alignment>
class StackAllocatorTS<void, tm_alignment>
{
	static_assert(isPowerOfTwo(tm_alignment), "Stack Allocator's alignment value must be a power of 2.");

	template<typename U, std::size_t otherAlignment>
	friend class StackAllocatorTS;

	using TSArena = SArena<tm_alignment>;

	std::shared_ptr<TSArena> m_localArena;
public:
	using value_type = void;
	using size_type = std::size_t;
	using difference_type = ptrdiff_t;
	using pointer = void*;
	using const_pointer = void* const;

	// defctor
	StackAllocatorTS() noexcept
	{}
	// ctor
	StackAllocatorTS(std::size_t bytes) noexcept
	{
		m_localArena = std::make_shared<TSArena>(bytes);
	}
	// dtor
	~StackAllocatorTS()
	{}

	// cctors
	StackAllocatorTS(const StackAllocatorTS& rhs) noexcept
	{// delegate to caop
		*this = rhs;
	}
	template<typename U>
	StackAllocatorTS(const StackAllocatorTS<U, tm_alignment>& rhs) noexcept
	{
		*this = rhs;
	}

	// caops
	StackAllocatorTS& operator=(const StackAllocatorTS& rhs)
	{
		m_localArena = rhs.m_localArena;
		return *this;
	}
	template<typename U>
	StackAllocatorTS& operator=(const StackAllocatorTS<U, tm_alignment>& rhs)
	{
		m_localArena = rhs.m_localArena;
		return *this;
	}

	// mctors
	StackAllocatorTS(const StackAllocatorTS&& rhs) noexcept
		: m_localArena{ rhs.m_localArena }
	{}
	template<typename Other>
	StackAllocatorTS(const StackAllocatorTS<Other>&& rhs) noexcept
		: m_localArena{ rhs.m_localArena }
	{}

	// rebinding ctor to another allocator of type U
	template<typename U>
	struct rebind
	{
		using other = StackAllocatorTS<U, tm_alignment>;
	};

	[[nodiscard]]
	void* allocate(std::size_t count, const void* hint = nullptr)
	{
		assert(m_localArena != nullptr);
		std::size_t bytes = count * sizeof(void);

		void* ret = m_localArena->allocate(bytes);
		if (ret == nullptr)
		{
			throw std::bad_alloc{};
		}

		return ret;
	}

	void deallocate(void* plastAllocation, [[maybe_unused]] std::size_t count = 0)
	{
		assert(m_localArena != nullptr);
		m_localArena->deallocate(plastAllocation);
	}

	template<typename J, typename... Args>
	void construct(J* p, Args&&... args) const
	{
		new(p) J(std::forward<Args>(args)...);
		// or:
		//::new (static_cast<void*>(p)) J(std::forward<Args>(args)...)
	}
	template<typename J>
	void destroy(J* p) const noexcept
	{
		p->~J();
	}

	/// returns the largest possible value in Bytes that can meaningfully be passed to `allocate`
	std::size_t max_size() const
	{
		assert(m_localArena != nullptr);
		return m_localArena->getMaxSize();
	}

	/// reinitialize the allocator. All existing allocated memory will be lost
	void reset()
	{
		assert(m_localArena != nullptr);
		m_localArena->reset();
	}

	// get the memory pool used by this allocator
	const TSArena& getArena() const
	{
		assert(m_localArena != nullptr);
		return *m_localArena;
	}

	std::size_t getAvailableMemory() const noexcept {
		return m_localArena->getAvailableMemory();
	}

	std::size_t getSize() const noexcept {
		return m_localArena->getMaxSize();
	}
};


/// an other allocator of the same type can deallocate from this one
template<typename T, std::size_t tm_alignment, std::size_t tm_otherAlignment>
inline bool operator==(const StackAllocatorTS<T, tm_alignment>&, const StackAllocatorTS<T, tm_otherAlignment>&)
{
	return true;
}
// equivalent statement:
template<typename T, std::size_t tm_alignment, std::size_t tm_otherAlignment>
inline bool operator!=(const StackAllocatorTS<T, tm_alignment>&, const StackAllocatorTS<T, tm_otherAlignment>&)
{
	return false;
}

/// an other allocator of a different type cannot deallocate from this one
template<typename T, std::size_t tm_alignment, typename OtherAllocator>
inline bool operator==(const StackAllocatorTS<T, tm_alignment>&, const OtherAllocator&)
{
	return false;
}
// equivalent statement:
template<typename T, std::size_t tm_alignment, typename OtherAllocator>
inline bool operator!=(const StackAllocatorTS<T, tm_alignment>&, const OtherAllocator&)
{
	return true;
}