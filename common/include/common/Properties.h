#pragma once

#include <string>
#include <Windows.h>
#include "logger.h"
#include <sstream>
#include <algorithm>
#include "StreamHelper.h"
#include "PropertiesBase.h"
#include "Property.h"
#include "Map.h"
#define toConst() (*const_cast<Properties<Class> *>(this))

namespace common
{
	template<typename Class>
	class Properties : public PropertiesBase
	{
	public:

		typedef Class Class;
	public:
		virtual ~Properties()
		{

		}

		Properties& operator = (const Properties& other)
		{
			_typeMap = other._typeMap;
			return *this;
		}

		bool operator == (const Properties<Class>& right) const
		{
			return _typeMap == right._typeMap;
		}

		bool operator < (const Properties<Class>& right) const
		{
			return _typeMap < right._typeMap;
		}

		DECLARE_REL_OPERATORS(Properties<Class>)

	protected:
		template<typename Type>
		bool has(const char *name) const
		{
			type_info *ti = (type_info *)&typeid(Property<Type>);
			typedef Map<const char *, Property<Type>> MapType;
			MapType *map = NULL;

			if (_typeMap.has(ti) == false)
				return false;
			else
				map = dynamic_cast<MapType *>(_typeMap.get(ti));
			return map->has(name);
		}

		template<typename Type>
		Class& set(const char *name, const Type& value)
		{
			type_info *ti = (type_info *)&typeid(Property<Type>);
			typedef Map<const char *, Property<Type>> MapType;

			MapType *map;
			if (_typeMap.has(ti) == false) {
				map = new MapType;
				_typeMap.set(ti, map);
			}
			else
				map = dynamic_cast<MapType *>(_typeMap.get(ti));
			Property<Type>& prop = map->get(name);
			prop.setOwner(this);
			prop.setBehavior(getBehavior());
			prop.set(value);
			return dynamic_cast<Class&>(*this);
		}

		template <typename Type >
		Type& get(const char *name)
		{
			try {
				return getProp<Type>(name);
			}
			catch (const PropertyNotAssignedException&) {
				throw PropertyNotAssignedException(name);
			}
		}

		template <typename Type >
		Type& get(const char *name, const Type& def)
		{
			return getProp<Type>(name, def);
		}

		template <typename Type >
		Type& get(const char *name, const Behavior& behavior)
		{
			return getProp<Type>(name, behavior);
		}

		template <typename Type >
		Type& get(const char *name, const Type& def, const Behavior& behavior)
		{
			return getProp<Type>(name, def, behavior);
		}

		template <typename Type >
		Property<Type>& getProp(const char *name)
		{
			type_info *ti = (type_info *)&typeid(Property<Type>);
			typedef Map<const char *, Property<Type>> MapType;
			/*if (_typeMap.has(ti) == false) {
				_typeMap.set(ti, new MapType);
			}*/
			MapType *map = nullptr;
			try {
				map = dynamic_cast<MapType *>(_typeMap.get(ti));
				return map->get(name);

			}
			catch (std::exception& e) {
				map = new MapType;
				_typeMap.set(ti, map);
				Property<Type>& prop = map->get(name);
				prop.setOwner(this);
				prop.setBehavior(getBehavior());
				return prop;
			}
			//Property<Type> *prop = map->findRef(name);
			Property<Type>& prop = map->get(name);
			//prop.setOwner(this);
			//prop.setBehavior(getBehavior());
			return prop;
		}

		template <typename Type >
		Property<Type>& getProp(const char *name, const Type& def)
		{
			// default variant sets the property when not exist to avoid not set event
			if (!has<Type>(name))
				set<Type>(name, def);
			Property<Type>& prop = getProp<Type>(name);
			if (prop.hasValue() == false)
				prop.set(def);
			return prop;
		}

		template <typename Type >
		Property<Type>& getProp(const char *name, const Behavior& behavior)
		{
			return getProp<Type>(name)
				.setBehavior(behavior);
		}

		template <typename Type >
		Property<Type>& getProp(const char *name, const Type& def, const Behavior& behavior)
		{
			return getProp<Type>(name, def)
				.setBehavior(behavior);
		}


		/**
		*	Const implementations
		*/

		template <typename Type >
		const Type& get(const char *name) const
		{
			return toConst().get<Type>(name);
		}

		template <typename Type >
		const Type& get(const char *name, const Type& def) const
		{
			return toConst().get<Type>(name, def);
		}

		template <typename Type >
		const Type& get(const char *name, const Behavior& behavior) const
		{
			return toConst().get<Type>(name, behavior);
		}

		template <typename Type >
		const Type& get(const char *name, const Type& def, const Behavior& behavior) const
		{
			return toConst().get<Type>(name, def, Behavior);
		}

		// Const implementations
		template <typename Type >
		const Property<Type>& getProp(const char *name) const
		{
			return toConst().getProp<Type>(name);

		}

		template <typename Type >
		const Property<Type>& getProp(const char *name, const Type& def) const
		{
			return toConst().getProp<Type>(name, def);
		}

		template <typename Type >
		const Property<Type>& getProp(const char *name, const Behavior& behavior) const
		{
			return toConst().getProp<Type>(name, behavior);
		}

		template <typename Type >
		const Property<Type>& getProp(const char *name, const Type& def, const Behavior& behavior) const
		{
			return toConst().getProp<Type>(name, def, behavior);
		}

public:
		std::string dump(const std::string& filter = "", int indent = 0) const
		{
			std::stringstream ss;
			ss << "Properties " << typeid(Class).name() << " (" << std::endl;
			{
				IndentingOStreambuf indention(ss, 4);
				common::dump(ss, (MapBase *)&_typeMap, filter, indent);
			}
			ss << "}";
			return ss.str();
		}

		friend std::ostream& operator << (std::ostream& os, const Properties<Class>& right)
		{
			os << right.dump();
			return os;
			IndentingOStreambuf indent(os, 4);
			common::dump(os, (MapBase *)&right._typeMap);
			return os;
		}
	};


	// hidden macros means that you will not find with intelicense when u type PROP...
#define hidden_PROPERTY_INIT(NAME) \
private: const char *get##NAME##Name() const { return #NAME; }

#define hidden_PROPERTY_SET(TYPE, NAME, ACCESS, PREFIX, ...) \
	ACCESS: Class&	PREFIX##NAME(const TYPE& value)	 \
		{ return common::Properties<Class>::set<TYPE>(get##NAME##Name(), value); } \
	public: common::Property_##ACCESS<TYPE, Class>& NAME() \
		{ return static_cast<common::Property_##ACCESS<TYPE, Class>&>(common::Properties<Class>::getProp<TYPE>(get##NAME##Name(), __VA_ARGS__)); }

#define hidden_PROPERTY_GET(TYPE, NAME, ACCESS, PREFIX, ...) \
	ACCESS: const TYPE&	PREFIX##NAME() const \
		{ return common::Properties<Class>::get<TYPE>(get##NAME##Name(), __VA_ARGS__); }\
	ACCESS: const TYPE&	PREFIX##NAME(const TYPE& def) const	\
		{ return common::Properties<Class>::get<TYPE>(get##NAME##Name(), def); }\
	ACCESS: const common::Property_##ACCESS<TYPE, Class>& NAME() const \
		{ return static_cast<const common::Property_##ACCESS<TYPE, Class>&>(common::Properties<Class>::getProp<TYPE>(get##NAME##Name(), __VA_ARGS__)); }


#define properties_class(className) \
	class className : public common::Properties < className >

#define properties_set_format = lowercamelcase
	namespace tags {
		struct Type;
		struct String;
		struct Property;
	}
inline tags::Property PROPERTY_LCC(tags::Type, tags::String);

#define property_format_get = get
#define property_format_set = set
#define propertyx(TYPE, NAME, ...) \
	hidden_PROPERTY_INIT(NAME) \
	hidden_PROPERTY_GET(TYPE, NAME, public, property_format_get, __VA_ARGS__) \
	hidden_PROPERTY_SET(TYPE, NAME, public, property_format_set, __VA_ARGS__)


#define PROPERTY_UCC(TYPE, NAME, ...) \
	hidden_PROPERTY_INIT(NAME) \
	hidden_PROPERTY_GET(TYPE, NAME, public, Get, __VA_ARGS__) \
	hidden_PROPERTY_SET(TYPE, NAME, public, Set, __VA_ARGS__)

#define PROPERTY_LCC(TYPE, NAME, ...) \
	hidden_PROPERTY_INIT(NAME) \
	hidden_PROPERTY_GET(TYPE, NAME, public, get, __VA_ARGS__) \
	hidden_PROPERTY_SET(TYPE, NAME, public, set, __VA_ARGS__)

#define PROPERTY_US(TYPE, NAME, ...) \
	hidden_PROPERTY_INIT(NAME) \
	hidden_PROPERTY_GET(TYPE, NAME, public, get_, __VA_ARGS__) \
	hidden_PROPERTY_SET(TYPE, NAME, public, set_, __VA_ARGS__)

#define PROPERTY_UCC_PUBL_PROT(TYPE, NAME, ...) \
	hidden_PROPERTY_INIT(NAME) \
	hidden_PROPERTY_GET(TYPE, NAME, public, Get, __VA_ARGS__) \
	hidden_PROPERTY_SET(TYPE, NAME, protected, Set, __VA_ARGS__)

#define PROPERTY_LCC_PUBL_PROT(TYPE, NAME, ...) \
	hidden_PROPERTY_INIT(NAME) \
	hidden_PROPERTY_GET(TYPE, NAME, public, get, __VA_ARGS__) \
	hidden_PROPERTY_SET(TYPE, NAME, protected, set, __VA_ARGS__)

#define PROPERTY_US_PUBL_PROT(TYPE, NAME, ...) \
	hidden_PROPERTY_INIT(NAME) \
	hidden_PROPERTY_GET(TYPE, NAME, public, get_, __VA_ARGS__) \
	hidden_PROPERTY_SET(TYPE, NAME, protected, set_, __VA_ARGS__)

}
#undef toConst
