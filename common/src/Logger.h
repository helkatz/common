#pragma once
#include "stringformat.h"
#include <string>
#include <map>
#include <iostream>
#include <Commctrl.h>
#include <Windows.h>
#include <time.h>
class Logger
{
	static std::map<std::string, Logger *> _loggers;

	Logger(const std::string& name)
	{

	}
public:
	static Logger *get(const std::string& name)
	{
		auto it = _loggers.find(name);
		if (it != _loggers.end())
			return it->second;
		Logger *l = new Logger(name);
		_loggers[name] = l;
		return l;

	}

	static Logger *getLogger(type_info *ti)
	{
#if 1
		const int maxCache = 10;
		static type_info *last_ti[maxCache] = { NULL };
		static Logger *last_logger[maxCache] = { NULL };
		for (int i = 0; i < maxCache; i++) {
			if (last_ti[i] == ti)
				return last_logger[i];
			if (last_ti[i] == NULL) {
				last_ti[i] = ti;
				std::string name = ti->name();
				// find space after class or struct the classname should start here
				size_t beginName = name.find(' ');
				// eliminate all spaces
				beginName = name.find_first_not_of(' ', beginName);
				// find the end of the class name
				size_t endName = name.find(' ', beginName + 1);
				name = name.substr(beginName, endName - beginName);
				last_logger[i] = Logger::get(name);
				return NULL;
			}
		}
		for (int i = 0; i < maxCache; i++)
			last_ti[i] = NULL;
		last_ti[0] = ti;
#else
		static type_info *last_ti = ti;
		if (last_ti != ti) {
			last_ti = ti;
			return Logger::get(ti->name());
		}
#endif
		return NULL;
	}
	const char *getLoggerName(const char *name)
	{
	}

	static Logger *getLogger(const char *name)
	{
		static char *last_name = NULL;
		static Logger *last_logger = NULL;
		// compare first only the address
		if (last_name != name) {
			//std::string className = regex(name, "class ([^ ]+)");

			last_name = (char *)name;
			std::string loggerName = name;
			last_logger = get(loggerName);
		}
		return last_logger;
		//cout << typeid(ti).name() << endl;

		return NULL;
	}
	Logger& debug(const std::string& message)
	{
		static clock_t Start = clock();
		std::cout << clock() - Start << ": " << message << std::endl;
		return *this;
	}
	friend Logger& operator << (Logger& l, const std::string& s)
	{
		return l;
	}

};
#define cpDebug(MSG, ...) Logger::getLogger((type_info *)&typeid(this))->debug(FMT(MSG, __VA_ARGS__))
#define cpDebug_(MSG, ...) Logger::getLogger(__FILE__)->debug(FMT(MSG, __VA_ARGS__))