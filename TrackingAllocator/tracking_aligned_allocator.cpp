#include <iostream>
#include <deque>
#include <vector>
#include <list>
#include <set>
#include <intrin.h>
#include <queue>
#include <map>
#include <string>
#include "tracking_aligned_allocator.h"


int main()
{
	std::ios_base::sync_with_stdio( false );
	int* Ai = new int[100];
#pragma warning( suppress: 6001 )
	std::cout << Ai[0] << '\n';
	std::cout << Ai[17] << '\n';
	std::cout << Ai[57] << '\n';
	int* Aii = new int[100]();
	std::cout << Aii[0] << '\n';
#pragma warning( suppress: 6385 6001 )
	std::cout << Aii[17] << '\n';
	std::cout << Aii[57] << '\n';

	delete[] Ai;
	delete[] Aii;

	int i1 = 5;
	int i2 = 543;

	if ( typeid( i1 ) == typeid( i2 ) )
	{
		std::cout << "Same\n";
	}
	else
	{
		std::cout << "Not same.\n";
	}

	// rebinding: 
	// a way to use an internal alias of an allocator to create another allocator able to work on a different type
	// In STL we have a default rebinding mechanism so we don't need to write our own.
	// But we will here for demo purposes.
	std::cout << "\nCustom allocator\n================\n";
	using TAInt = TrackingAlignedAllocator<int>;
	using TADbl = TrackingAlignedAllocator<double>;
	using TAdouble = TAInt::rebind<double>::other;

	std::vector<double, TADbl> v{5.0};
	for ( const auto& i : v )
	{
		std::cout << i << ' ';
	}

	std::cout << '\n'
		<< v.get_allocator().getAllocations()
		<< '\n';
	std::cout << "max size = "
		<< v.get_allocator().max_size()
		<< '\n';


	std::cout << "\nAllocator Conversions and Comparisons\n=====================================\n";
	TrackingAlignedAllocator<int> ai1;
	TrackingAlignedAllocator<int> ai2 = ai1;

	std::cout << std::boolalpha << '\n';
	std::cout << ( ai1 == ai2 ) << '\n';
	std::cout << ( ai1 != ai2 ) << '\n';

	TrackingAlignedAllocator<int> ai3{ai1};
	TrackingAlignedAllocator<int> ai4 = std::move( ai1 );
	TrackingAlignedAllocator<int> ai5{std::move( ai2 )};

	TrackingAlignedAllocator<int> ai11;
	TrackingAlignedAllocator<float> af = ai3;
	std::cout << ( ai11 == af ) << '\n';
	TrackingAlignedAllocator<float> af2 = std::move( ai4 );
	std::cout << ( ai11 == ai4 ) << '\n';
	std::cout << ( af == af2 ) << '\n';
	std::cout << ( af == af ) << '\n';


	std::cout << "\nNon-aggregates" << '\n';
	// alignment and size of a variable of fundamental type:
	alignas(128) int myVariable = 100;
	std::cout << "sizeof(int)=" << sizeof(int) << '\n';
	std::cout << "sizeof(myVariable)=" << sizeof(myVariable) << '\n';
	std::cout << "alignof(decltype(myVariable))=" << alignof(decltype(myVariable)) << '\n';
	std::cout << "isAligned(&myVariable, 128)=" << isAligned(&myVariable, 128) << '\n';

	alignas(256) std::string myStringVariable = "hello you got to live or be free for life f;dksj;lfj afl;jskfjlajfklaj;dfjsal";
	std::cout << "sizeof(std::string)=" << sizeof(std::string) << '\n';
	std::cout << "alignof(std::string)=" << alignof(std::string) << '\n';
	std::cout << "sizeof(myStringVariable)=" << sizeof(myStringVariable) << '\n';
	std::cout << "alignof(decltype(myStringVariable))=" << alignof(decltype(myStringVariable)) << '\n';
	std::cout << "isAligned(&myStringVariable, 256)=" << isAligned(&myStringVariable, 256) << '\n';

	TrackingAlignedAllocator<int, 128> TAA;
	int* dynamicallyAllocatedInteger = TAA.allocate(sizeof(myVariable));
	std::cout << "\ntracking aligned allocator fundamental type alignment" << '\n';
	std::cout << "isAligned(dynamicallyAllocatedInteger, 128)=" << isAligned(dynamicallyAllocatedInteger, 128) << '\n';

	TrackingAlignedAllocator<std::string, 256> TAAS;
	std::string* dynamicallyAllocatedString = TAAS.allocate(sizeof(myStringVariable));
	std::cout << "\ntracking aligned allocator std::string type alignment" << '\n';
	std::cout << "isAligned(dynamicallyAllocatedString, 256)=" << isAligned(dynamicallyAllocatedString, 256) << '\n';


	std::cout << "\nAggregates" << '\n';
	std::cout << "alignas" << '\n';
	// alignment and size of an array of fundamental types:
	alignas(64) int option[4];
	option[0] = 123;
	option[1] = 12321;
	option[2] = 123321;
	option[3] = 123123;
	std::cout << "sizeof(option)=" << sizeof(option) << '\n';
	std::cout << "alignof(decltype(option))=" << alignof(decltype(option)) << '\n';
	std::cout << "isAligned(&option[0], 64)=" << isAligned(option, 64) << '\n';
	std::cout << "sizeof(option[1])=" << sizeof(option[1]) << '\n';
	std::cout << "alignof(decltype(option[1]))=" << alignof(decltype(option[1])) << '\n';
	std::cout << "isAligned(&option[1], 64)=" << isAligned(&option[1], 64) << '\n';	// no!

	std::cout << "\ntracking aligned allocator aggregates" << '\n';
	std::vector<double, TrackingAlignedAllocator<double, 16>> vd;
	for (int i = 0; i < 100; i++)
	{
		vd.emplace_back(i * 1.1);
	}
	std::cout << "alignof(decltype(vd[2]))=" << alignof(decltype(vd[2])) << '\n';
	std::cout << "alignof(decltype(vvdec))=" << alignof(decltype(vd)) << '\n';
	std::cout << "isAligned(&vd[0], 16)=" << isAligned(&vd[0], 16) << '\n';


	std::cout << "\nSTL containers" << '\n';
	std::vector<std::string> myVec(100);
	void* initialVectorAddress = myVec.data();
	std::cout << "&myVec[0]=" << myVec.data() << '\n';
	//myVec.reserve(100);	// reallocation will occur with .reserve
	//myVec.reserve(10);	// if we don't specify an initial capacity
	//myVec.reserve(110);	// unless the capacity we specify in .reseve is larger than its present capacity
	void* vectorAddressAfterSettingCapacity = myVec.data();
	std::cout << "&myVec[0]=" << myVec.data() << '\n';
	if (reinterpret_cast<std::uintptr_t>(initialVectorAddress) != reinterpret_cast<std::uintptr_t>(vectorAddressAfterSettingCapacity))
	{
		std::cout << "std::vector addresses don't match - reallocation occured dammit!" << '\n';
	}
	else
	{
		std::cout << "std::vector addresses match - it's all good!" << '\n';
	}

	std::cout << "\nVector" << '\n';
	std::vector<std::string, TrackingAlignedAllocator<std::string, 64>> vec{ 100 };
	initialVectorAddress = vec.data();
	std::cout << "&vec[0]=" << vec.data() << '\n';
	vec.reserve(100);
	vectorAddressAfterSettingCapacity = vec.data();
	std::cout << "&vec[0]=" << vec.data() << '\n';
	if (reinterpret_cast<std::uintptr_t>(initialVectorAddress) != reinterpret_cast<std::uintptr_t>(vectorAddressAfterSettingCapacity))
	{
		std::cout << "addresses don't match - reallocation occured dammit!" << '\n';
	}
	else
	{
		std::cout << "addresses match - it's all good!" << '\n';
	}

	vec.emplace_back("Hello");
	vec.emplace_back("w/e");
	vec.emplace_back("whatever");
	vec.push_back("there is ist sofi j");
	vec.push_back("wisdom");
	vec.push_back("fear");
	vec.push_back("there's more than meets the eye");

	std::cout << "sizeof(vec)=" << sizeof(vec) << '\n';
	std::cout << "sizeof(vec[0])=" << sizeof(vec[0]) << '\n';
	std::cout << "alignof(decltype(vec))=" << alignof(decltype(vec)) << '\n';
	std::cout << "alignof(decltype(vec[0]))=" << alignof(decltype(vec[0])) << '\n';
	std::cout << "alignof(decltype(vec[2]))=" << alignof(decltype(vec[2])) << '\n';
	std::cout << "isAligned(&vec[0], 64)=" << isAligned(&vec[0], 64) << '\n';
	std::cout << "isAligned(&vec[2], 64)=" << isAligned(&vec[2], 64) << '\n';	// probably won't be aligned to said boundary, but it doesn't matter, as long as the first element is aligned to specified boundary!
	std::cout << '\n' << vec.get_allocator().getAllocations() << '\n';
	std::cout << "max size = " << vec.get_allocator().max_size() << '\n';

	for ( const auto &s : vec )
	{
		std::cout << s << '\n';
	}

	std::cout << "\nDequeue" << '\n';
	std::deque<int, TrackingAlignedAllocator<int, 256>> dq;
	dq.push_back( 23 );
	dq.push_back( 90 );
	dq.push_back( 38794 );
	dq.push_back( 7 );
	dq.push_back( 0 );
	dq.push_back( 2 );
	dq.push_back( 13 );
	dq.push_back( 24323 );
	dq.push_back( 0 );
	dq.push_back( 1234 );
	for ( const auto &i : dq )
	{
		std::cout << i << '\n';
	}

	std::cout << "sizeof(dq)=" << sizeof(dq) << '\n';
	std::cout << "sizeof(dq[0])=" << sizeof(dq[0]) << '\n';
	std::cout << "alignof(decltype(dq))=" << alignof(decltype(dq)) << '\n';
	std::cout << "alignof(decltype(dq[0]))=" << alignof(decltype(dq[0])) << '\n';
	std::cout << "alignof(decltype(dq[2]))=" << alignof(decltype(dq[2])) << '\n';
	std::cout << "isAligned(&dq[0], 256)=" << isAligned(&dq[0], 256) << '\n';
	std::cout << "isAligned(&dq[2], 256)=" << isAligned(&dq[2], 256) << '\n';	// probably won't be aligned to said boundary, but it doesn't matter, as long as the first element is aligned to specified boundary!
	std::cout << '\n' << dq.get_allocator().getAllocations() << '\n';
	std::cout << "max size = " << dq.get_allocator().max_size() << '\n';
	std::cout << (dq.get_allocator() == vec.get_allocator()) << '\n';

	std::cout << typeid(dq.get_allocator()).name() << '\n';

	std::cout << "\nQueue" << '\n';
	std::queue<int, decltype(dq)> q;
	q.push(23);
	q.push(90);
	q.push(38794);
	q.push(7);
	q.push(0);
	q.push(2);
	q.push(13);
	q.push(24323);
	q.push(0);
	q.push(1234);
	// no random element access in queue, however we could use ._Get_container()
	for (int i = 0; i < q.size(); i++)
	{
		std::cout << q._Get_container()[i] << '\n';
	}

	std::cout << "sizeof(q)=" << sizeof(q) << '\n';
	std::cout << "sizeof(q._Get_container()[0])=" << sizeof(q._Get_container()[0]) << '\n';
	std::cout << "alignof(decltype(q))=" << alignof(decltype(q)) << '\n';
	std::cout << "alignof(decltype(q._Get_container()[0]))=" << alignof( decltype( q._Get_container()[0] ) ) << '\n';
	std::cout << "alignof(decltype(q._Get_container()[2]))=" << alignof( decltype( q._Get_container()[2] ) ) << '\n';
	std::cout << "isAligned(&q._Get_container()[0], 256)=" << isAligned( &q._Get_container()[0], 256 ) << '\n';
	std::cout << "isAligned(&q._Get_container()[2], 256)=" << isAligned( &q._Get_container()[2], 256 ) << '\n';	// probably won't be aligned to said boundary, but it doesn't matter, as long as the first element is aligned to specified boundary!
	std::cout << '\n' << q._Get_container().get_allocator().getAllocations() << '\n';
	std::cout << "max size = " << q._Get_container().get_allocator().max_size() << '\n';
	std::cout << ( q._Get_container().get_allocator() == vec.get_allocator() ) << '\n';


	std::cout << "\nList" << '\n';
	std::list<long long int, TrackingAlignedAllocator<long long int, 16>> ls{30};
	std::cout << "ls.push_back 3 values" << '\n';
	ls.emplace_back( 1729 );
	ls.emplace_back( -6978 );
	ls.emplace_back( 3239 );
	ls.emplace_back( 1002 );

	std::cout << "sizeof(ls)=" << sizeof(ls) << '\n';
	std::cout << "sizeof(ls.begin())=" << sizeof(ls.begin()) << '\n';
	std::cout << "alignof(decltype(ls))=" << alignof(decltype(ls)) << '\n';
	std::cout << "alignof(decltype(ls.begin()))=" << alignof(decltype(ls.begin())) << '\n';
	std::cout << "isAligned(&ls.begin(), 16)=" << isAligned( &ls.begin(), 16 ) << '\n';
	std::cout << "isAligned(*(ls.begin()), 16)=" << isAligned( &*( ls.begin() ), 16 ) << '\n';
	std::cout << '\n' << ls.get_allocator().getAllocations() << '\n';
	std::cout << "max size = " << ls.get_allocator().max_size() << '\n';
	std::cout << ( ls.get_allocator() == vec.get_allocator() ) << '\n';
	std::cout << ( ls.get_allocator() == dq.get_allocator() ) << '\n';

	struct Key
	{
		int k;
		bool operator<( const Key& rhs ) const noexcept
		{
			return this->k < rhs.k;
		}
	};
	Key key1{543};
	Key key2{3};
	Key key3{-897034};

	struct Value
	{
		std::string s;
	};
	Value value1{"Hello"};
	Value value2{"Hi"};
	Value value3{"Howdy"};


	std::cout << "\nMap" << '\n';
	//#define _ENFORCE_MATCHING_ALLOCATORS=0
	std::map<Key, Value, std::less<Key>, TrackingAlignedAllocator<std::pair<const Key, Value>, 32>> myMap;
	myMap.emplace( std::pair{key1, value1} );
	myMap.emplace( std::make_pair( key2, value2 ) );
	myMap.emplace( std::make_pair( key3, value3 ) );

	for ( const auto& av : myMap )
	{
		std::cout << av.first.k
			<< ' '
			<< av.second.s
			<< '\n';
	}

	std::cout << "\nsizeof(myMap)=" << sizeof(myMap) << '\n';
	std::cout << "sizeof(Key)=" << sizeof(Key) << '\n';
	std::cout << "sizeof(Value)=" << sizeof(Value) << '\n';
	std::cout << "sizeof(myMap.begin())=" << sizeof(myMap.begin()) << '\n';
	std::cout << "alignof(decltype(myMap))=" << alignof(decltype(myMap)) << '\n';
	std::cout << "alignof(decltype(myMap.begin()))=" << alignof(decltype(myMap.begin())) << '\n';
	std::cout << "isAligned(&myMap.begin()->first, 32)=" << isAligned( &myMap.begin()->first/*.k*/, 32 ) << '\n';
	std::cout << "isAligned(&myMap.begin()->second, 32)=" << isAligned( &myMap.begin()->second, 32 ) << '\n';
	std::cout << '\n' << myMap.get_allocator().getAllocations() << '\n';
	std::cout << "max size = " << myMap.get_allocator().max_size() << '\n';
	std::cout << ( myMap.get_allocator() == vec.get_allocator() ) << '\n';
	std::cout << ( myMap.get_allocator() == dq.get_allocator() ) << '\n';
	std::cout << ( myMap.get_allocator() == ls.get_allocator() ) << '\n';


	std::cout << "Set" << '\n';
	std::set<float, std::less<float>, TrackingAlignedAllocator<float, 2048>> mySet;
	mySet.insert( 42.4287f );
	mySet.insert( .587f );
	mySet.insert( 32.7f );
	mySet.insert( 2.4287f );
	mySet.insert( 44322.124353f );
	mySet.insert( 32432.22f );

	for ( const auto& as : mySet )
	{
		std::cout << as << '\n';
	}

	std::cout << "\nSet" << '\n';
	std::cout << "\nsizeof(mySet)=" << sizeof(mySet) << '\n';
	std::cout << "sizeof(mySet.begin())=" << sizeof(mySet.begin()) << '\n';
	std::cout << "sizeof(*(mySet.begin()))=" << sizeof(*(mySet.begin())) << '\n';
	std::cout << "alignof(decltype(mySet))=" << alignof(decltype(mySet)) << '\n';
	std::cout << "alignof(decltype(mySet.begin()))=" << alignof(decltype(mySet.begin())) << '\n';
	std::cout << '\n' << mySet.get_allocator().getAllocations() << '\n';
	std::cout << "max size = " << mySet.get_allocator().max_size() << '\n';
	std::cout << "isAligned(*(mySet.begin()), 2048)=" << isAligned(*(mySet.begin()), 2048) << '\n';
	std::cout << ( mySet.get_allocator() == vec.get_allocator() ) << '\n';
	std::cout << ( mySet.get_allocator() == dq.get_allocator() ) << '\n';
	std::cout << ( mySet.get_allocator() == ls.get_allocator() ) << '\n';
	std::cout << ( mySet.get_allocator() == myMap.get_allocator() ) << '\n';

	// If we want to align each element of an array there is a workaround.
	// We can use an array of structures and align the structure itself
	std::cout << "\nAlign each element of an array:" << '\n';
	struct AlignedInt
	{
		alignas(64) int aint;
	};
	AlignedInt alignedInts[4];
	//or :
	struct alignas(64) AlignedStructInt
	{
		int aint;
	};
	AlignedStructInt alignedStructInts[4];
	// both produce the same result


	std::cout << "\nIntrinsic type alignment" << '\n';
	using vector128 = std::vector<__m128, TrackingAlignedAllocator<__m128, sizeof(__m128)>>;
	vector128 lhs;
	vector128 rhs;

	__m128 val{143, 534, 5342, 543};
	__m128 val2{143, 43204, 5342, 543};
	lhs.push_back(val);
	lhs.push_back(val);
	rhs.push_back(val2);
	rhs.push_back(val2);

	std::cout << "sizeof(__m128)=" << sizeof(__m128) << '\n';

	std::cout << "alignof(decltype(lhs[1]))=" << alignof(decltype(lhs[1])) << '\n';
	std::cout << "alignof(vector128)=" << alignof(vector128) << '\n';

	std::cout << "isAligned(lhs.data(), sizeof(__m128))=" << isAligned(lhs.data(), sizeof(__m128)) << '\n';
	std::cout << "isAligned(&lhs[1], sizeof(__m128))=" << isAligned(&lhs[1], sizeof(__m128)) << '\n';
	std::cout << "isAligned(rhs.data(), sizeof(__m128))=" << isAligned(rhs.data(), sizeof(__m128)) << '\n';
	std::cout << "isAligned(&rhs[1], sizeof(__m128))=" << isAligned(&rhs[1], sizeof(__m128)) << '\n';

	float a = 1.0f;
	float b = 2.0f;
	float c = 3.0f;
	float d = 4.0f;

	float e = 5.0f;
	float f = 6.0f;
	float g = 7.0f;
	float h = 8.0f;

	for ( std::size_t i = 0; i < 1000; ++i )
	{
		lhs.push_back( _mm_set_ps( a, b, c, d ) );
		rhs.push_back( _mm_set_ps( e, f, g, h ) );

		a += 1.0f; b += 1.0f; c += 1.0f; d += 1.0f;
		e += 1.0f; f += 1.0f; g += 1.0f; h += 1.0f;
	}

	__m128 mul = _mm_mul_ps( lhs[10], rhs[10] );


	std::cout << "Individual allocations" << '\n';

	std::cout << "int fundamental type example" << '\n';

	std::cout << "using placement new alongside the allocator:" << '\n';
	TrackingAlignedAllocator<int, 256> taa;
	void* vi = taa.allocate( sizeof(int) );
	int* pi = new(vi) int{4};
	std::cout << *pi
		<< '\n';
	using INT = int;
	pi ->~INT();				// pseudo destructor call
	taa.deallocate( pi );

	std::cout << "isAligned(vi, 256)=" << isAligned( vi, 256 ) << '\n';
	std::cout << "isAligned(vi, 256)=" << isAligned( vi, 256 ) << '\n';
	std::cout << "isAligned(pi, 256)=" << isAligned( pi, 256 ) << '\n';
	std::cout << "isAligned(pi, 256)=" << isAligned( pi, 256 ) << '\n';

	std::cout << "Using only the allocator" << '\n';
	int* pint = taa.allocate( sizeof(int) );
	taa.construct( pint, 1453 );
	std::cout << *pint << '\n';
	taa.destroy( pint );
	taa.deallocate( pint );
	std::cout << "isAligned(pint, 256)="
		<< isAligned( pint, 256 )
		<< '\n';
	std::cout << "isAligned(pint, 256)="
		<< isAligned( pint, 256 )
		<< '\n';

	std::cout << "\nstd::string example" << '\n';
	TrackingAlignedAllocator<std::string, 16384> saa;

	std::string* ps = saa.allocate( sizeof( std::string ) );
	saa.construct( ps,
		"My name is Maximus Decimus Meridius.\n"
		"Commander of the armies of the North.\n"
		"General of the Phoelix legions.\n"
		"Loyal servant to the true emperor, Marcus Aurelius.\n"
		"Father to a murdered son, husband to a murdered wife\n"
		" and I will have my vengeance, in this life or the m_pNext.\n" );
	std::cout << *ps << '\n';
	std::cout << "isAligned(ps, 16384)="
		<< isAligned( ps, 16384 )
		<< '\n';
	std::cout << "isAligned(ps, 16384)="
		<< isAligned( ps, 16384 )
		<< '\n';
	saa.destroy( ps );
	saa.deallocate( ps );

	std::system( "pause" );
	return 0;
}
