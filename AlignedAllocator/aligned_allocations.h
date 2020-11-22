#pragma once

#include <cstddef>
#include <cassert>

constexpr bool isPowerOfTwo( const std::size_t value ) noexcept
{
	return ( value != 0 && ( value & ( value - 1 ) ) == 0 );
}

//  check whether the address is aligned to `alignment` boundary
bool isAligned( const volatile void *p,
	std::size_t alignment )
{
	return ( reinterpret_cast<std::uintptr_t>( p ) % alignment ) == 0;
}
bool isAligned( std::uintptr_t pi,
	std::size_t alignment )
{
	return ( pi % alignment ) == 0;
}

/// align pointer forward with given alignment
template<typename T>
T* alignForward( T* p,
	std::size_t alignment ) noexcept
{
	if ( alignment == 0 )
	{
		return p;
	}
	const std::uintptr_t ip = reinterpret_cast<std::uintptr_t>( p );
	if ( ip % alignment == 0 )
	{
		return p;
	}
	return reinterpret_cast<T*>( ( ip + ( alignment - 1 ) ) & ~( alignment - 1 ) );
	// or: (ip + alignment - 1) / alignment * alignment;
}

std::uintptr_t alignForward( std::uintptr_t ip,
	std::size_t alignment ) noexcept
{
	if ( alignment == 0 )
	{
		return ip;
	}
	if ( ip % alignment == 0 )
	{
		return ip;
	}
	return ( ip + ( alignment - 1 ) ) & ~( alignment - 1 );
	// or: (ip + alignment - 1) / alignment * alignment;
}

// calculates alignment in bits supposedly
std::size_t calcAlignedSize( std::size_t size,
	std::size_t alignment )
{
	return size + ( size % ( alignment / 8 ) );
}

// calculate padding bytes needed to align address p forward given the alignment
const std::size_t getForwardPadding( const std::size_t p,
	const std::size_t alignment )
{
	const std::size_t mult = ( p / alignment ) + 1;
	const std::size_t alignedAddress = mult * alignment;
	const std::size_t padding = alignedAddress - p;
	return padding;
}

const std::size_t getForwardPaddingWithHeader( const std::size_t p,
	const std::size_t alignment,
	const std::size_t headerSize )
{
	std::size_t padding = getForwardPadding( p,
		alignment );
	std::size_t neededSpace = headerSize;

	if ( padding < neededSpace )
	{
		// Header does not fit - Calculate next aligned address that header fits
		neededSpace -= padding;

		// How many alignments I need to fit the header        
		if ( neededSpace % alignment > 0 )
		{
			padding += alignment * ( 1 + ( neededSpace / alignment ) );
		}
		else
		{
			padding += alignment * ( neededSpace / alignment );
		}
	}
	return padding;
}

template<typename T>
T* alignPtr( const T *ptr,
	const std::size_t alignment )
{
	const std::uintptr_t uintPtr = reinterpret_cast<std::uintptr_t>( ptr );
	const std::uintptr_t alignedUintPtr = ( uintPtr + ( alignment - 1 ) ) & ~( alignment - 1 );
	T* alignedPtr = reinterpret_cast<T*>( alignedUintPtr );
	assert( isAligned( alignedPtr, alignment ) );
	return alignedPtr;
}

// allocate `bytes` + `bytesForAlignment` required given requested `alignment` value
// store malloced address in `pmem`
// compute aligned address `palignedMem` by adding the `bytesForAdjustment` to malloced `pmem` address
// TODO: doesn't work properly
void* _alignedMalloc(std::size_t bytes, std::size_t alignment)
{
	assert( false );
	void *pmem = nullptr;
	std::size_t bytesForAlignment = alignment - 1 + sizeof(void*);
	if ( ( pmem = static_cast<void*>( ::operator new( bytes + bytesForAlignment ) ) ) == nullptr )
	{
		return nullptr;
	}
	// round-up / align address forward
	std::size_t bytesForAdjustment = ( bytesForAlignment ) & ~( alignment - 1 );
	void** palignedMem = reinterpret_cast<void**>( reinterpret_cast<std::size_t>( pmem )
		+ bytesForAdjustment );
	palignedMem[-1] = pmem;
	return palignedMem;
}

void _alignedFree( void *p ) noexcept
{
	assert(false);
	::operator delete( static_cast<void**>( p )[-1] );
}

void* alignedMalloc( std::size_t count,
	std::size_t alignment )
{
	void* p;
#if defined(_MSC_VER) || defined(_WIN32) || defined(_WIN64)
	p = _aligned_malloc( count,
		alignment );
	if ( p == nullptr )
	{
		throw std::bad_alloc();
	}
#elif defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))
	if ( posix_memalign( &p, alignment, count ) )
	{
		throw std::bad_alloc();	// p = 0
	}
#endif
	assert( isAligned( p, alignment ) );
	return p;
};	//  The address of the allocated memory (returned by p) will be a multiple of alignment, which must be a power of two and a multiple of sizeof(void *)

void alignedFree( void *p ) noexcept
{
#if defined(_MSC_VER) || defined(_WIN32) || defined(_WIN64)
	_aligned_free( p );
#elif defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))
	free( p );
#endif
}

// INTEL:
//void* _mm_malloc(int size, int align)
//void _mm_free(void *p)
