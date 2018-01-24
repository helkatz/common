#include <string>
#include <Windows.h>
#include "logger.h"
/*
 * Base class for the Map necessary to destructing the during runtime created Map
*/
class MapBase
{
public:
	virtual ~MapBase()
	{}
};

/*
 * Map class
 * This map is much more faster than a std::map when its used for not so much items eq till 1000 items
 * for more items there should be a think about a binary search
 * Usaly the most Property classes will hold less than 100 items
*/

template<typename _KeyType, typename _ValueType>
class Map : public MapBase
{
	_KeyType *_keyTypeArr;
	_ValueType *_valueTypeArr;
	mutable int _accessIndex;
	int _size;
	_ValueType* findRef(const _KeyType& key)
	{
		// find the reference by the last accessIndex its fast when the same property
		// will be accessed more times without no other access
		if (_size && _keyTypeArr[_accessIndex] == key) {
			return &_valueTypeArr[_accessIndex];
		}
		for (_accessIndex = 0; _accessIndex < _size; _accessIndex++) {
			if (_keyTypeArr[_accessIndex] == key) {
				return &_valueTypeArr[_accessIndex];
			}
		}
		return NULL;
	}
public:
	Map() :
		_keyTypeArr(NULL),
		_valueTypeArr(NULL),
		_accessIndex(0),
		_size(0)
	{
		cpDebug("Map");
	}

	Map(const Map& other):
		Map()
	{
		if (other._size == 0)
			return;
		_size = other._size;
		_accessIndex = other._accessIndex;
		_keyTypeArr = new _KeyType[_size];
		_keyTypeArr = new _ValueType[_size];
		memcpy(_keyTypeArr, other._keyTypeArr, sizeof(_KeyType) * _size);
		memcpy(_valueTypeArr, other._valueTypeArr, sizeof(_ValueType) * _size);
	}

	virtual ~Map()
	{
		if (_size) {
			delete[] _keyTypeArr;
			delete[] _valueTypeArr;
		}
		cpDebug("~Map");
	}

	size_t size() const
	{
		return _size;
	}

	bool has(const _KeyType& key) const
	{
		return findRef(key) != NULL;
	}

	void set(_KeyType key, const _ValueType& value)
	{
		// find the reference to set the value
		_ValueType *v = findRef(key);
		if (v) {
			*v = value;
			return;
		}
		// when not found grow the array and append the new value
		_KeyType *newKeyTypeArr = new _KeyType[_size + 1];
		_ValueType *newValueTypeArr = new _ValueType[_size + 1];
		if (_size) {
			memcpy(newKeyTypeArr, _keyTypeArr, sizeof(_KeyType) * _size);
			memcpy(newValueTypeArr, _valueTypeArr, sizeof(_ValueType) * _size);
			delete[] _keyTypeArr;
			delete[] _valueTypeArr;
		}
		_keyTypeArr = newKeyTypeArr;
		_valueTypeArr = newValueTypeArr;
		_keyTypeArr[_size] = key;
		_valueTypeArr[_size] = value;
		_accessIndex = _size;
		_size++;
	}

	const _ValueType& get(_KeyType key)
	{
		_ValueType *v = findRef(key);
		if (v)
			return *v;
		throw std::exception("property not initialized");
	}

	const _ValueType& get() const
	{
		if (_size && _keyTypeArr[_accessIndex] == key) {
			return _valueTypeArr[_accessIndex];
		}
		throw std::exception("property not initialized");
	}

	const _ValueType& at(UINT index) const
	{
		if (index >= _size)
			throw std::exception("bounderies exeeded");
		return _valueTypeArr[index];
	}
};

/**
 * PropertiesBase base class for template class Properties
 * \param[in]
 *
*/
class PropertiesBase
{
	/// this map holds the properties for each data type 
	Map<type_info *, MapBase *> _typeMap;
protected:
	PropertiesBase()
	{
	}

	PropertiesBase(const PropertiesBase& other)
	{
		_typeMap = other._typeMap;
	}

	virtual ~PropertiesBase()
	{
		for (UINT i = 0; i < _typeMap.size(); i++)
			delete _typeMap.at(i);
	}

	/// <summary>
	/// Ons the property not found.
	/// </summary>
	/// <param name="name">The name.</param>
	/// <returns>true when the property was setted here</returns>
	virtual bool onPropertyNotFound(const char *name)
	{
		return false;
	}

	/// <summary>
	/// Ons the set property.
	/// </summary>
	/// <param name="name">The name.</param>
	virtual void onSetProperty(const char *name)
	{

	}

	/// <summary>
	/// Determines whether the specified property is modified.
	/// </summary>
	/// <param name="name">The name.</param>
	/// <returns></returns>
	bool isModified(const std::string& name)
	{

	}

	/// <summary>
	/// Clears the modified state of the property.
	/// </summary>
	/// <param name="name">The name.</param>
	/// <returns></returns>
	bool clearModified(const std::string& name)
	{

	}
};

template<typename Class>
struct Properties : public PropertiesBase
{
	typedef Class Class;
	template<typename Type>
	Class& set(const char *name, const Type& value)
	{
		return PropertiesBase::set<Class, Type>(name, value);
	}

	template<typename Type>
	bool has(const char *name) const
	{
		type_info *ti = (type_info *)&typeid(Type);
		typedef Map<const char *, Type> MapType;
		MapType *map = NULL;

		if (_typeMap.has(ti) == false)
			return false;
		else
			map = dynamic_cast<MapType *>(_typeMap.get(ti));
		return map->has(name);
	}

	template<typename Class, typename Type>
	Class& set(const char *name, const Type& value)
	{
		type_info *ti = (type_info *)&typeid(Type);
		typedef Map<const char *, Type> MapType;

		MapType *map;
		if (_typeMap.has(ti) == false) {
			map = new MapType;
			_typeMap.set(ti, map);
		}
		else
			map = (MapType *)_typeMap.get(ti);
		map->set(name, value);

		return dynamic_cast<Class&>(*this);
	}

	template <typename Type >
	const Type& get(const char *name)
	{
		type_info *ti = (type_info *)&typeid(Type);
		typedef Map<const char *, Type> MapType;

		if (_typeMap.has(ti) == false) {
			throw std::exception("property not found");
		}
		MapType *map = dynamic_cast<MapType *>(_typeMap.get(ti));
		return map->get(name);
	}

	template <typename Type >
	const Type& get(const char *name, const Type& def)
	{
		if (has<Type>(name) == false)
			return def;
		return get<Type>(name);
	}
};


#define PROPERTY_US_SET(TYPE, NAME, ...) \
Class&	set_##NAME(const TYPE& value)			{ return Properties::set<TYPE>(#NAME, value); }\

#define PROPERTY_US_GET(TYPE, NAME, ...) \
const TYPE&	get_##NAME()						{ return Properties::get<TYPE>(#NAME, __VA_ARGS__); }\
const TYPE&	NAME()								{ return Properties::get<TYPE>(#NAME, __VA_ARGS__); }\
bool has_##NAME()								{ return Properties::has<TYPE>(#NAME); }

#define PROPERTY_UCC_SET(TYPE, NAME, ...) \
Class&	Set##NAME(const TYPE& value)			{ return Properties::set<TYPE>(#NAME, value); }\

#define PROPERTY_UCC_GET(TYPE, NAME, ...) \
const TYPE&	Get##NAME()							{ return Properties::get<TYPE>(#NAME, __VA_ARGS__); }\
const TYPE&	NAME()								{ return Properties::get<TYPE>(#NAME, __VA_ARGS__); }\
bool Has##NAME()								{ return Properties::has<TYPE>(#NAME); }


#define PROPERTY_LCC_SET(TYPE, NAME, ...) \
Class&	set##NAME(const TYPE& value)			{ return Properties::set<TYPE>(#NAME, value); }\

#define PROPERTY_LCC_GET(TYPE, NAME, ...) \
const TYPE&	get##NAME()							{ return Properties::get<TYPE>(#NAME, __VA_ARGS__); }\
const TYPE&	NAME()								{ return Properties::get<TYPE>(#NAME, __VA_ARGS__); }\
bool has##NAME()								{ return Properties::has<TYPE>(#NAME); }



#define PROPERTY_US(TYPE, NAME, ...) \
	PROPERTY_US_GET(TYPE, NAME, __VA_ARGS__) \
	PROPERTY_US_SET(TYPE, NAME, __VA_ARGS__)

#define PROPERTY_LCC(TYPE, NAME, ...) \
	PROPERTY_LCC_GET(TYPE, NAME, __VA_ARGS__) \
	PROPERTY_LCC_SET(TYPE, NAME, __VA_ARGS__)

#define PROPERTY_UCC(TYPE, NAME, ...) \
	PROPERTY_UCC_GET(TYPE, NAME, __VA_ARGS__) \
	PROPERTY_UCC_SET(TYPE, NAME, __VA_ARGS__)
