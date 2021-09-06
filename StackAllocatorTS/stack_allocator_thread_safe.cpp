#include <deque>
#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <vector>
#include <scoped_allocator>
#include "stack_allocator_thread_safe.h"


template<typename T, std::size_t alignment = alignof(std::max_align_t)>
using SA = std::scoped_allocator_adaptor<StackAllocatorTS<T, alignment>>;

struct GameObject
{
	int32_t x, y, z;
	int32_t cost;
};


int stressTest()
{
	std::cout << "Preliminary" << '\n';
	// Example usage of StackAllocatorTS
	StackAllocatorTS<char> alloc(128);
	using Str = std::basic_string<char, std::char_traits<char>, StackAllocatorTS<char>>;

	std::cout << "sizeof(Str)=" << sizeof(Str) << '\n' << "sizeof(std::string)=" << sizeof(std::string) << '\n';
	Str str(alloc);
	str = "lalala";
	str = "lalalalo";

	// Example usage of StackAllocatorTS No2
	GameObject go{ 43, 54, 85, 200 };
	std::cout << "x=" << go.x
		<< "y=" << go.y
		<< "z=" << go.z
		<< "cost=" << go.cost;

	std::cout << "sizeof(StackAllocatorTS<GameObject,64>)=" << sizeof(StackAllocatorTS<GameObject, 64>) << '\n';

	std::cout << "sizeof(StackAllocatorTS)=" << sizeof(StackAllocatorTS<GameObject>) << '\n';
	std::cout << "10 * sizeof(GameObject) =" << 10 * sizeof(GameObject) << '\n';

	std::cout << "\nVector" << '\n';

	StackAllocatorTS<GameObject> fooalloc(7000);
	std::vector<GameObject, StackAllocatorTS<GameObject>> vec(fooalloc);

	for (int32_t i = 0; i < 100; i++)
	{
		vec.push_back(GameObject{ i, i, i, i });
	}

	std::cout << "vec allocator = " << typeid(vec.get_allocator()).name() << '\n';
	std::cout << "vec allocator alignment = " << vec.get_allocator().getAlignment() << '\n';


	std::cout << "\nVector with special alignment" << '\n';
	StackAllocatorTS<GameObject, sizeof(GameObject)> goAlloc{ 10500 };
	using GameObjectVector = std::vector<GameObject, SA<GameObject, sizeof(GameObject)>>;
	GameObjectVector govector{ goAlloc };
	std::cout << std::is_copy_constructible_v<GameObject> << '\n';
	for (int32_t i = -80; i < 80; i++)
	{
		govector.emplace_back(GameObject{ i, i + 1, i + 2, i + 3 });
	}

	for (const auto& a : govector)
		std::cout << a.x << ' ' << a.y << ' ' << a.z << ' ' << a.cost << '\n';

	std::cout << "govector allocator = " << typeid(govector.get_allocator()).name() << '\n';
	std::cout << "govector allocator alignment = " << govector.get_allocator().getAlignment() << '\n';


	std::cout << "\nDeque" << '\n';

	using astring = std::basic_string<char, std::char_traits<char>, SA<char>>;

	StackAllocatorTS<astring> la5{ 8192 };
	std::deque<astring, SA<astring>> dq{ la5 };
	for (int i = 0; i < 6; i++)
	{
		dq.emplace_back("Hello");
		dq.emplace_back("w/e");
		dq.emplace_back("whatever");
		dq.emplace_back("there is ist sofi j");
		dq.emplace_back("there's more than meets the eye");
		dq.emplace_back("Alice");
		dq.emplace_back("Jackie");
		dq.emplace_back("Hirohito");
		dq.emplace_back("Jean Claude Van Damme");
	}
	for (const auto &s : dq)
	{
		std::cout << s << '\n';
	}
	std::cout << "getting the allocator's type = " << typeid(dq.get_allocator()).name() << '\n';
	std::cout << "getting the arena's type = " << typeid(dq.get_allocator().getArena()).name() << '\n';


	std::cout << "\nStrings" << '\n';
	using overaligned_string = std::basic_string<char, std::char_traits<char>, SA<char, 32>>;
	StackAllocatorTS<char, 32> sa{ 2048 };
	overaligned_string s1{ "Short string", sa };
	overaligned_string s2{ "My name is Maximus Decimus Meridius.\n"
				"Commander of the armies of the North.\n"
				"General of the Phoelix legions.\n"
				"Loyal servant to the true emperor, Marcus Aurelius.\n"
				"Father to a murdered son, husband to a murdered wife\n"
				" and I will have my vengeance, in this life or the m_pNext.\n", sa };
	std::cout << s1 << '\n';
	std::cout << s2 << '\n';

	std::cout << "isAligned(s1.data(), 32)=" << isAligned(s1.data(), 32) << '\n';	// address on the dynamically allocated string
	std::cout << "isAligned(&s1, 32)=" << isAligned(&s1, 32) << '\n';				// address of the stack object
	std::cout << "isAligned(s2.data(), 32)=" << isAligned(s2.data(), 32) << '\n';

	/// for some reason this section fails
	//std::cout << "\nAlign each element of an array:" << '\n';
	//const std::size_t intAlignment = 16;
	//struct AlignedInt {
	//	alignas(intAlignment) int aint;
	//};
	//StackAllocatorTS<AlignedInt, intAlignment> LA{ 10000 };
	//AlignedInt* palignedInts = LA.allocate(64);
	//for (int i = 0; i < 64; i++)
	//	(palignedInts + sizeof(AlignedInt) * i)->aint = i;
	//for (int i = 0; i < 64; i++)
	//	std::cout << (palignedInts + sizeof(AlignedInt) * i)->aint << ' ' << isAligned(((std::size_t) palignedInts + sizeof(AlignedInt) * i), intAlignment) << '\n';
	//// it's aligned!


	std::cout << "\nMap with Scoped Allocator adaptor" << '\n';
	using mapstring = std::basic_string<char, std::char_traits<char>, SA<char, 16>>;
	using amap = std::map<mapstring, int, std::less<>, SA<std::pair<mapstring const, int>, 16>>;
	StackAllocatorTS<void, 16> la1{ 2048 };

	std::cout << sizeof(amap) << '\n';
	std::cout << sizeof(std::map<std::string, int>) << '\n';

	mapstring ms1{ "Topper Harley", la1 };
	amap am{ la1 };
	am[{"foo", la1}] = 10;
	am[{"fooooooooooooooooooooooooooo", la1}] = 20;
	am[{"hidfsaf255555555555444444444", la1}] = 200;
	am[{"5444444444", la1}] = 2;

	am[ms1] = 30;
	std::cout << "available memory = " << la1.getAvailableMemory() << '\n';
	am[mapstring{ std::move(s2), la1 }] = 50;
	std::cout << "available memory = " << la1.getAvailableMemory() << '\n';
	am[{"baaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaba", la1}] = 50005;

	auto it = am.find("foo");
	//if (it != am.end())
	//	it->second *= 10;
	for (const auto& a : am)
		std::cout << a.first << ' ' << a.second << '\n';


	std::cout << "\nMap with StackAllocatorTS bare" << '\n';
	using smapstring = std::basic_string<char, std::char_traits<char>, StackAllocatorTS<char, 16>>;
	using samap = std::map<smapstring, int, std::less<>, StackAllocatorTS<std::pair<smapstring const, int>, 16>>;
	StackAllocatorTS<smapstring, 16> la2{ 2048 };

	std::cout << sizeof(samap) << '\n';
	std::cout << sizeof(std::map<std::string, int>) << '\n';

	smapstring ms2{ "Topper Harley", la2 };
	samap sam{ la2 };
	sam[{"foo", la2}] = 10;
	sam[{"fooooooooooooooooooooooooooo", la2}] = 20;
	sam[{"hidfsaf255555555555444444444", la2}] = 200;
	sam[{"5444444444", la2}] = 2;

	sam[ms2] = 30;
	std::cout << "available memory = " << la2.getAvailableMemory() << '\n';
	std::cout << "available memory = " << la2.getAvailableMemory() << '\n';
	sam[{"baaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaba", la2}] = 50005;

	auto sit = sam.find("foo");
	//if (sit != sam.end())
	//	sit->second *= 10;
	for (const auto& a : sam)
		std::cout << a.first << ' ' << a.second << '\n';


	std::cout << "\nIndividual allocations" << '\n';
	StackAllocatorTS<int, 256> la6(1024);
	int* pi = la6.allocate(1);
	new(pi) int{ 4 };
	std::cout << *pi << '\n';
	std::cout << "isAligned(pi, 256)=" << isAligned(pi, 256) << '\n';

	int* pint = la6.allocate(1);
	*pint = 1453;
	std::cout << *pint << '\n';
	std::cout << "isAligned(pint, 256)=" << isAligned(pint, 256) << '\n';

	std::cout << "few allocator tests.." << '\n';
	{
		StackAllocatorTS<int, 16> salo{ 1024 };
		StackAllocatorTS<int, 16> salo2{ salo };
		//StackAllocatorTS<int, 32> salo3{salo2}; // incompatible!
		StackAllocatorTS<int, 16> salo4 = std::move(salo);
		StackAllocatorTS<char, 16> saloc{ salo2 };
		// StackAllocatorTS<char, 32> saloc2 = std::move(salo2);	// nope!
		// StackAllocatorTS<char, 32> saloc3 = std::move(saloc);	// nope!
		StackAllocatorTS<void, 16> saloc4 = std::move(saloc);
		StackAllocatorTS<void, 16> saloc5{ saloc4 };
		std::cout << (saloc == saloc4) << '\n';			// false!
		std::cout << (saloc4 == saloc5) << '\n';		// true!
	}// die!

	return EXIT_SUCCESS;
}



int main()
{
	std::cout << std::boolalpha << '\n';

	std::vector<std::thread> threads{std::thread::hardware_concurrency()};

	for ( auto& t : threads )
	{
		t = std::thread{stressTest};
	}

	for( auto& t : threads )
	{
		if ( t.joinable() )
		{
			t.join();
		}
	}

	std::system( "pause" );
	return 0;
}