#pragma once

#include <type_traits>
#include <cstddef>
#include "aligned_allocations.h"

//======================================================================
// minimal stateless aligned allocator
// C++11 and onwards compatible
template <class T/*, std::size_t alignment = alignof( std::max_align_t )*/>
struct AlignedAllocator
{
	using value_type = T;
	//using size_type = size_t;
	//using difference_type = std::ptrdiff_t;

	AlignedAllocator()
	{
		/*static_assert( isPowerOfTwo( alignment ),
			"Alignment value must be a power of 2." );
		*/
	}

	template<class U>
	AlignedAllocator( const AlignedAllocator<U>& ) noexcept
	{}
	
	// allocates memory equal to count of objects of type T
	template<std::size_t alignment = alignof( std::max_align_t )>
	[[nodiscard]]
	T* allocate( std::size_t n/*, std::size_t alignment = alignof(std::max_align_t)*/)
	{
		if ( n > std::size_t( -1 ) / sizeof( T ) ) // if (n > max_size())
		{
			throw std::bad_alloc{};
		}
		if ( T* p = static_cast<T*>( alignedMalloc( sizeof( T ) * n, alignment ) ) )
		{
			return p;
		}
		throw std::bad_alloc{};
	}

	// deallocates previously allocated memory
	void deallocate( T* p,
		[[maybe_unused]] std::size_t count ) noexcept
	{
		alignedFree( p );
	}

	//constexpr size_type max_size() const noexcept
	//{
	//	return static_cast<size_type>( std::numeric_limits<size_type>::max() ) / sizeof( T );
	//}

	//// initialize elements with value
	//template<typename... Args>
	//void construct( T* p, Args&&... args )
	//{
	//	new (p) T( std::forward<Args>( args )... );
	//}
	//
	///// destroy elements
	//void destroy( T* p ) noexcept
	//{
	//	p->~T();
	//}
	//
	//AlignedAllocator select_on_container_copy_construction() const noexcept
	//{
	//	return *this;
	//}
};

template<class T, class U>
bool operator==( const AlignedAllocator<T>&,
	const AlignedAllocator<U>& ) noexcept
{
	return true;
}

template<class T, class U>
bool operator!=( const AlignedAllocator<T>&,
	const AlignedAllocator<U>& ) noexcept
{
	return false;
}