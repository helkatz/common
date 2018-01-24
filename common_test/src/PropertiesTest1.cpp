#include "PropertiesTest.h"
using namespace common;

TEST(PropertiesTest1, assignHardCoded)
{
	User u;
	u
		.setFirstName("Helmut")
		.setLastName("Katz");
	ASSERT_EQ("Helmut", u.getFirstName());
	ASSERT_EQ("Katz", u.getLastName());
	ASSERT_EQ(1, u.getId());

	User u2 = u;
	ASSERT_EQ("Helmut", u2.getFirstName());
	ASSERT_EQ("Katz", u2.getLastName());
	ASSERT_EQ(1, u2.getId());
}

TEST(PropertiesTest1, get)
{
	User u;
	// throws exception because FirstName is not set yet
	ASSERT_THROW(u.getFirstName(), PropertyNotAssignedException);
	ASSERT_THROW(u.FirstName().get(), PropertyNotAssignedException);
	// firstname is initialized as a valid variable but not setted yet 
	// so it will take the default parameter declared on property declaration
	ASSERT_EQ("defaultName", u.getFirstName("defaultName"));
	// here the default parameter has no more effect it will not be modified
	ASSERT_EQ("defaultName", u.getFirstName("defaultName1"));


	// throws an exception but its a POD type with not default initializition
	ASSERT_THROW(u.getState(), PropertyNotAssignedException);

	// if u use a default value to get a prop those default is then setted to the prop
	// same when u call setState(New)
	ASSERT_NO_THROW(u.getState(UserState::New));

	// now the default value is setted to this prop
	ASSERT_EQ(UserState::New, u.getState());


	// language has an default value declared on its declaration
	ASSERT_EQ(UserLanguage::German, u.getLanguage());

	// here u can access Friends without any sets before because thats the setted behavior 
	// see declaration of that property
	ASSERT_EQ(0, u.getFriends().size());

}

/**
	in this test u will see different ways to set properties
*/
TEST(PropertiesTest1, set)
{
	User u;
	
	u.setState(UserState::New);
	u.State().set(UserState::New);
	ASSERT_EQ(UserState::New, u.getState());

	// get the reference and work with them 
	// in this case u will loose the event behavior and some states will also not work correctly
	// but if u need this reference e.q for performance reasons, thats the way
	UserState& ustate = u.State().get();
	ustate = UserState::Active;
	ASSERT_EQ(UserState::Active, u.getState());

	User uFriend;
	uFriend.setFirstName("myfriend");
	u.Friends().get().push_back(uFriend);
	ASSERT_EQ(1, u.getFriends().size());
}

TEST(PropertiesTest1, accessLayer)
{
	User u;
	// id is declared as PROPERTY_LCC_PUBL_PROT that means read are public and write are protected


	// protected properties not modifieable
	// uncomment CHECK_ACCESSLAYER and u should get innaccessable compile errors 
//#define CHECK_ACCESSLAYER
#ifdef CHECK_ACCESSLAYER
	int v1 =	u.Id().get();	// thats allowed
	int& v2 =	u.Id().get();	// compile error 
	u.Id().set(1);				// compile error
	u.Id().clearModified();		// compile error
	int& i = u.Id().getRef();	// compile error
	int& v = u.Id().get();		// compile error
#endif	
}

TEST(PropertiesTest1, modified)
{
	User u;
	ASSERT_FALSE(u.State().modified());

	// first set to a property will not set the modified flag
	u.setState(UserState::New);
	ASSERT_FALSE(u.State().modified());

	u.setState(UserState::Active);
	ASSERT_TRUE(u.State().modified());

	// set a property without changes does not effect the modified flag
	u.State().clearModified();
	u.setState(UserState::Active);
	ASSERT_FALSE(u.State().modified());
}

TEST(PropertiesTest1, compare)
{
	TestStruct ts1;
	TestStruct ts2;
	ASSERT_TRUE(ts1 == ts2);
	ts1.setDescription("Alpha");
	ASSERT_FALSE(ts1 == ts2);
	ASSERT_TRUE(ts1 != ts2);

	ts2.setDescription("Alpha");
	ASSERT_TRUE(ts1 == ts2);
	ASSERT_FALSE(ts1 != ts2);

	ts2.setDoubleProp(3.8);
	ASSERT_FALSE(ts1 == ts2);

	ts1 = ts2;
	ASSERT_TRUE(ts1 == ts2);
}

