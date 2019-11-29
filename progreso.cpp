#include <iostream>
#include <vector>
#include <array>

using namespace std;


template<typename T1, class T2>
auto woo(T1 t1, T2 t2) -> decltype(t1+t2)
{
	return t1 + t2;
}

struct S
{
	int i;
};

template<typename T>
auto operator+(T t, S s) -> decltype(t + s)
{
	return t + s.i;
}

struct F
{
	float f;
	double operator+(S s) { return f + s; }
};


int main()
{
	S s{42};
	F f{3.14};
	
	auto d = f + s;
	
	cout << d;
	
	
	return 0;
}
