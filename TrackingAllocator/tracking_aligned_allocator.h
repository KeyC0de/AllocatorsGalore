#pragma once
#include <memory>
#include <cstddef>
#include <limits>
#include "aligned_allocations.h"
///-------------------------------------------------------------------------------------------------
/// \class TrackingAlignedAllocator
///	
///	\author Nikos Lazaridis (KeyC0de)
///	\version 1.0
/// \date 27/9/2019
///
/// only the address of the first byte is guaranteed to be aligned to boundary specified
/// C++03 + compatible allocator with tracking apability, - tracking the amount of allocation calls
///-------------------------------------------------------------------------------------------------
template<typename T, std::size_t alignment = alignof(std::max_align_t)>
class TrackingAlignedAllocator final
{
	static_assert(isPowerOfTwo(alignment), "Alignment value must be a power of 2.");
public:
	using value_type = T;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;
	/// optional (custom) aliases:
	using void_pointer = void*;
	using const_void_pointer = void* const;
	/// the following aliases are deprecated in C++17 and removed in C++20:
	using pointer = T*;
	using const_pointer = T* const;
	using reference = T& ;
	using const_reference = const T&;

	//using propagate_on_container_copy_assignment	= std::false_type;
	//using propagate_on_container_move_assignment	= std::true_type;
	//using propagate_on_container_swap				= std::true_type;
	//using is_always_equal							= std::is_empty<TrackingAlignedAllocator>;
private:
	std::size_t m_nallocations;
public:
	/// required
	TrackingAlignedAllocator()
		: m_nallocations{ 0 }	
	{}

	~TrackingAlignedAllocator()
	{}

	/// \struct Rebinding constructor - required
	//[[deprecated("rebind is deprecated in C++17 and will be removed in C++20")]]
	template<typename Other, std::size_t OtherAlignment = alignof(std::max_align_t)>
	struct rebind {
		using other = TrackingAlignedAllocator<Other, OtherAlignment>;
	};
	
	/// copy constructors - required
	TrackingAlignedAllocator(TrackingAlignedAllocator& rhs) noexcept
		: m_nallocations{ rhs.getAllocations() }
	{}
	template<typename Other, std::size_t OtherAlignment>
	TrackingAlignedAllocator(const TrackingAlignedAllocator<Other, OtherAlignment>& rhs) noexcept
		: m_nallocations{ rhs.getAllocations() }
	{}
	/// copy assignment operator
	template<typename Other, std::size_t OtherAlignment>
	TrackingAlignedAllocator& operator=(TrackingAlignedAllocator<Other, OtherAlignment>& rhs) noexcept
	{
		m_nallocations = rhs.getAllocations();
		return *this;
	}

	/// move ctors
	TrackingAlignedAllocator(TrackingAlignedAllocator&& rhs) noexcept
		: m_nallocations{ rhs.getAllocations() }
	{}
	template<typename Other, std::size_t OtherAlignment>
	TrackingAlignedAllocator(TrackingAlignedAllocator<Other, OtherAlignment>&& rhs) noexcept
		: m_nallocations{ rhs.getAllocations() }
	{}
	/// move assignment operator
	template<typename Other, std::size_t OtherAlignment>
	TrackingAlignedAllocator& operator=(TrackingAlignedAllocator<Other, OtherAlignment>&& rhs) noexcept
	{
		m_nallocations = rhs.getAllocations();
		return *this;
	}

	T* address(T& r) const
	{
		return &r;
	}
	const T* address(const T& cr) const
	{
		return &cr;
	}

	constexpr std::size_t getAlignment() const noexcept {
		return (alignment > sizeof(void*)) ? alignment : sizeof(void*);
	}

	/// allocates count * sizeof(T) bytes aligned to specified alignment - required
	[[nodiscard]]
	T* allocate(const std::size_t count)
	{
		if (count == 0) {
			//assert(false);
			throw std::bad_alloc{};
		}
		if (count > max_size()) {
			throw std::length_error("TrackingAlignedAllocator<T,alignment>::allocate - Invalid argument - Integer Overflow");
		}
		
		void_pointer p = alignedMalloc(sizeof(T) * count, getAlignment());
		++m_nallocations;
		return static_cast<T*>(p);
	}
	template <typename U>
	T* allocate(const std::size_t count, [[maybe_unused]] const U hint)
	{
		return allocate(count);
	}

	/// `p` must be a value returned by an earlier call to `allocate` that has not been invalidated by an intervening call to `deallocate` - required
	void deallocate(T* const p, [[maybe_unused]] const std::size_t n = 0)
	{
		--m_nallocations;
		alignedFree(p);
	}

	/// \typeparameter `args` are the constructor arguments for the object of type `J`
	//[[deprecated("construct() is deprecated in C++17 and will be removed in C++20")]]
	template<typename J, typename... Args>
	void construct(J* p, Args&&... args) const
	{
		new(p) J(std::forward<Args>(args)...);
	}

	//[[deprecated("destroy() is deprecated in C++17 and will be removed in C++20")]]
	template<typename J>
	void destroy(J* p) const noexcept
	{
		p->~J();
	}

	template<typename... Args>
	void construct(T* const p, Args&&... args) const
	{
		new(p) T(std::forward<Args>(args)...);
	}

	void destroy(T* const p) const noexcept
	{
		p->~T();
	}

	/// returns the largest possible value that can meaningfully be passed to `allocate`
	//[[deprecated("max_size() is deprecated and will be removed in C++20")]]
	constexpr std::size_t max_size() const noexcept {
		return static_cast<std::size_t>(std::numeric_limits<std::size_t>::max() / sizeof(value_type));
	}

	/// returns amount of distinct allocations made through this allocator object
	std::size_t getAllocations() const noexcept {
		return m_nallocations;
	}
};

/// compares two allocator objects
template<typename T, std::size_t alignment1, typename Other, std::size_t alignment2>
inline bool operator==(const TrackingAlignedAllocator<T, alignment1>&, const TrackingAlignedAllocator<Other, alignment2>&) noexcept
{
	return true;
}
template<typename T, std::size_t alignment1, typename Other, std::size_t alignment2>
inline bool operator!=(const TrackingAlignedAllocator<T, alignment1>&, const TrackingAlignedAllocator<Other, alignment2>&) noexcept
{
	return false;
}
