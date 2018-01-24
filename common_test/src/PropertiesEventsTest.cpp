#include "PropertiesTest.h"
using namespace common;

class PropertiesEventsTest : public common::Properties<PropertiesEventsTest>, public testing::Test
{
protected:
	PROPERTY_LCC(std::string, TestOnNotAssigned);
	PROPERTY_LCC(std::string, TestAssignedByEventHandler);
	PROPERTY_LCC(std::string, TestChangedByGetEventHandler);
	PROPERTY_LCC(std::string, TestChangedBySetEventHandler);

	// this overloaded function will called when a property is writing
	void onPropertySet(const char *name)
	{
		if (std::string("TestChangedBySetEventHandler") == name) {
			setTestChangedBySetEventHandler("changedBySetEventHandler");
		}
	}

	// this overloaded function will called when a property is readed
	void onPropertyGet(const char *name)
	{
		if (std::string("TestChangedByGetEventHandler") == name) {
			setTestChangedByGetEventHandler("changedByGetEventHandler");
		}
	}

	// this overloaded function handles not assigend properties
	void onPropertyNotAssigned(const char *name)
	{
		cpDebug("onPropertyNotFound %1", name);
		if (std::string("TestAssignedByEventHandler") == name) {
			setTestAssignedByEventHandler("assignedByEventHandler");
		}
	}
};

TEST_F(PropertiesEventsTest, onPropertyNotAssigned)
{
	// the following will not handled in the onPropertyNotAssigned event handler
	// so it throws an exception
	ASSERT_THROW(TestOnNotAssigned().get(), PropertyNotAssignedException);
	ASSERT_THROW(getTestOnNotAssigned(), PropertyNotAssignedException);

	// the follwing will be assigend by the onPropertyNotAssigned event handler 
	ASSERT_EQ("assignedByEventHandler", TestAssignedByEventHandler().get());
	ASSERT_EQ("assignedByEventHandler", getTestAssignedByEventHandler());

	ASSERT_EQ("changedByGetEventHandler", getTestChangedByGetEventHandler());

	setTestChangedBySetEventHandler("will be overritten by onSet handler");
	ASSERT_EQ("changedBySetEventHandler", getTestChangedBySetEventHandler());
}
