#include <algorithm>
#include <deque>
#include <map>
#include <scoped_allocator>
#include <string>
#include <utility>
#include <vector>
#include "linear_allocator_thread_safe.h"


template<typename T, std::size_t TAlignment = alignof(std::max_align_t)>
using SA = std::scoped_allocator_adaptor<LinearAllocator<T, TAlignment>>;

int main()
{
	std::cout << std::boolalpha << '\n';

	Arena<16> arena1{ 1024 };
	Arena<16> arena2{ 1536 };
	LinearAllocator<void, 16> la1{ &arena1 };
	LinearAllocator<void, 16> la2{ &arena2 };

	using astring = std::basic_string<char, std::char_traits<char>, SA<char, 16>>;
	std::cout << sizeof(astring) << ' ' << sizeof(int) << '\n';
	std::cout << alignof(astring) << '\n';
	std::cout << sizeof(std::string) << '\n';
	using amap = std::map<astring, int, std::less<>, SA<std::pair<astring const, int>, 16>>;
	std::cout << sizeof(amap) << '\n';
	std::cout << sizeof(std::map<std::string, int>) << '\n';
	astring s1{ "Short string", la1 };
	astring s2{ "My name is Maximus Decimus Meridius.\n"
				"Commander of the armies of the North.\n"
				"General of the Phoelix legions.\n"
				"Loyal servant to the true emperor, Marcus Aurelius.\n"
				"Father to a murdered son, husband to a murdered wife\n"
				" and I will have my vengeance, in this life or the next.\n", la1 };

	std::cout << s1 << '\n';
	std::cout << s2 << '\n';
	amap am{ la2 };
	am[{"foo", la1}] = 10;
	am[{"fooooooooooooooooooooooooooo", la1}] = 20;
	am[{"hidfsaf255555555555444444444", la1}] = 200;
	am[{"5444444444", la1}] = 2;

	std::cout << "available memory = " << la2.getAvailableMemory() << '\n';
	am[s1] = 30;
	std::cout << "available memory = " << la2.getAvailableMemory() << '\n';
	am[astring{ std::move(s2), la1 }] = 50;
	std::cout << "available memory = " << la2.getAvailableMemory() << '\n';
	am[{"baaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaba", la2}] = 50005;

	auto it = am.find("foo");
	//if (it != am.end())
	//	it->second *= 10;
	for (const auto& a : am)
		std::cout << a.first << ' ' << a.second << '\n';


	Arena arena3{ 1024 };
	LinearAllocator<void> la3{ &arena3 };
	std::cout << la3.getAvailableMemory() << '\n';
	LinearAllocator<void> la4 = std::move(la3);
	std::cout << la4.getAvailableMemory() << '\n';
	std::cout << la4.getAlignment() << '\n';

	//la3.allocate(250);	// can't create objects of type void - alignof(void) is illegal!

	std::vector<int, SA<int>> va{ 100, la4 };
	for (int i = 0; i < 10; i++)
	{
		va.push_back(i * 11);
	}
	for (const auto& a : va)
		std::cout << a << '\n';

	using overaligned_string = std::basic_string<char, std::char_traits<char>, SA<char, 32>>;

	Arena<32> arena5{ 4096 };
	LinearAllocator<void, 32> la5{ &arena5 };
	std::deque<overaligned_string, SA<overaligned_string, 32>> dq{ la5 };
	dq.emplace_back("Hello");
	dq.emplace_back("w/e");
	dq.emplace_back("whatever");
	dq.emplace_back("there is ist sofi j");
	dq.emplace_back("there's more than meets the eye");
	dq.emplace_back("Alice");
	dq.emplace_back("Jackie");
	for (const auto &s : dq)
	{
		std::cout << s << '\n';
	}
	std::cout << typeid(dq.get_allocator()).name() << '\n';



	std::cout << "Individual allocations" << '\n';
	Arena<256> arena4{ 1024 };
	LinearAllocator<int, 256> la6(&arena4);
	int* vi = la6.allocate(1);
	new(vi) int{ 4 };
	std::cout << *vi << '\n';
	std::cout << "isAligned(pi, 256)=" << isAligned(vi, 256) << '\n';

	int* pint = la6.allocate(1);
	*pint = 1453;
	std::cout << *pint << '\n';
	std::cout << "isAligned(pint, 256)=" << isAligned(pint, 256) << '\n';



	//std::cout << "\nAlign each element of an array:" << '\n';
	// for some reason this doesn't work in release mode!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	constexpr std::size_t intAlignment = 16;
	Arena<intAlignment> arena05{ 10000 };
	struct AlignedInt {
		alignas(intAlignment) int aint;
	};
	LinearAllocator<AlignedInt, intAlignment> LA{ &arena05 };
	AlignedInt* palignedInts = LA.allocate(64);
	for (int i = 0; i < 64; i++)
		(palignedInts + sizeof(AlignedInt) * i)->aint = i;
	for (int i = 0; i < 64; i++)
		std::cout << (palignedInts + sizeof(AlignedInt) * i)->aint << ' ' << isAligned((palignedInts + sizeof(AlignedInt) * i), intAlignment) << '\n';
	// it's aligned!
	// up to here!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


	std::cout << "\nstd::string example" << '\n';
	using lstring = std::basic_string<char, std::char_traits<char>, SA<char, 128>>;
	Arena<128> arena7{ 600 };
	LinearAllocator<lstring, 128> sls{ &arena7 };
	lstring lstr{ "My name is Maximus Decimus Meridius.\n"
		"Commander of the armies of the North.\n"
		"General of the Phoelix legions.\n"
		"Loyal servant to the true emperor, Marcus Aurelius.\n"
		"Father to a murdered son, husband to a murdered wife\n"
		" and I will have my vengeance, in this life or the next.\n", sls };
	std::cout << lstr << '\n';
	std::cout << "isAligned(lstr.data(), 128)=" << isAligned(lstr.data(), 128) << '\n';	// address on the dynamically allocated string
	std::cout << "isAligned(&lstr, 128)=" << isAligned(&lstr, 128) << '\n';				// address of the stack object
	//auto start_address = &(lstr.begin());	//Take the address at the beginning
	//auto end_address = &(lstr.end());	//Take the address at the end
	std::cout << '\n' << '\n';



	std::cout << "Sample Game Object class" << '\n';
	struct GameObject
	{
		int32_t x, y, z;
		int32_t cost;
	};
	Arena sar{ 1024 };
	LinearAllocator<void> stralloc{ &sar };
	using Str = std::basic_string<char, std::char_traits<char>, SA<char>>;

	std::cout << "sizeof(Str)=" << sizeof(Str) << '\n' << "sizeof(std::string)=" << sizeof(std::string) << '\n';
	Str str{ stralloc };
	str = "lalala";
	std::cout << "str=" << str << '\n';
	str = "lalalalo";
	std::cout << "str=" << str << '\n';

	GameObject go{ 43, 54, 85, 200 };
	std::cout << "x=" << go.x
		<< "y=" << go.y
		<< "z=" << go.z
		<< "cost=" << go.cost;

	std::cout << "sizeof(LinearAllocator<GameObject,64>)=" << sizeof(LinearAllocator<GameObject, 64>) << '\n';

	Arena<sizeof(GameObject)> gos{ 20000 };
	LinearAllocator<GameObject, sizeof(GameObject)> goAlloc(&gos);
	using ClassVector = std::vector<GameObject, SA<GameObject, sizeof(GameObject)>>;
	ClassVector govector{ goAlloc };
	std::cout << "goAlloc.getAlignment()=" << goAlloc.getAlignment() << '\n';
	std::cout << std::is_copy_constructible_v<GameObject> << '\n';
	for (int32_t i = -20; i < 20; i++)
	{
		govector.emplace_back(GameObject{ i, i + 1, i + 2, i + 3 });
	}

	for (const auto& a : govector)
		std::cout << a.x << ' ' << a.y << ' ' << a.z << ' ' << a.cost << '\n';
}