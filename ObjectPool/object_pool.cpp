#include <algorithm>
#include <ctime>
#include <exception>
#include <execution>
#include <fcntl.h>
#include <future>
#include <io.h>
#include <iostream>
#include <scoped_allocator>
#include <string>
#include <thread>
#include <vector>
#include "object_pool.h"
#include "..\..\KeyTimer\KeyTimer\timer.h"
//#include "..\..\Leak_Checker\assertions.cpp"
//#include "..\..\Leak_Checker\leak_checker.cpp"

//	template<typename T>
//using SA = std::scoped_allocator_adaptor<ObjectPool<T>>;

struct GameObject
{
	int x_, y_, z_;
	int m_cost;

	GameObject() = default;
	GameObject(int x, int y, int z, int cost)
		: x_(x), y_(y), z_(z), m_cost(cost)
	{}
};

struct Elf : GameObject
{
	Elf() = default;
	Elf(int x, int y, int z, int cost)
		: GameObject(x, y, z, cost)
	{
		std::cout << "Elf created" << '\n';
	}

	~Elf() noexcept
	{
		std::cout << "Elf destroyed" << '\n';
	}
	std::string m_cry = "\nA hymn for Gandalf\n";
};

struct Dwarf : GameObject
{
	Dwarf() = default;
	Dwarf(int x, int y, int z, int cost)
		: GameObject(x, y, z, cost)
	{
		std::cout << "dwarf created" << '\n';
	}

	~Dwarf() noexcept
	{
		std::cout << "dwarf destroyed" << '\n';
	}
	std::string m_cry = "\nFind more cheer in a graveyard\n";
};


int elvenFunc()
{
	thread_local ObjectPool<Elf> elvenPool{ 229 };
	for (int i = 0; i < elvenPool.getSize(); ++i)
	{
		Elf* elf = elvenPool.construct(i, i + 1, i + 2, 100);
		std::cout << elf->m_cry << '\n';
		elvenPool.destroy(elf);
	}

	return 1024;
}

int dwarvenFunc()
{
	thread_local ObjectPool<Dwarf> dwarvenPool{ 256 };
	for (int i = 0; i < dwarvenPool.getSize(); ++i)
	{
		Dwarf* dwarf = dwarvenPool.construct(i - 1, i - 2, i - 3, 100);
		std::cout << dwarf->m_cry << '\n';
		dwarvenPool.destroy(dwarf);
	}

	return 2048;
}


int main()
{
	std::ios_base::sync_with_stdio(false);
	srand(time(nullptr));

	/// testing the Pool's functionality
	std::vector<std::future<int>> vec;
	vec.reserve(4);
	vec.emplace_back(std::async(std::launch::async, elvenFunc));
	vec.emplace_back(std::async(std::launch::async, dwarvenFunc));
	vec.emplace_back(std::async(std::launch::async, elvenFunc));
	vec.emplace_back(std::async(std::launch::async, dwarvenFunc));

	int term = 0;
	try
	{
		std::for_each(std::execution::par, vec.begin(), vec.end(),
			[&term](std::future<int>& t)
			{
				int ret = t.get();
				std::cout << "thread brought me " << ret << '\n';
				term += ret;
			}
		);
	}
	catch (const std::exception& ex)
	{
		std::cout << ex.what() << '\n';
	}
	catch (...)
	{
		//const auto& ex = std::current_exception();
		std::cout << "unknown exception" << '\n';
	}
	std::cout << "Final word = " << term << '\n';

	/// testing the Pool's performance
	//ObjectPool<GameObject> gopool{ 1024 };
	//GameObject *go = gopool.allocate();
	//
	//*go = { 2, 3, 4, 100 };
	//
	//std::cout << go->x_ << ' ' << go->y_ << ' ' << go->z_ << ' ' << go->m_cost << '\n';
	//
	//gopool.deallocate(go);

//	Timer<std::chrono::nanoseconds> timer;
	//for (int i = 0; i < 100000; i++)
	//{
	//	delete new int{i};
	//}
	// ~31ms - Debug
	// ~5ms - Release

	//delete[] new int[100000];
	// ~1ms - Debug
	// ~750us - Release

	// whereas with the object pool:
//	ObjectPool<int> poolOfInts{100000};
//	
//	for (int i = 0; i < 100000; i++)
//	{
//		poolOfInts.destroy(poolOfInts.construct(i));
//	}
	// ~40ms - Debug
	// ~1ms - Release

	std::system( "pause" );
	return 0;
}