#pragma once

#include <type_traits>
#include <cstddef>
#include "../allocator_utils.h"


//======================================================================
// minimal stateless aligned allocator C++11 compatible
template <class T/*, std::size_t alignment = alignof( std::max_align_t )*/>
struct AlignedAllocator
{
	using value_type = T;

	AlignedAllocator()
	{
		/*static_assert( isPowerOfTwo( alignment ),
			"Alignment value must be a power of 2." );
		*/
	}

	template<class U>
	AlignedAllocator( const AlignedAllocator<U>& ) noexcept
	{

	}
	
	// allocates memory equal to count of objects of type T
	template<std::size_t alignment = alignof( std::max_align_t )>
	[[nodiscard]]
	T* allocate( std::size_t n/*, std::size_t alignment = alignof(std::max_align_t)*/ )
	{
		if ( n > std::size_t( -1 ) / sizeof( T ) )
		{
			throw std::bad_alloc{};
		}
		if ( T* p = static_cast<T*>( alignedMalloc( sizeof( T ) * n, alignment ) ) )
		{
			return p;
		}
		throw std::bad_alloc{};
	}

	void deallocate( value_type* p,
		[[maybe_unused]] std::size_t count ) noexcept
	{
		alignedFree( p );
	}
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