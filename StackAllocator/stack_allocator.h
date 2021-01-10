#pragma once

#include <cassert>
#include "../AlignedAllocator/aligned_allocations.h"


template<std::size_t tm_alignment = alignof( std::max_align_t )>
class SArena
{
	static inline constexpr std::size_t s_alignment = tm_alignment;

	char* m_pData;
	char* m_poffset;
	std::size_t m_maxSize;

	struct Header final
	{
		std::size_t allocationAddress;
	};
public:
	static constexpr std::size_t getAlignment() noexcept
	{
		return s_alignment;
	}

	// defctor
	SArena()
		:
		m_pData{nullptr},
		m_poffset{nullptr},
		m_maxSize{0}
	{}

	// Attention! DO NOT malloc inside the initializer list.
	SArena( std::size_t size )
		:
		m_maxSize( size )
	{
		assert( m_maxSize > 0 );
		m_pData = (char*)alignedMalloc( m_maxSize, s_alignment );

		m_poffset = m_pData;
		if ( m_pData == nullptr )
		{
			throw std::runtime_error( "alignedMalloc() failed" );
		}
	}

	~SArena()
	{
		if ( m_pData != nullptr )
		{
			alignedFree( m_pData );
		}
	}

	SArena( const SArena& rhs ) = delete;
	SArena& operator=( const SArena& rhs ) = delete;

	SArena( SArena&& rhs )
	{// delegate work to the maop
		*this = std::move( rhs );
	}
	SArena& operator=( SArena&& rhs )
	{
		if ( this != &rhs )
		{
			if ( m_pData != nullptr )
			{
				alignedFree( m_pData );
			}

			m_pData = std::move( rhs.m_pData );
			m_maxSize = rhs.m_maxSize;
			m_poffset = rhs.m_poffset;

			// destroy the other one
			rhs.m_pData = nullptr;
			rhs.m_maxSize = 0;
			rhs.m_poffset = nullptr;
		}
		return *this;
	}


	// the `bytes` to allocate - more will be allocated due to alignment padding and Header size
	// The address to the start of this allocated memory region, or throw an exception if you can't
	[[nodiscard]]
	void* allocate( std::size_t bytes )
	{
		assert( m_pData != nullptr );

		std::size_t currentAllocationStartAddress = alignForward( (std::size_t) m_poffset
			+ sizeof( Header ),
			s_alignment );

#ifdef _DEBUG
		std::cout << "allocating "
			<< bytes
			<< " bytes starting at address "
			<< currentAllocationStartAddress
			<< '\n';
#endif // _DEBUG

		// set the new offset
		m_poffset = (char*) ( currentAllocationStartAddress + bytes );
		// check that we haven't run out of memory
		if ( m_poffset <= m_pData + m_maxSize )
		{
			( (Header*)
				( currentAllocationStartAddress - sizeof( Header ) ) )->allocationAddress
				= currentAllocationStartAddress;
		}
		else
		{
			throw std::bad_alloc{};
		}
		return reinterpret_cast<void*>( currentAllocationStartAddress );
	}

	void deallocate( void* plastAllocationAddress,
		[[maybe_unused]] std::size_t count = 0) noexcept
	{
		//assert(m_pData != nullptr);
		//
		//std::size_t headerAddress = (std::size_t) plastAllocationAddress - sizeof(Header);
		//assert(headerAddress >= (std::size_t) m_pData && headerAddress < getEndAddress());
		//
		//// get the previous allocation address
		//std::size_t allocationSize = ((Header*)headerAddress)->allocationAddress;
		//
		//char* expectedOffset = (char*)(headerAddress + allocationSize);
		//char* desiredOffset = (char*)headerAddress;
		// or:
		*m_poffset = *(char*)( (std::size_t) plastAllocationAddress - sizeof( Header ) );
		return;
	}

	// reset the arena. All existing allocated memory will be lost
	void reset()
	{
		assert( m_pData != nullptr );
		m_poffset = m_pData;
	}

	std::size_t getAvailableMemory() const noexcept
	{
		return getEndAddress() - reinterpret_cast<std::size_t>( m_poffset );
	}

	// GETTERS
	char* getStartAddress() const noexcept
	{
		return m_pData;
	}
	std::size_t getCurrentAddress() const noexcept
	{
		return (std::size_t) m_pData + (std::size_t) m_poffset;
	}
	std::size_t getEndAddress() const noexcept
	{
		return (std::size_t) m_pData + m_maxSize;
	}
	char* getOffset() const noexcept
	{
		return m_poffset;
	}
	std::size_t getMaxSize() const noexcept
	{
		return m_maxSize;
	}
};

//======================================================================
// \class	StackAllocator
//
// \author	Nikos Lazaridis (KeyC0de)
// \date	30-Sep-19
//
// \tparam T : the type
// \tparam tm_alignment : alignment value in bytes
//
// 	The allocator uses a memory pool (an Arena) through which it requests and releases memory to its clients
// 	The stack allocator is an extension of the Linear allocator, with the additional ability
//			that it can also deallocate.
//		Although the StackAllocator is copyable, the Arena is not, so effectively copies become moves. 
//	Caveat: requires LIFO order of deallocation calls. Standard library types that use allocators or make indirect use of std::allocator_traits
//			do not operate in LIFO memory deallocation order by nature, thus you have to make sure that deallocations follow this order.
//			This will mostly be invisible to the operator of the class, however the error space has not been exhausted. Errors could come up.
//			For all other non-allocator users there is no problem.
//======================================================================
template<typename T,
	std::size_t tm_alignment = alignof( std::max_align_t )>
class StackAllocator
{
	static_assert( isPowerOfTwo( tm_alignment ),
		"Stack Allocator's alignment value must be a power of 2." );

	template<typename U, std::size_t otherAlignment>
	friend class StackAllocator;

	using TSArena = SArena<tm_alignment>;

	TSArena* m_localArena;
public:
	using value_type = T;
	using size_type = std::size_t;
	using difference_type = ptrdiff_t;
	using pointer = T*;
	using const_pointer = T* const;
	using reference = T&;
	using const_reference = const T&;

	// defctor
	StackAllocator() noexcept
	{}
	// ctor
	StackAllocator( std::size_t bytes ) noexcept
	{
		m_localArena = new TSArena( bytes );
	}
	// dtor
	~StackAllocator()
	{}

	// cctors
	StackAllocator( const StackAllocator& rhs ) noexcept
	{// delegate to caop
		*this = rhs;
	}
	template<typename U>
	StackAllocator( const StackAllocator<U, tm_alignment>& rhs ) noexcept
	{
		*this = rhs;
	}

	// caops
	StackAllocator& operator=( const StackAllocator& rhs )
	{
		m_localArena = rhs.m_localArena;
		return *this;
	}
	template<typename U>
	StackAllocator& operator=( const StackAllocator<U, tm_alignment>& rhs )
	{
		m_localArena = rhs.m_localArena;
		return *this;
	}

	// mctors
	StackAllocator( const StackAllocator&& rhs ) noexcept
		:
		m_localArena{ rhs.m_localArena }
	{}
	template<typename Other>
	StackAllocator( const StackAllocator<Other>&& rhs ) noexcept
		:
		m_localArena{ rhs.m_localArena }
	{}

	// rebinding ctor to another allocator of type U
	template<typename U>
	struct rebind
	{
		using other = StackAllocator<U, tm_alignment>;
	};

	// get the address of a reference
	T* address( T& x ) const noexcept
	{
		return &x;
	}
	T* const address( const T& x ) const noexcept
	{
		return &x;
	}

	// allocates count * sizeof(T) bytes
	[[nodiscard]]
	T* allocate( std::size_t count,
		[[maybe_unused]] const void* hint = nullptr )
	{
		assert( m_localArena != nullptr );

		void* ret = m_localArena->allocate( count * sizeof( T ) );
		if ( ret == nullptr )
		{
			throw std::bad_alloc{};
		}

		return (T*)ret;
	}

	void deallocate( void* plastAllocationAddress,
		[[maybe_unused]] std::size_t count ) noexcept
	{
		assert( m_localArena != nullptr );
		m_localArena->deallocate( plastAllocationAddress );
	}

	template<typename... Args>
	void construct( T* p,
		Args&&... args ) const
	{
		new( p ) T( std::forward<Args>( args )... );
	}
	template<typename J, typename... Args>
	void construct( J* p,
		Args&&... args ) const
	{
		new( p ) J( std::forward<Args>( args )... );
	}

	void destroy( T* p ) const noexcept
	{
		p->~T();
	}
	template<typename J>
	void destroy( J* p ) const noexcept
	{
		p->~J();
	}

	// returns the largest possible value in Bytes that can meaningfully be passed to `allocate`
	std::size_t max_size() const noexcept
	{
		assert( m_localArena != nullptr );
		return m_localArena->getMaxSize();
	}

	void reset()
	{
		assert( m_localArena != nullptr );
		m_localArena->reset();
	}

	// get the memory pool used by this allocator
	const TSArena& getArena() const noexcept
	{
		return *m_localArena;
	}

	std::size_t getAvailableMemory() const noexcept
	{
		return m_localArena->getAvailableMemory();
	}

	constexpr std::size_t getAlignment() const noexcept
	{
		return m_localArena->getAlignment();
	}

	std::size_t getSize() const noexcept {
		return m_localArena->getMaxSize();
	}
};


// void specialization such that one particular allocator instance may allocate and construct different 
//			types of objects (see amap and mapstring example)
// it is usable, but you're advised to abstain and use concrete types in all cases
template<std::size_t tm_alignment>
class StackAllocator<void, tm_alignment>
{
	static_assert( isPowerOfTwo( tm_alignment ),
		"Stack Allocator's alignment value must be a power of 2." );

	template<typename U, std::size_t otherAlignment>
	friend class StackAllocator;

	using TSArena = SArena<tm_alignment>;

	TSArena* m_localArena;
public:
	using value_type = void;
	using size_type = std::size_t;
	using difference_type = ptrdiff_t;
	using pointer = void*;
	using const_pointer = void* const;

	// defctor
	StackAllocator() noexcept
	{}
	// ctor
	StackAllocator( std::size_t bytes ) noexcept
	{
		m_localArena = new TSArena( bytes );
	}
	// dtor
	~StackAllocator()
	{}

	// cctors
	StackAllocator( const StackAllocator& rhs ) noexcept
	{// delegate to caop
		*this = rhs;
	}
	template<typename U>
	StackAllocator( const StackAllocator<U, tm_alignment>& rhs ) noexcept
	{
		*this = rhs;
	}

	// caops
	StackAllocator& operator=( const StackAllocator& rhs )
	{
		m_localArena = rhs.m_localArena;
		return *this;
	}
	template<typename U>
	StackAllocator& operator=( const StackAllocator<U, tm_alignment>& rhs )
	{
		m_localArena = rhs.m_localArena;
		return *this;
	}

	// mctors
	StackAllocator( const StackAllocator&& rhs ) noexcept
		:
		m_localArena{ rhs.m_localArena }
	{}
	template<typename Other>
	StackAllocator( const StackAllocator<Other>&& rhs ) noexcept
		:
		m_localArena{ rhs.m_localArena }
	{}

	// rebinding ctor to another allocator of type U
	template<typename U>
	struct rebind
	{
		using other = StackAllocator<U, tm_alignment>;
	};

	[[nodiscard]]
	void* allocate( std::size_t count,
		const void* hint = nullptr )
	{
		assert( m_localArena != nullptr );
		std::size_t bytes = count * sizeof( void );

		void* ret = m_localArena->allocate( bytes );
		if ( ret == nullptr )
		{
			throw std::bad_alloc{};
		}

		return ret;
	}

	void deallocate( void* plastAllocation,
		[[maybe_unused]] std::size_t count = 0 )
	{
		assert( m_localArena != nullptr );
		m_localArena->deallocate( plastAllocation );
	}

	template<typename J,
		typename... Args>
	void construct( J* p,
		Args&&... args ) const
	{
		new( p ) J( std::forward<Args>( args )... );
		// or:
		//::new ( static_cast<void*>( p ) ) J( std::forward<Args>( args )... )
	}
	template<typename J>
	void destroy( J* p ) const noexcept
	{
		p->~J();
	}

	// returns the largest possible value in Bytes that can meaningfully be passed to `allocate`
	std::size_t max_size() const
	{
		assert( m_localArena != nullptr );
		return m_localArena->getMaxSize();
	}

	// reinitialize the allocator. All existing allocated memory will be lost
	void reset()
	{
		assert( m_localArena != nullptr );
		m_localArena->reset();
	}

	// get the memory pool used by this allocator
	const TSArena& getArena() const
	{
		assert( m_localArena != nullptr );
		return *m_localArena;
	}

	std::size_t getAvailableMemory() const noexcept
	{
		return m_localArena->getAvailableMemory();
	}

	std::size_t getSize() const noexcept
	{
		return m_localArena->getMaxSize();
	}
};


// an other allocator of the same type can deallocate from this one
template<typename T,
	std::size_t tm_alignment,
	std::size_t tm_otherAlignment>
inline bool operator==( const StackAllocator<T, tm_alignment>&,
	const StackAllocator<T, tm_otherAlignment>&)
{
	return true;
}
// equivalent statement:
template<typename T, std::size_t tm_alignment, std::size_t tm_otherAlignment>
inline bool operator!=( const StackAllocator<T, tm_alignment>&,
	const StackAllocator<T, tm_otherAlignment>& )
{
	return false;
}

// an other allocator of a different type cannot deallocate from this one
template<typename T,
	std::size_t tm_alignment,
	typename OtherAllocator>
inline bool operator==( const StackAllocator<T, tm_alignment>&,
	const OtherAllocator&)
{
	return false;
}
// equivalent statement:
template<typename T,
	std::size_t tm_alignment,
	typename OtherAllocator>
inline bool operator!=( const StackAllocator<T, tm_alignment>&,
	const OtherAllocator& )
{
	return true;
}