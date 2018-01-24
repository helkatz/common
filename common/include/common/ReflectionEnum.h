#pragma once
/**
REFLECTION_ENUM declares a reflectable enum with the following features
* use toString to get a string literal of the enum
* use fromString to set a enum value from a string
* enum can only be used with full qualified name that allows the declaration of enums
  in the same scope with same enum values

usage simply delcare like an enum
REFLECTION_ENUM(eTest,
	A=1,
	B=3,
	C=5,
	D,
	EValue = 10,
	FValue
)

eTest e1 = eTest::EValue;
cout << e1.toString();
prints out EValue
cout << e1;
prints out 10
*/

#include <string>
#include <sstream>
#include <algorithm>
#include <regex>
#include <map>
#include <unordered_map>


#define REFLECTION_ENUM(NAME, ...)								\
	class NAME : public ReflectionEnum <NAME>					\
	{															\
	public:														\
		using ReflectionEnum::ReflectionEnum;					\
		using ReflectionEnum::operator=;						\
		enum Enum { __VA_ARGS__	};								\
	private:													\
		friend class ReflectionEnum <NAME>;						\
		Enum _e;												\
		static const char *data() { return #__VA_ARGS__; }		\
	};

/**
this is the base class of REFLECTION_ENUM
it cannot be used direct only by the REFLECTION_ENUM macro
*/
template<class Derived>
class ReflectionEnum
{
private:
	static std::string &trim(std::string &s)
	{
		s.erase(std::remove_if(s.begin(), s.end(), isspace), s.end());
		return s;
	}
	template<typename T>
	static const std::vector<std::pair<T, std::string>>& toMap(std::string enumDeclaration)
	{
		static std::vector<std::pair<T, std::string>> resultMap;
		std::regex r("(.+?)((?:=)(.+?))*(,|$)");
		int eVal = -1;
		std::smatch sm;
		while (std::regex_search(enumDeclaration, sm, r)) {
			std::string name = sm.str(1);
			if (sm[3].matched)
				eVal = atoi(trim(sm.str(3)).c_str());
			else
				eVal++;
			resultMap.push_back(std::pair<T, std::string>(static_cast<T>(eVal), name));
			enumDeclaration = sm.suffix().str();
		}
		return resultMap;
	}

	Derived& derived()
	{
		return static_cast<Derived&>(*this);
	}
	const Derived& derived() const
	{
		return dynamic_cast<const Derived&>(*this);
	}

public:
	virtual ~ReflectionEnum()
	{}

	ReflectionEnum()
	{

	}
	template<typename E, typename = std::enable_if_t<std::is_same<Derived::Enum, E>::value>>
	ReflectionEnum(const E& e)
	{
		derived()._e = e;
	}

	void fromString(const std::string& str)
	{
		auto m = toMap<Derived::Enum>(Derived::data());
		auto found = std::find_if(
			m.begin(), m.end(),
			[str](const std::pair<Derived::Enum, std::string>& v) -> bool { return v.second == str; }
		);
		if (found == m.end())
			throw std::exception("not found");
		derived()._e = found->first;
	}

	std::string toString() const
	{
		auto m = toMap<Derived::Enum>(Derived::data());
		auto found = std::find_if(
			m.begin(), m.end(),
			[this](const std::pair<Derived::Enum, std::string>& v) -> bool { return v.first == derived()._e; }
		);
		if (found == m.end())
			throw std::exception("not found");
		return found->second;
	}

	bool operator == (const ReflectionEnum& right) const
	{
		return derived()._e == right._e;
	}

	template<typename E, typename = std::enable_if_t<std::is_same<Derived::Enum, E>::value>>
	bool operator == (const E& right) const
	{
		return derived()._e == right;
	}

	Derived& operator = (const Derived& right)
	{
		derived()._e = right._e;
		return derived();
	}

	template<typename E, typename = std::enable_if_t<std::is_same<Derived::Enum, E>::value>>
	Derived& operator = (const E& right)
	{
		derived()._e = right;
		return derived();
	}
};

