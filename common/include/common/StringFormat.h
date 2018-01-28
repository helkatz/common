#pragma once
#include <boost/format.hpp>
namespace common
{
	inline std::string fmt_(boost::format& fmt)
	{
		return fmt.str();
	}

	template<typename T, typename... T2>
	std::string fmt_(boost::format& fmt, const T& t, const T2& ...t2)
	{
		return fmt_(fmt % t, t2...);
	}

	template<typename T, typename... T2>
	std::string sfmt(const std::string& fmtStr, const T& t, const T2& ...t2)
	{
		boost::format fmt{ fmtStr };
		return fmt_(fmt % t, t2...);
	}

	inline std::string sfmt(const std::string& fmtStr)
	{
		return fmtStr;
	}
}