#pragma once

#include <memory>
#include <new>
#include <type_traits>

//=============================================================
// \class	ObjectPool
//
// \author	Nikos Lazaridis (KeyC0de)
// \date	3-Oct-19
//
//
// \brief	This is NOT thread safe.You should create a different instance for each thread(suggested) 
//				or find some way of scheduling queries to the allocator.
//=============================================================
template<typename T>
class ObjectPool
{
	union Object
	{
		std::aligned_storage_t<sizeof( T ), alignof( T )> m_storage;
		Object* next;
	};
	std::unique_ptr<Object[]> m_pool;
	Object* m_nextFree;
	std::size_t m_size;
public:
	using value_type = T;
	using pointer = T*;

	// constructor creates the pool given its size
	explicit ObjectPool( const std::size_t size )
		:
		m_pool{ std::make_unique<Object[]>( size ) },
		m_nextFree{nullptr},
		m_size( size )
	{
		for ( int i = 1; i < size; ++i )
		{
			m_pool[i-1].next = &m_pool[i];
		}

		m_nextFree = &m_pool[0];
		//m_nextFree->next = nullptr;
	}

	~ObjectPool() noexcept = default;

	ObjectPool( const ObjectPool& rhs ) = delete;
	ObjectPool& operator=( const ObjectPool& rhs ) = delete;

	ObjectPool( ObjectPool&& rhs ) noexcept
		:
		m_pool{std::move( rhs.m_pool )},
		m_nextFree{rhs.m_nextFree},
		m_size{ rhs.getSize }
	{
		rhs.m_nextFree = nullptr;
	}

	ObjectPool& operator=( ObjectPool&& rhs ) noexcept
	{
		if (this != &rhs)
		{
			m_size = rhs.getSize();
			std::swap( m_pool, rhs.m_pool );
			m_nextFree = rhs.m_nextFree;
			rhs.m_nextFree = nullptr;
		}
		return *this;
	}

	template <typename U>
	struct rebind
	{
		using otherAllocator = ObjectPool<U>;
	};

	T* address( T& r ) const noexcept
	{
		return &r;
	}
	const T* address( const T& r ) const noexcept
	{
		return &r;
	}

	// add object to the list
	[[nodiscard]]
	T* allocate()
	{
		if ( m_nextFree == nullptr )
		{
			throw std::bad_alloc{};
		}

		const auto currentObj = m_nextFree;
		m_nextFree = currentObj->next;

		return reinterpret_cast<T*>( &currentObj->m_storage );
	}

	// remove object from the list
	void deallocate( T* p,
		[[maybe_unused]] std::size_t count = 0 ) noexcept
	{
		const auto o = reinterpret_cast<Object*>( p );
		o->next = m_nextFree;
		m_nextFree = o;
	}

	template<typename... TArgs>
	[[nodiscard]]
	T* construct( TArgs... args )
	{
		return new ( allocate() ) T{std::forward<TArgs>( args )...};
	}

	void destroy( T* p ) noexcept
	{
		if ( p == nullptr )
		{
			return;
		}

		p->~T();
		deallocate( p );
	}

	std::size_t getSize() noexcept
	{
		return m_size;
	}
};

template <class T, class Other>
bool operator==( const ObjectPool<T>& lhs,
	const ObjectPool<Other>& rhs ) noexcept
{
	return lhs.m_pool == rhs.m_pool;
}

template <class T, class Other>
bool operator!=( const ObjectPool<T>& lhs,
	const ObjectPool<Other>& rhs ) noexcept
{
	return lhs.m_pool != rhs.m_pool;
}