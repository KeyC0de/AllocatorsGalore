#pragma once

#include <cstddef>
#include <iostream>
#include "../allocator_utils.h"
#include "../assertions.h"


//======================================================================
// \class	Arena
//
// \author	Nikos Lazaridis (KeyC0de)
// \date	27-Sep-19
//
// \brief	A memory buffer with linear or stack allocation policy.
//			New allocations simply move an offset forward
//			`allocate()` throws exception if there is no more space in the arena
//			you can't use the same arena for different types (at least not for different
//				types that have different alignment requirements).
//======================================================================
template<std::size_t alignment = alignof( std::max_align_t )>
class Arena
{
	inline static constexpr std::size_t m_alignment = alignment;
	unsigned char* const m_pData;
	std::size_t const m_maxSize;
	std::size_t m_offset;
public:
	Arena( std::size_t size )
		:
		m_pData{static_cast<unsigned char*>( alignedMalloc( size, m_alignment ) )},
		m_maxSize{size},
		m_offset{0}
	{
		static_assert( isPowerOfTwo( alignment ),
			"Arena alignment value must be a power of 2." );
#if defined _DEBUG && !defined NDEBUG
		std::cout << "arena["
			<< this
			<< "] of size "
			<< size
			<< " created.\n";
#endif // _DEBUG
	}

	~Arena() noexcept
	{
#if defined _DEBUG && !defined NDEBUG
		std::cout << "arena["
			<< this
			<< "] destroyed; final offset was: "
			<< m_offset
			<< '\n';
#endif // _DEBUG
		alignedFree( m_pData );
	}

	Arena( const Arena& rhs ) = delete;
	Arena& operator=( const Arena& rhs ) = delete;

	Arena( Arena&& rhs ) noexcept
		:
		m_pData{rhs.m_pData},
		m_maxSize{rhs.m_maxSize},
		m_offset{rhs.m_offset}
	{

	}

	Arena& operator=( Arena&& rhs ) noexcept
	{
		std::swap( m_pData,
			rhs.m_pData );
		std::swap( m_maxSize,
			rhs.m_maxSize );
		std::swap( m_offset,
			rhs.m_offset );
		return *this;
	}

	[[nodiscard]]
	void* allocate( std::size_t bytes )
	{
		m_offset = alignForward( m_offset,
			m_alignment );
		ASSERT( isAligned( m_offset, m_alignment ),
			"Not aligned!" );
		std::size_t currentAllocationStartAddress = getCurrentAddress();

		// check if there is enough memory available
		if ( m_offset + bytes > m_maxSize )
		{
			throw std::bad_alloc{};
		}
#if defined _DEBUG && !defined NDEBUG
		std::cout << "arena["
			<< this
			<< "] allocating "
			<< bytes
			<< " bytes at offset "
			<< m_offset << '\n';
#endif // _DEBUG
		m_offset += bytes;
		return reinterpret_cast<void*>( currentAllocationStartAddress );
	}

	// may be called for temporaries, but nothing gets deallocated in an arena ever,
	//	but the arena itself!
	void deallocate( void*,
		std::size_t count )
	{
#if defined _DEBUG && !defined NDEBUG
		std::cout << "arena["
			<< this
			<< "] may deallocate "
			<< count
			<< " bytes.\n";
#endif // _DEBUG
	}

	constexpr std::size_t getAvailableMemory() const noexcept
	{
		return m_maxSize - m_offset;
	}

	std::size_t getTotalMemory() const noexcept
	{
		return m_maxSize;
	}

	std::size_t getCurrentAddress() const noexcept
	{
		return reinterpret_cast<std::size_t>( const_cast<unsigned char*>( m_pData ) )
			+ m_offset;
	}

	std::size_t getEndAddress() const noexcept
	{
		return reinterpret_cast<std::size_t>( const_cast<unsigned char*>( m_pData ) )
			+ m_maxSize;
	}

	// GETTERS
	char* getStartAddress() const noexcept
	{
		return m_pData;
	}

	std::size_t getOffset() const noexcept
	{
		return m_offset;
	}

	const std::size_t getSize() const noexcept
	{
		return m_maxSize;
	}

	static constexpr std::size_t getAlignment() noexcept
	{
		return m_alignment;
	}
};

//----------------------------------------------------------------------------------------
// LinearAllocator
//
// \author	Nikos Lazaridis (KeyC0de)
// \date	26/9/2019
//
// \brief	allocates memory in a linear way; simply adjusting an offset we make memory available
//			the constructor allocates the initial memory chunk
//			the destructor clears the entire allocated memory chunk
//				"micro"-deallocations cannot be made
//			the allocator obtains its memory from an arena
//----------------------------------------------------------------------------------------
template<typename T, std::size_t alignment = alignof( std::max_align_t )>
class LinearAllocator
{
	using TArena = Arena<alignment>;

	inline static constexpr std::size_t m_alignment = alignment;
	TArena* m_pArena;
public:
	using value_type = T;
	using pointer = T*;

	//using propagate_on_container_copy_assignment = std::true_type;
	//using propagate_on_container_move_assignment = std::true_type;
	//using propagate_on_container_swap = std::true_type;

	explicit LinearAllocator( TArena* pArena )
		:
		m_pArena{pArena}
	{
		static_assert( isPowerOfTwo( alignment ),
			"Alignment value must be a power of 2." );
		ASSERT( pArena->getAlignment() == getAlignment(),
			"Arena not aligned!" );
	}

	LinearAllocator( const LinearAllocator& rhs ) noexcept
		:
		m_pArena{rhs.getArena()}
	{

	}

	template<typename Other, std::size_t OtherAlignment>
	LinearAllocator( const LinearAllocator<Other, OtherAlignment>& rhs ) noexcept
		:
		m_pArena{rhs.getArena()}
	{

	}
	LinearAllocator( const LinearAllocator&& rhs ) noexcept
		:
		m_pArena{rhs.getArena()}
	{

	}

	template<typename Other, std::size_t OtherAlignment>
	LinearAllocator( const LinearAllocator<Other, OtherAlignment>&& rhs ) noexcept
		:
		m_pArena{rhs.getArena()}
	{

	}

	static constexpr std::size_t getAlignment() noexcept
	{
		return m_alignment;
	}

	template<typename Other, std::size_t OtherAlignment = getAlignment()>
	struct rebind
	{
		using other = LinearAllocator<Other, OtherAlignment>;
	};

	[[nodiscard]]
	T* allocate( std::size_t count )
	{
		return static_cast<T*>( m_pArena->allocate( count * sizeof( T ) ) );
	}

	void deallocate( T* p,
		std::size_t count ) noexcept
	{
		m_pArena->deallocate( p,
			count * sizeof( T ) );
	}

	TArena* getArena() const noexcept
	{
		return m_pArena;
	}

	constexpr std::size_t getAvailableMemory() const noexcept
	{
		return m_pArena->getAvailableMemory();
	}
};

template<typename T, std::size_t TAlignment, typename Other, std::size_t OtherAlignment>
inline bool operator==( const LinearAllocator<T, TAlignment>& lhs,
	const LinearAllocator<Other, OtherAlignment>& rhs ) noexcept
{
	return lhs.getArena() == rhs.getArena();
}

template<typename T, std::size_t TAlignment, typename Other, std::size_t OtherAlignment>
inline bool operator!=( const LinearAllocator<T, TAlignment>& lhs,
	const LinearAllocator<Other, OtherAlignment>& rhs ) noexcept
{
	return lhs.getArena() != rhs.getArena();
}
