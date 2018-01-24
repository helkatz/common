#pragma once
#include <boost/optional.hpp>
#define xDECLARE_BEHAVIOR(TYPE, NAME, DEF) \
	private: TYPE _##NAME = DEF; bool _##NAME##Modified = false; \
	public: Behavior& NAME(const TYPE& v) { _##NAME = v; _##NAME##Modified = true; return *this; } \
	public: const bool NAME##Modified() const { return _##NAME##Modified; } \
	public: const TYPE& NAME() const { return _##NAME##Modified ? _##NAME : DEF; }
#define DECLARE_BEHAVIOR(TYPE, NAME, DEF) \
	private: boost::optional<TYPE> _##NAME; \
	public: Behavior& ##NAME(const TYPE& v) { _##NAME = v; return *this; } \
	protected: const bool is_##NAME##_modified() const { return _##NAME.is_initialized(); } \
	protected: const TYPE& ##NAME() const { return _##NAME ? _##NAME.get() : DEF; } 
namespace common {
struct Events
{
	static const int
		none = 0x00,
		get = 0x1,
		set = 0x2,
		not_assigned = 0x4,
		all = 0xff;
};

class Behavior
{	
	DECLARE_BEHAVIOR(uint8_t, fireEvents, Events::all);
	DECLARE_BEHAVIOR(bool, serializeable, true);
	DECLARE_BEHAVIOR(uint16_t, dataFieldPos, -1);
	DECLARE_BEHAVIOR(std::string, dataFieldName, "");
public:
	// for performance resons friend classes needs access to private data
	friend class PropertyBase;
	template<typename Type> friend class Property;

	Behavior()
	{

	}

	/**
	merges all behaviors that are modified from other to this
	*/
	Behavior& merge(const Behavior& other)
	{
		if (other.is_fireEvents_modified())
			fireEvents(other.fireEvents());
	}
};
}