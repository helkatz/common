#pragma once
#include <vector>
#include <sstream>

/**
Typesafe Helper Makro to format strings
@example:
std::string s = FMT("this %1 is %2 formatted d=%3 i=%4 f=%f", "string", string("typesafe"), 3.33, 1000, 5.3231);
*/
#define FMT(FMT, ...) (StringFormat(FMT),__VA_ARGS__)

/**
This is a typesafe String format class it has no argument limit and works simly
with %1.%N placeholders formating will done in the argument but ints not implemented yet
Use the above described makro to
*/
class StringFormat
{
	std::vector<std::string> _params;
	std::string _fmt;
	std::string _ret;
	void replaceStringInPlace(std::string& subject, const std::string& search,
		const std::string& replace) {
		size_t pos = 0;
		while ((pos = subject.find(search, pos)) != std::string::npos) {
			subject.replace(pos, search.length(), replace);
			pos += replace.length();
		}
	}

	std::string to_string(int i)
	{
		std::stringstream ss;
		std::string s;
		ss << i;
		s = ss.str();
		return s;
	}

	const std::string& buildString()
	{
		if (_params.empty())
			return _fmt;
		int argCount = 1;
		_ret = _fmt;
		for (auto param = _params.begin(); param != _params.end(); ++param) {
			std::string arg = "%" + to_string(argCount++);
			replaceStringInPlace(_ret, arg, *param);
		}
		return _ret;
	}
public:
	StringFormat(const std::string& fmt)
	{
		_fmt = fmt;
	}

	operator const std::string& ()
	{
		return buildString();
	}
	#define ASSIGN_AND_RET(PARAM) \
		std::ostringstream stringStream;\
		stringStream << v;\
		os._params.push_back(stringStream.str());\
		return os;
	friend StringFormat& operator , (StringFormat& os, const __int8& v) { ASSIGN_AND_RET(v); }
	friend StringFormat& operator , (StringFormat& os, const __int16& v) { ASSIGN_AND_RET(v); }
	friend StringFormat& operator , (StringFormat& os, const __int32& v) { ASSIGN_AND_RET(v); }
	friend StringFormat& operator , (StringFormat& os, const __int64& v) { ASSIGN_AND_RET(v); }
	friend StringFormat& operator , (StringFormat& os, const long& v) { ASSIGN_AND_RET(v); }
	friend StringFormat& operator , (StringFormat& os, const unsigned long& v) { ASSIGN_AND_RET(v); }
	friend StringFormat& operator , (StringFormat& os, const bool& v) { ASSIGN_AND_RET(v); }
	friend StringFormat& operator , (StringFormat& os, const double& v) { ASSIGN_AND_RET(v); }
	friend StringFormat& operator , (StringFormat& os, const float& v) { ASSIGN_AND_RET(v); }
	friend StringFormat& operator , (StringFormat& os, const wchar_t& v) { ASSIGN_AND_RET(v); }
	friend StringFormat& operator , (StringFormat& os, const std::string& v) { ASSIGN_AND_RET(v); }
	friend StringFormat& operator , (StringFormat& os, const char* v) { ASSIGN_AND_RET(v); }
	friend StringFormat& operator , (StringFormat& os, const wchar_t* v) { ASSIGN_AND_RET(v); }
	typedef std::string(*TFormatter)(double, int);
};