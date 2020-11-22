#pragma once

#include <type_traits>
#include <cstddef>

//======================================================================
// minimal stateless allocator
// C++11 and onwards compatible
template <class T>
struct Allocator
{
	using value_type = T;
	//using size_type = size_t;
	//using difference_type = std::ptrdiff_t;

	Allocator() {
		static_assert(isPowerOfTwo(alignment), "Alignment value must be a power of 2.");
	}

	template<class U>
	Allocator(const Allocator<U>&) noexcept
	{

	}

	/// allocates memory equal to count of objects of type T
	[[nodiscard]]
	value_type* allocate(std::size_t n)
	{
		if (n > std::size_t(-1) / sizeof(T)) // if (n > max_size())
			throw std::bad_alloc{};
		if (value_type* p = static_cast<value_type*>(::operator new(sizeof(T) * n)))
			return p;
		throw std::bad_alloc{};
	}

	/// deallocates previously allocated memory
	void deallocate(value_type* p, [[maybe_unused]] std::size_t count) noexcept
	{
		::operator delete(p);
	}

	//constexpr size_type max_size() const noexcept {
	//	return static_cast<size_type>(std::numeric_limits<size_type>::max()) / sizeof(value_type);
	//}

	///// initialize elements with value
	//template<typename... Args>
	//void construct(value_type* p, Args&&... args)
	//{
	//	new (p) T(std::forward<Args>(args)...);
	//}

	///// destroy elements
	//void destroy(value_type* p) noexcept
	//{
	//	p->~T();
	//}

	Allocator select_on_container_copy_construction() const noexcept
	{
		return *this;
	}

	/// Your allocator must be CopyConstructible and MoveConstructible. If propagate_on_container_copy_assignment{} is true, your allocator must be CopyAssignable.
	/// If propagate_on_container_move_assignment{} is true, your allocator must be MoveAssignable.
	/// If propagate_on_container_swap{} is true, your allocator must be Swappable.
	/// If they exist, these operations should not propagate an exception out. However they do not need to be marked with noexcept.
	///	It would be best to mark them as noexcept, st traits like is_nothrow_copy_constructible<Allocator<T>> give the right answer.
	//using propagate_on_container_copy_assignment	= std::false_type;
	//using propagate_on_container_move_assignment	= std::false_type;
	//using propagate_on_container_swap				= std::false_type;
	//using is_always_equal							= std::is_empty<Allocator>;
};

template<class T, class U>
bool operator==(const Allocator<T>&, const Allocator<U>&) noexcept
{
	return true;
}

template<class T, class U>
bool operator!=(const Allocator<T>&, const Allocator<U>&) noexcept
{
	return false;
}