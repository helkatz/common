
#include <gtest/gtest.h>
#include "PropertiesTest.h"
#include <Poco/DateTimeFormatter.h>
#include <iostream>
#include <array>
#include <new>
std::map<std::string, Logger *> Logger::_loggers;
using namespace std;
using namespace common;
#define prperty(...) PROPERTY
#define lcc(...)
class cT {};
class PropertiesTest : public common::Properties < PropertiesTest >, public testing::Test
{
public:
	
	PROPERTY_LCC(cT, T);
	// declare some lower camel case Properties getter and setter are public
	PROPERTY_LCC(int, Type);
	PROPERTY_LCC(std::string, Description);
	PROPERTY_LCC(std::string, StringProp);	
	PROPERTY_LCC(double, DoubleProp);
	PROPERTY_LCC(int, IntProp);
	PROPERTY_LCC(long long, LongLongProp);
	PROPERTY_LCC(int, Id);
	//PROPERTY_LCC(Poco::DateTime, Timestamp);
	//friend std::ostream& operator << (std::ostream& os, const Poco::DateTime& right);
	// declare some lower camel case Properties getter public and setter protected
	PROPERTY_LCC_PUBL_PROT(std::string, StringProp_SetProtected, "Test");
	struct TInner : Properties < TInner >
	{
		PROPERTY_LCC(int, IntProp);
		PROPERTY_LCC(int, IntProp2, 10);
	};
	TInner inner;
	PROPERTY_LCC(TInner, Inner);
	PROPERTY_LCC(std::vector<TInner>, InnerList, Behavior().fireEvents(Events::none));
	friend std::ostream& operator << (std::ostream& os, const std::vector<TInner>& right)
	{
		for (auto it = right.cbegin(); it != right.cend(); ++it) {
			os << *it << endl;// << "    " << it->dump();
		}
		return os;
	}
};

std::ostream& operator << (std::ostream& os, const cT& right);
bool operator == (const cT& left, const cT& right);
bool operator < (const cT& left, const cT& right);
std::ostream& operator << (std::ostream& os, const Poco::DateTime& v)
{
	os << Poco::DateTimeFormatter::format(v, "%d-%m-%Y %H:%M:%S");
	return os;
}
DTEST_F(PropertiesTest, dump)
{
	setDescription("this is a dump");

	TInner inner;
	inner.setIntProp(1);
	inner.setIntProp2(2);
	InnerList().get().push_back(inner);
	inner.setIntProp2(6);
	InnerList().get().push_back(inner);
	cout << InnerList();
	cout << *this;
}
