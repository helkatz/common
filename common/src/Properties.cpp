#include <iostream>
#include <common/Properties.h>
using namespace std;
#define ASSERT_TRUE(CONDITION) {cout << #CONDITION << " " << (CONDITION ? "TRUE" : "FALSE") << endl;}
#define ASSERT_FALSE(CONDITION) ASSERT_TRUE(!(CONDITION))

static const char *prop2Name = "Prop2";
//template<typename T> class B;

#if 0
class A
{
	
	
	int _priv;
public:

};


class B //: public A
{
	A a;
protected:
	Behavior behavior;
public:
	B()
	{
		std::cout << behavior._fireEvents;
	}
};
template<typename T>
class C : public B
{
public:
	C()
	{
		std::cout << behavior._fireEvents;
	}
};
C<int> b;
#endif
#if 0
struct PropertiesTest
{
	PropertiesTest();
};
template<typename Type>
class PropertyPublic : public Property<Type>
{
public:
	const Type& operator = (const Type& value)
	{
		return Property<Type>::operator = (value);
	}
	void clearModified()
	{
		return Property<Type>::clearModified();
	}
	void set(const Type& value)
	{
		return Property<Type>::set(value);
	}
};

template<typename Type, typename Friend>
class PropertyProtected : public Property<Type>
{
friend Friend;
protected:
	const Type& operator = (const Type& value)
	{
		return Property<Type>::operator = (value);
	}
	void clearModified()
	{
		return Property<Type>::clearModified();
	}
	void set(const Type& value)
	{
		return Property<Type>::set(value);
	}
public:
	operator  Type&() 
	{
		return Property<Type>::operator const Type&();
	}/*
	operator Type&()
	{
		return Property<Type>::operator Type&();
	}*/
};


PropertiesTest propertiesTest;

PropertiesTest::PropertiesTest()
{


	
	PropertiesObject o;
	o.stringProp();
	ASSERT_TRUE(o.stringProp().valid());
	o.stringProp().get() == "";
	o.GetProp2() == 3;
	string s = o.stringProp();
	o.SetProp2(5);
	ASSERT_TRUE(o.GetProp2() == 5);
	ASSERT_TRUE(o.Prop2() == 5);
	ASSERT_TRUE(o.Prop2().modified());
	//o.Prop2().clearModified();
	//o.Prop2() = 3;
	//o.Prop2().set(5);
	int i = o.Prop2();
	int ci = 0;// o.Prop2().getRef();
	ci = 3;
	ASSERT_TRUE(o.Prop2() == 3);
	ASSERT_TRUE(o.Prop2().modified());
	//o.Prop2().clearModified();
	ASSERT_TRUE(o.Prop2().modified());
	//o.Prop2().set(5);
	ASSERT_TRUE(o.GetProp2() == 5);
	ci = 6;
	ASSERT_TRUE(o.Prop2() == 5);
	ASSERT_TRUE(o.Prop2().modified());
	//o.Prop2() = 3;

	PropertyPublic<int> intProp;
	Property<string> stringProp;
	intProp = 3;

	ASSERT_TRUE(intProp.modified() == false)
	ASSERT_TRUE(intProp == 3)
	ASSERT_FALSE(intProp != 3)
	ASSERT_TRUE(intProp.modified())
	cout << "proptest";

}
#endif