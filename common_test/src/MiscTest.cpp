
#include "PropertiesTest.h"
#include "Common/ScopedEnum.h"
using namespace common;

#include <initializer_list>
template<class T> void f(std::initializer_list<T>);
TEST(MiscTest, T)
{
	//f({ 1 });
}

template<typename T>
class mytype
{
	T _value;
	bool _inititalized;
public:
	void putprop(const T& value)
	{
		_value = value;
	}
	T& getprop()
	{
		return _value;
	}
	__declspec(property(get = getprop, put = putprop)) T value;
};

void func1(int& i)
{
	i = 5;
}

TEST(MiscTest, propertyTest)
{

}
#if 0
class Base
{
public:
	virtual void foo() = 0;
};
template <class T, bool = std::is_base_of<common::PropertiesBase, T>::value>
class B : public Base
{
public:
	void foo() override { std::cout << "A is not base of T!" << std::endl; }
};

template <class T>
class B<T, true> : public Base{
public:
	void foo() override { std::cout << "A is base of T!" << std::endl; }
};

class C : public Base
{

};

class D
{
};

TEST(MiscTest, Test2)
{
	B<C> b;
	B<D> b1;
}

#endif