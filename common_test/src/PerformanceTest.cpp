
#include <gtest/gtest.h>
#include "PropertiesTest.h"
#include <iostream>
#include <array>
#include <new>


struct PerformanceTestStruct : public common::Properties < PerformanceTestStruct >
{
	PROPERTY_LCC(int, IntProp);
	PROPERTY_LCC(std::string, StringProp);

	int nativeInt;
	int getNativeInt()
	{
		return nativeInt;
	}
};

int getNativeInt()
{
	return 1;
}

TEST(PerformanceTest, PerformanceTest)
{
	PerformanceTestStruct ts;
	int tries = 10000000;
	ts.setIntProp(1);
	cpDebug("getIntProp begin");
	for (int i = 0; i < tries; i++) {
		ts.getIntProp();// .IntProp().get();
	}
	cpDebug("getIntProp done");

	cpDebug("getNativeInt begin");
	for (int i = 0; i < tries; i++) {
		ts.getNativeInt();
	}
	cpDebug("getNativeInt done");

	cpDebug("Map begin");
	common::Map<const char *, int> map1;
	const char *mapIntKey = "mapIntKey";
	map1.set(mapIntKey, 1);
	for (int i = 0; i < tries; i++) {
		map1.get(mapIntKey);
	}
	cpDebug("Map done");


	cpDebug("std::map begin");
	std::map<const char *, int> map2;
	map2[mapIntKey] = 1;
	
	for (int i = 0; i < tries; i++) {
		map2.at(mapIntKey);
	}
	cpDebug("std::map done");

}
