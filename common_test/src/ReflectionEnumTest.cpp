
#include "Common/ReflectionEnum.h"
#include <gtest/gtest.h>

#include <vector>
#include <boost/algorithm/string.hpp>
#include <Poco/Timespan.h>
#include <Poco/DateTime.h>
class CronTrigger
{
public:
	CronTrigger()
	{

	}
	int parse(const std::string& pattern)
	{
		std::string line(pattern);
		std::vector<std::string> strs;
		boost::split(strs, line, boost::is_any_of(" "));
		Poco::Timespan timespan;
		Poco::DateTime now;
		std::cout << "* size of the vector: " << strs.size() << std::endl;

		for (int i = 0; i < strs.size(); i++) {
			std::string& s = strs[i];
			boost::trim(s);
			int value = 0;
			if (s != "*")
				value = atoi(s.c_str());
			switch (i) {
			case 0:
				timespan += Poco::Timespan(0, 0, 0, value, 0);
				break;
			case 1:
				timespan += Poco::Timespan(0, 0, value, 0, 0);
				break;
			case 2:
				timespan += Poco::Timespan(0, value, 0, 0, 0);
				break;
			case 3:
				timespan += Poco::Timespan(value, 0, 0, 0, 0);
				break;
			}
			
			std::cout << s << std::endl;
		}
		
		return (now + timespan).timestamp().epochTime() - now.timestamp().epochTime();
	}
};

TEST(CronTrigger, all)
{
	std::cout << "start in " << CronTrigger().parse("* * *") << std::endl;
	std::cout << "start in " << CronTrigger().parse("* 1 *") << std::endl;
	std::cout << "start in " << CronTrigger().parse("* * 1") << std::endl;
	std::cout << "start in " << CronTrigger().parse("* 0 12 ") << std::endl;
	std::cout << "start in " << CronTrigger().parse("* * 0 2") << std::endl;
	std::cout << "start in " << CronTrigger().parse("* * 1") << std::endl;
	std::cout << "start in " << CronTrigger().parse("* * 1") << std::endl;
	ASSERT_TRUE(false);
}
REFLECTION_ENUM(EnumType1,
	A, B, C=15, D = 20
);
REFLECTION_ENUM(EnumType2,
	A, B
);

template<typename T>
void serialize(const ReflectionEnum<T>& e)
{
	std::cout << e.toString();
}
#if 1
struct E {
enum Enum {
	A, B = 3, C = 2, D, E1, F = 10, G
};
};
struct Enum1 {
	constexpr static int A = 1;
	constexpr static int B = 2;
};
#define HARG(DECL) std::cout << #DECL << std::endl;

#define COUNT_ARGS(...) COUNT_ARGS_(__VA_ARGS__,6,5,4,3,2,1,0)
#define COUNT_ARGS_(z,a,b,c,d,e,f,cnt,...) \
	std::cout << "z="<<#z<<",a="<<#a<<",b="<<#b<<",c="<<#c<<",d="<<#d<<",e="<<#e<<",f="<<#f<<",cnt="<<#cnt<<std::endl;
#define PP_NARG(...) \
         PP_NARG_(__VA_ARGS__,PP_RSEQ_N())
#define PP_NARG_(...) \
         PP_ARG_N(__VA_ARGS__)
#define PP_ARG_N( \
          _1, _2, _3, _4, _5, _6, _7, _8, _9,_10, \
         _11,_12,_13,_14,_15,_16,_17,_18,_19,_20, \
         _21,_22,_23,_24,_25,_26,_27,_28,_29,_30, \
         _31,_32,_33,_34,_35,_36,_37,_38,_39,_40, \
         _41,_42,_43,_44,_45,_46,_47,_48,_49,_50, \
         _51,_52,_53,_54,_55,_56,_57,_58,_59,_60, \
         _61,_62,_63,N,...) N
#define PP_RSEQ_N() \
         63,62,61,60,                   \
         59,58,57,56,55,54,53,52,51,50, \
         49,48,47,46,45,44,43,42,41,40, \
         39,38,37,36,35,34,33,32,31,30, \
         29,28,27,26,25,24,23,22,21,20, \
         19,18,17,16,15,14,13,12,11,10, \
         9,8,7,6,5,4,3,2,1,0

#define M(...) PP_NARG(__VA_ARGS__)
#define HARG1(DECL, ...) COUNT_ARGS(__VA_ARGS__)
#define M4(DECL, ...) 
#define M3(DECL, ...) HARG1(DECL, __VA_ARGS__)
#define M2(DECL, ...) HARG1(DECL, __VA_ARGS__)
#define M1(DECL, ...) M(DECL, __VA_ARGS__)
#define P(DECL, ...) M(DECL,__VA_ARGS__)
#define FOO1(A)
#define FOO2(_1)
#define FOO3(_1,_2,_3)
#define FOO4(_1,_2,_3,_4)
#define GET_MACRO(_1,_2,_3,_4,NAME,...) NAME
#define FOO(...) GET_MACRO(__VA_ARGS__, FOO4, FOO3, FOO2)
TEST(ReflectionEnum, all)
{
	FOO(a, B, C, D);
	//std::cout << M(C);
	//std::cout << PP_NARG(A, 10);
	PP_NARG(A,B);
	PP_NARG(A  10, B = 20, C, D = A*B);

	E::Enum;
	std::cout << E::Enum::A << std::endl;
	std::cout << E::Enum::B << std::endl;
	std::cout << E::Enum::C << std::endl;
	std::cout << E::Enum::D << std::endl;
	std::cout << E::Enum::E1 << std::endl;
	std::cout << E::Enum::F << std::endl;
	std::cout << E::Enum::G << std::endl;

	EnumType1 e1 = EnumType1::A;
	EnumType1 e2 = EnumType1::B;
	EnumType2 e3;

	e1.fromString("A");
	ASSERT_EQ(e1, EnumType1::A);
	e1.fromString("B");
	ASSERT_EQ(e1, EnumType1::B);
	ASSERT_THROW(e1.fromString("C"), std::exception);

	e1 = e2;
	ASSERT_TRUE(e1 == EnumType1::B);
	e1 = EnumType1::A;

	ASSERT_EQ("A", e1.toString());
	serialize(e1);
	serialize(e2);

}
#endif

TEST(ReflectionEnum, declare_in_class)
{
	struct TestClass
	{
		REFLECTION_ENUM(EnumType1,
			A = 1,
			B = 10,
			C
		);
	};
	EnumType1 e1;
	TestClass::EnumType1 e2;
}

