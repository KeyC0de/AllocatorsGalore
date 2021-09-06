#pragma once

#include "../allocator_utils.h"
#include "../assertions.h"


template<std::size_t t_alignment = alignof( std::max_align_t )>
class SArena
{
	static inline constexpr std::size_t m_alignment = t_alignment;

	char* m_pData;
	char* m_pOffset;
	std::size_t m_maxSize;

	struct Header final
	{
		std::size_t allocationAddress;
	};
public:
	static constexpr std::size_t getAlignment() noexcept
	{
		return m_alignment;
	}

	SArena()
		:
		m_pData{nullptr},
		m_pOffset{nullptr},
		m_maxSize{0}
	{

	}

	// Attention! DO NOT malloc inside the initializer list.
	SArena( std::size_t size )
		:
		m_maxSize(size)
	{
		ASSERT( m_maxSize > 0,
			"Invalid size!" );
		m_pData = (char*)alignedMalloc( m_maxSize,
			m_alignment );

		m_pOffset = m_pData;
		if ( m_pData == nullptr )
		{
			throw std::runtime_error{"alignedMalloc() failed"};
		}
	}

	~SArena()
	{
		if ( m_pData )
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
		if ( m_pData )
		{
			alignedFree( m_pData );
		}

		m_pData = std::move( rhs.m_pData );
		m_maxSize = rhs.m_maxSize;
		m_pOffset = rhs.m_pOffset;

		// destroy the other one
		rhs.m_pData = nullptr;
		rhs.m_maxSize = 0;
		rhs.m_pOffset = nullptr;
	}

	// the `bytes` to allocate - more will be allocated due to alignment padding and Header size
	// return the address to the start of this allocated memory region, or throw an exception if you can't
	[[nodiscard]]
	void* allocate( std::size_t bytes )
	{
		ASSERT( m_pData != nullptr,
			"m_pData is null!" );

		std::size_t currentAllocationStartAddress = alignForward( (std::size_t) m_pOffset
			+ sizeof( Header ),
			m_alignment );

#if defined _DEBUG && !defined NDEBUG
		std::cout << "allocating "
			<< bytes
			<< " bytes starting at address "
			<< currentAllocationStartAddress
			<< '\n';
#endif // _DEBUG

		// set the new offset
		m_pOffset = (char*) ( currentAllocationStartAddress + bytes );
		// check that we haven't run out of memory
		if ( m_pOffset <= m_pData + m_maxSize )
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

	void deallocate( void* pLastAllocationAddress,
		[[maybe_unused]] std::size_t count = 0) noexcept
	{
		//ASSERT( m_pData != nullptr,
		//	"m_pData is null!" );
		//
		//std::size_t headerAddress = (std::size_t) pLastAllocationAddress - sizeof( Header );
		//ASSERT( headerAddress >= (std::size_t) m_pData && headerAddress < getEndAddress(),
		//	"Invalid headerAddress" );
		//
		//// get the previous allocation address
		//std::size_t allocationSize = ((Header*)headerAddress)->allocationAddress;
		//
		//char* expectedOffset = (char*)( headerAddress + allocationSize );
		//char* desiredOffset = (char*)headerAddress;
		// or:
		*m_pOffset = *(char*)( (std::size_t) pLastAllocationAddress - sizeof( Header ) );
		return;
	}

	// reset the arena. All existing allocated memory will be lost
	void reset()
	{
		ASSERT( m_pData != nullptr,
			"m_pData is null!" );
		m_pOffset = m_pData;
	}

	std::size_t getAvailableMemory() const noexcept
	{
		return getEndAddress() - reinterpret_cast<std::size_t>( m_pOffset );
	}

	// GETTERS
	char* getStartAddress() const noexcept
	{
		return m_pData;
	}

	std::size_t getCurrentAddress() const noexcept
	{
		return (std::size_t) m_pData + (std::size_t) m_pOffset;
	}

	std::size_t getEndAddress() const noexcept
	{
		return (std::size_t) m_pData + m_maxSize;
	}

	char* getOffset() const noexcept
	{
		return m_pOffset;
	}

	std::size_t getMaxSize() const noexcept
	{
		return m_maxSize;
	}
};

//======================================================================
//	\class	StackAllocator
//
//	\author	Nikos Lazaridis (KeyC0de)
//	\date	30-Sep-19
//
//	\tparam	T : the type
//	\tparam	t_alignment : alignment value in bytes
//
//	\brief	The allocator uses a memory pool (an Arena) through which it requests and releases memory to its clients
//			The stack allocator is an extension of the Linear allocator, with the additional ability
//				that it can also deallocate.
//			Although the StackAllocator is copyable, the Arena is not, so effectively copies become moves. 
//			Caveat: requires LIFO order of deallocation calls.
//				Standard library types that use allocators or make indirect use of std::allocator_traits
//					do not operate in LIFO memory deallocation order by nature,
//					thus you have to make sure that deallocations follow this order.
//				This will mostly be invisible to the operator of the class,
//					however the error space has not been exhausted. Errors could come up.
//				For all other non-allocator users there is no problem.
//======================================================================
template<typename T, std::size_t t_alignment = alignof( std::max_align_t )>
class StackAllocator
{
	static_assert( isPowerOfTwo( t_alignment ),
		"Stack Allocator's alignment value must be a power of 2." );

	template<typename U, std::size_t t_otherAlignment>
	friend class StackAllocator;

	using TSArena = SArena<t_alignment>;

	TSArena* m_pArena;
public:
	using value_type = T;
	using size_type = std::size_t;
	using difference_type = ptrdiff_t;
	using pointer = T*;
	using const_pointer = T* const;
	using reference = T&;
	using const_reference = const T&;

	StackAllocator() noexcept
	{

	}

	StackAllocator( std::size_t bytes ) noexcept
	{
		m_pArena = new TSArena{bytes};
	}

	~StackAllocator()
	{

	}

	StackAllocator( const StackAllocator& rhs ) noexcept
	{// delegate to caop
		*this = rhs;
	}

	template<typename U>
	StackAllocator( const StackAllocator<U, t_alignment>& rhs ) noexcept
	{
		*this = rhs;
	}

	StackAllocator& operator=( const StackAllocator& rhs )
	{
		m_pArena = rhs.m_pArena;
		return *this;
	}

	template<typename U>
	StackAllocator& operator=( const StackAllocator<U, t_alignment>& rhs )
	{
		m_pArena = rhs.m_pArena;
		return *this;
	}

	StackAllocator( const StackAllocator&& rhs ) noexcept
		:
		m_pArena{rhs.m_pArena}
	{

	}

	template<typename Other>
	StackAllocator( const StackAllocator<Other>&& rhs ) noexcept
		:
		m_pArena{rhs.m_pArena}
	{

	}

	// rebinding ctor to another allocator of type U
	template<typename U>
	struct rebind
	{
		using other = StackAllocator<U, t_alignment>;
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
		ASSERT( m_pArena != nullptr,
			"m_pArena is null!" );

		void* ret = m_pArena->allocate( count * sizeof( T ) );
		if ( ret == nullptr )
		{
			throw std::bad_alloc{};
		}

		return (T*)ret;
	}

	void deallocate( void* plastAllocationAddress,
		[[maybe_unused]] std::size_t count ) noexcept
	{
		ASSERT( m_pArena != nullptr,
			"m_pArena is null!" );
		m_pArena->deallocate( plastAllocationAddress );
	}

	template<typename... TArgs>
	void construct( T* p,
		TArgs&&... args ) const
	{
		new( p ) T( std::forward<TArgs>( args )... );
	}
	
	template<typename U, typename... TArgs>
	void construct( U* p,
		TArgs&&... args ) const
	{
		new( p ) U( std::forward<TArgs>( args )... );
	}

	void destroy( T* p ) const noexcept
	{
		p->~T();
	}

	template<typename U>
	void destroy( U* p ) const noexcept
	{
		p->~U();
	}

	// returns the largest possible value in Bytes that can meaningfully be passed to `allocate`
	std::size_t max_size() const noexcept
	{
		ASSERT( m_pArena != nullptr,
			"m_pArena is null!" );
		return m_pArena->getMaxSize();
	}

	// reinitialize the allocator. All existing allocated memory will be lost
	void reset()
	{
		ASSERT( m_pArena != nullptr,
			"m_pArena is null!" );
		m_pArena->reset();
	}

	const TSArena& getArena() const noexcept
	{
		return *m_pArena;
	}

	std::size_t getAvailableMemory() const noexcept
	{
		return m_pArena->getAvailableMemory();
	}

	constexpr std::size_t getAlignment() const noexcept
	{
		return m_pArena->getAlignment();
	}

	std::size_t getSize() const noexcept
	{
		return m_pArena->getMaxSize();
	}
};


// void specialization such that one particular allocator instance may allocate and construct different 
// it is usable, but you're advised to abstain and use concrete types in all cases
template<std::size_t t_alignment>
class StackAllocator<void, t_alignment>
{
	static_assert( isPowerOfTwo( t_alignment ),
		"Stack Allocator's alignment value must be a power of 2." );

	template<typename U, std::size_t t_otherAlignment>
	friend class StackAllocator;

	using TSArena = SArena<t_alignment>;

	TSArena* m_pArena;
public:
	using value_type = void;
	using size_type = std::size_t;
	using difference_type = ptrdiff_t;
	using pointer = void*;
	using const_pointer = void* const;

	StackAllocator() noexcept
	{

	}

	StackAllocator( std::size_t bytes ) noexcept
	{
		m_pArena = new TSArena{bytes};
	}

	~StackAllocator()
	{

	}

	// cctors
	StackAllocator( const StackAllocator& rhs ) noexcept
	{// delegate to caop
		*this = rhs;
	}

	template<typename U>
	StackAllocator( const StackAllocator<U, t_alignment>& rhs ) noexcept
	{
		*this = rhs;
	}

	StackAllocator& operator=( const StackAllocator& rhs )
	{
		m_pArena = rhs.m_pArena;
		return *this;
	}

	template<typename U>
	StackAllocator& operator=( const StackAllocator<U, t_alignment>& rhs )
	{
		m_pArena = rhs.m_pArena;
		return *this;
	}

	StackAllocator( const StackAllocator&& rhs ) noexcept
		:
		m_pArena{rhs.m_pArena}
	{

	}

	template<typename Other>
	StackAllocator( const StackAllocator<Other>&& rhs ) noexcept
		:
		m_pArena{rhs.m_pArena}
	{

	}

	template<typename U>
	struct rebind
	{
		using other = StackAllocator<U, t_alignment>;
	};

	[[nodiscard]]
	void* allocate( std::size_t count,
		const void* hint = nullptr )
	{
		ASSERT( m_pArena != nullptr,
			"m_pArena is null!" );
		std::size_t bytes = count * sizeof( void );

		void* ret = m_pArena->allocate( bytes );
		if ( ret == nullptr )
		{
			throw std::bad_alloc{};
		}

		return ret;
	}

	void deallocate( void* plastAllocation,
		[[maybe_unused]] std::size_t count = 0 )
	{
		ASSERT( m_pArena != nullptr,
			"m_pArena is null!" );
		m_pArena->deallocate( plastAllocation );
	}

	template<typename U,
		typename... TArgs>
	void construct( U* p,
		TArgs&&... args ) const
	{
		new( p ) U( std::forward<TArgs>( args )... );
	}
	template<typename U>
	void destroy( U* p ) const noexcept
	{
		p->~U();
	}

	std::size_t max_size() const
	{
		ASSERT( m_pArena != nullptr,
			"m_pArena is null!" );
		return m_pArena->getMaxSize();
	}

	void reset()
	{
		ASSERT( m_pArena != nullptr,
			"m_pArena is null!" );
		m_pArena->reset();
	}

	const TSArena& getArena() const
	{
		ASSERT( m_pArena != nullptr,
			"m_pArena is null!" );
		return *m_pArena;
	}

	std::size_t getAvailableMemory() const noexcept
	{
		return m_pArena->getAvailableMemory();
	}

	std::size_t getSize() const noexcept
	{
		return m_pArena->getMaxSize();
	}
};


// an other allocator of the same type can deallocate from this one
template<typename T, std::size_t t_alignment, std::size_t t_otherAlignment>
inline bool operator==( const StackAllocator<T, t_alignment>&,
	const StackAllocator<T, t_otherAlignment>&)
{
	return true;
}
// equivalent statement:
template<typename T, std::size_t t_alignment, std::size_t t_otherAlignment>
inline bool operator!=( const StackAllocator<T, t_alignment>&,
	const StackAllocator<T, t_otherAlignment>& )
{
	return false;
}

// an other allocator of a different type cannot deallocate from this one
template<typename T, std::size_t t_alignment, typename OtherAllocator>
inline bool operator==( const StackAllocator<T, t_alignment>&,
	const OtherAllocator&)
{
	return false;
}
// equivalent statement:
template<typename T, std::size_t t_alignment, typename OtherAllocator>
inline bool operator!=( const StackAllocator<T, t_alignment>&,
	const OtherAllocator& )
{
	return true;
}
