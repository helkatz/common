#pragma once
#include <gtest/gtest.h>
#include "common/Properties.h"
#include <Poco/DateTime.h>
#define DTEST(NAMEA, NAMEB) class NAMEA##NAMEB { void disabled(); }; void NAMEA##NAMEB::disabled()
#define DTEST_F(NAMEA, NAMEB) class NAMEA##NAMEB: public NAMEA { void disabled(); }; void NAMEA##NAMEB::disabled()

/**
here are some classes to show the usage and benefit of the properties
*/
class Address : public common::Properties < Address >
{
	PROPERTY_LCC(std::string, Street, "none");
	PROPERTY_LCC(std::string, Country);
};

typedef Address UserAddress;
enum UserLanguage { German, English };
enum UserState { New, Active };
using namespace common;
class User : public common::Properties < User >
{
	// declare some lower camel case Properties
	PROPERTY_LCC_PUBL_PROT(int, Id);				// declares Id with protected setter
	PROPERTY_LCC(std::string, FirstName);
	PROPERTY_LCC(std::string, LastName);

	PROPERTY_LCC(UserState, State);					// declare without default value
	PROPERTY_LCC(UserLanguage, Language, UserLanguage::German);	// declare with default value

	// declare from an class and set event behavior to avoid PropertyNotSet event
	// when u access with getAddress()....
	// this is necessary because per default each non setted property anyway its POD or Class
	// throws an PropertyNotSetException for some circumstances
	PROPERTY_LCC(UserAddress, Address);// , common::Behavior().fireEvents(common::Events::get | common::Events::set));
	PROPERTY_LCC(std::vector<User>, Friends, Behavior().fireEvents(Events::get | Events::set));


public:
	User()
	{
		setId(1); // its protected 
		//auto address = getAddress().Street();
	}

};

inline std::ostream& operator << (std::ostream& os, const std::vector<User>& v)
{
	return os;
}

struct TestStruct : public common::Properties < TestStruct >
{
	// declare some lower camel case Properties getter and setter are public
	PROPERTY_LCC(int, Type);
	PROPERTY_LCC(std::string, Description);
	PROPERTY_LCC(std::string, StringProp);
	PROPERTY_LCC(double, DoubleProp);
	PROPERTY_LCC(int, IntProp);
	PROPERTY_LCC(long long, LongLongProp);
	PROPERTY_LCC(int, Id);
	PROPERTY_LCC_PUBL_PROT(std::string, Id1);
	// declare some lower camel case Properties getter public and setter protected
	PROPERTY_LCC_PUBL_PROT(std::string, StringProp_SetProtected, "Test");
};
