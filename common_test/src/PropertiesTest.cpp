
#include <gtest/gtest.h>
#include "PropertiesTest.h"
#include <iostream>
#include <array>
#include <new>

using namespace std;
using namespace common;


int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	::testing::GTEST_FLAG(filter) = 
		"*FileTest*"
		":xxCronTrigger*"
		":xxPerformanceTest*"
		":PropertiesDataTest*";
	//::testing::GTEST_FLAG(filter) = "CronTrigger*";
	return RUN_ALL_TESTS();
}

class PropertiesTest : public common::Properties < PropertiesTest >, public testing::Test
{
public:
	PropertiesTest()
	{
		//_behavior->fireEvents();
	}

	virtual const Behavior& getBehavior()
	{
		return _behavior->fireEvents(Events::none);
	}
	// declare some lower camel case Properties getter and setter are public
	PROPERTY_LCC(int, Type);
	PROPERTY_LCC(std::string, Description);
	PROPERTY_LCC(std::string, StringProp);	
	PROPERTY_LCC(double, DoubleProp);
	PROPERTY_LCC(int, IntProp);
	PROPERTY_LCC(long long, LongLongProp);
	PROPERTY_LCC(int, Id);

	// declare some lower camel case Properties getter public and setter protected
	PROPERTY_LCC_PUBL_PROT(std::string, StringProp_SetProtected, "Test");
	struct TInner : Properties < TInner >
	{
		PROPERTY_LCC(int, IntProp);
		PROPERTY_LCC(int, IntProp2, 10);
	};

	PROPERTY_LCC(std::vector<TInner>, InnerList, Behavior().fireEvents(Events::none));
	friend std::ostream& operator << (std::ostream& os, const std::vector<TInner>& right)
	{
		for (auto it = right.cbegin(); it != right.cend(); ++it) {
			os << *it << endl;
		}
		return os;
	}
};


struct Simple
{
	int i;
	Simple()
	{
		i = 10;
		cpDebug("Simple()");
	}
	Simple(const char *s)
	{
		cpDebug("Simple(%1)", s);
	}
	virtual ~Simple()
	{
		cpDebug("~Simple");
	}
	bool operator == (const Simple& r) const
	{
		return i == r.i;
	}
	
	bool operator < (const Simple& r) const
	{
		return i < r.i;
	}

	friend std::ostream& operator << (std::ostream& os, const Simple& v)
	{
		os << v.i;
		return os;
	}
};
struct TestSimple : public common::Properties < TestSimple >
{
	PROPERTY_LCC(Simple, simpleProp1);
	PROPERTY_LCC(Simple, simpleProp2);
	
	
};

struct User1: public Properties<User>
{
	//property(int, active);
};

TEST_F(PropertiesTest, assign)
{
	
//	User1 user;
//	user.active().set(1);
	
//	ASSERT_EQ(1, user.active());

	ASSERT_TRUE(is_class<vector<int>>::value);
	TestSimple simple;
	Simple s1, s2;

	simple
		.setsimpleProp1("string1")
		.setsimpleProp2("string2");
	TestStruct p;
	p.setDescription("Test");
	ASSERT_EQ("Test", p.Description().toString());
	p.setIntProp(10);
	ASSERT_EQ(10, p.IntProp());

	TInner inner2;
	TInner inner;
	inner.setIntProp(1);
	inner.setIntProp2(2);
	inner2 = inner;

	//cout << inner2.dump();

	setIntProp(5);
	ASSERT_TRUE(InnerList().valid());
	InnerList().get().push_back(inner);
	ASSERT_EQ(1, InnerList().get().size());
	ASSERT_EQ(1, getInnerList().size());
	ASSERT_EQ(1, getInnerList().at(0).IntProp());
	//cout << p.dump();
	//cout << dump();
}


TEST_F(PropertiesTest, assign_protected)
{
	TestStruct p;
	// members that causes an compile error because the prop is protected for modification
	// p.setStringProp_SetProtected("Test");		// error inaccesible
	// p.StringProp_SetProtected().set("Test");	// error inaccesible
	// p.StringProp_SetProtected().clearModified();// error inaccesible

	ASSERT_TRUE(p.StringProp_SetProtected().valid());
	ASSERT_FALSE(p.StringProp_SetProtected().modified());
	ASSERT_EQ("Test", p.StringProp_SetProtected().toString());
}

TEST_F(PropertiesTest, valid)
{
	TestStruct p;
	std::string desc = "Test";
	ASSERT_TRUE(p.Description().valid());
	p.setDescription(desc);
}

TEST_F(PropertiesTest, modified)
{
	TestStruct p;
	p.setDescription("Test");
	ASSERT_FALSE(p.Description().modified());
	p.setDescription("ModifiedTest");
	ASSERT_TRUE(p.Description().modified());
}

TEST_F(PropertiesTest, clearModified)
{
	TestStruct p;
	p.setDescription("Test");
	p.setDescription("ModifiedTest");
	ASSERT_TRUE(p.Description().modified());
	p.Description().clearModified();
	ASSERT_FALSE(p.Description().modified());
}
TEST_F(PropertiesTest, convert)
{
	TestStruct p;
	p.setStringProp("10.00");
	ASSERT_EQ(10, p.StringProp().toInt());
	ASSERT_EQ(string("10.00"), p.StringProp().toString());
	ASSERT_EQ("10.00", p.StringProp().toString());

	p.setIntProp(10);
	ASSERT_EQ(10, p.IntProp().toInt());
	ASSERT_EQ(string("10"), p.IntProp().toString());
	ASSERT_EQ(10.00, p.IntProp().toDouble());

	p.setDoubleProp(10.333);
	ASSERT_EQ(10, p.DoubleProp().toInt());
	ASSERT_EQ(string("10.333"), p.DoubleProp().toString());
	ASSERT_EQ(10.333, p.DoubleProp().toDouble());
}