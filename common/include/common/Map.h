#pragma once
#include <ios>

#define DECLARE_REL_OPERATORS(TYPE) \
	bool operator != (const TYPE& rhs) const { return !(*this == rhs); } \
	bool operator >(const TYPE& rhs) const { return rhs < *this; } \
	bool operator <= (const TYPE& rhs) const { return !(rhs < *this); } \
	bool operator >= (const TYPE& rhs) const { return !(*this < rhs); }

#define toConst() (*const_cast<Map<_KeyType, _ValueType> *>(this))

namespace common {

/*
* Base class for the Map necessary for destructing, cloning, dump, compare inherited maps
*/
class MapBase
{
public:
	virtual ~MapBase()
	{
	}

	virtual std::string toString() const = 0;
	virtual std::ostream& dump(std::ostream& os, const std::string& filter = "", int indent = 0) const = 0;

	virtual MapBase *clone() const = 0;
	virtual MapBase *clone(MapBase *into) const = 0;
	virtual int compare(MapBase *other) const = 0;
	virtual void *findByValueRef(void *) = 0;
};

/*
copy templates for deep copy
*/
template<typename T>
T copy(T& other)
{
	return other;
}

template<typename T>
T* copy(T *other)
{
	return other;
}

inline MapBase *copy(MapBase *other)
{
	return other->clone();
}

template<typename T>
int compare(T& left, T& right)
{
	if (left > right)
		return 1;
	if (left < right)
		return -1;
	return 0;
}
/*
template<typename T>
int compare(T *left, T *right)
{
	return left->compare(right);
}
*/
inline int compare(MapBase *left, MapBase *right)
{
	return left->compare(right);
}

template<typename T>
std::ostream& dump(std::ostream& os, T& other, const std::string& filter = "", int indent = 0)
{
	os << other;
	return os;
}
/*
template<typename T>
std::ostream& dump(std::ostream & os, T *other, const std::string& filter = "", int indent = 0)
{
	return other;
	return os;
}
*/
inline std::ostream& dump(std::ostream& os, MapBase *other, const std::string& filter = "", int indent = 0)
{
	return other->dump(os, filter, indent);
}

inline std::ostream& dump(std::ostream& os, MapBase& other, const std::string& filter = "", int indent = 0)
{
	return other.dump(os, filter, indent);
}

inline std::ostream& operator << (std::ostream& os, MapBase *other)
{
	return other->dump(os);
}

inline std::ostream& operator << (std::ostream& os, MapBase& other)
{
	return other.dump(os);
}

/*
* Map class
* This map is much more faster than a std::map when its used for not so much items eq till 1000 items
* for more items there should be a think about a binary search
* Usaly the most Property classes will hold less than 100 items
*/
template<class _KeyType, class _ValueType>
class Map : public MapBase
{
protected:
	typedef _KeyType _KeyType;
	typedef _ValueType _ValueType;
	struct Pair
	{
		_KeyType key;
		_ValueType value;
	};
	Pair *_pair;
	size_t _size;
	mutable size_t _accessIndex;
public:
	void *findByValueRef(void *ref)
	{
		for (size_t i = 0; i < _size; i++) {
			if (ref == &_pair[i].value)
				return &_pair[i].key;
		}
		return nullptr;
	}

protected:
	virtual MapBase *clone() const
	{
		Map *newMap = new Map<_KeyType, _ValueType>();
		return clone(newMap);
	}

	void resize(size_t newSize)
	{
		// allocate the new size
		Pair *newValues = (Pair *)malloc(sizeof Pair * newSize);
		// copy the currently inserted objects
		memcpy(&newValues[0], _pair, sizeof Pair * _size);

		// create new objects and add in place
		// call the add in place new method to call object constructors
		for (_size; _size < newSize; _size++)
			new(newValues + _size) Pair;
		free(_pair);
		_pair = newValues;

	}

	/**
		makes a deep copy of the map
	*/
	virtual MapBase *clone(MapBase *_outMap) const
	{
		typedef Map<_KeyType, _ValueType> MapType;
		MapType *outMap = dynamic_cast<MapType *>(_outMap);
		outMap->clear();
		outMap->resize(_size);
		for (size_t i = 0; i < _size; i++) {
			outMap->_pair[i].key = copy(_pair[i].key);
			outMap->_pair[i].value = copy(_pair[i].value);
		}
		return outMap;
	}

	_ValueType *findRef(const _KeyType& key)
	{
		// find the reference by the last accessIndex its fast when the same property
		// will be continues accessed more times 
		if (_size && _pair[_accessIndex].key == key) {
			return &_pair[_accessIndex].value;
		}
		for (_accessIndex = 0; _accessIndex < _size; _accessIndex++) {
			if (_pair[_accessIndex].key == key) {
				return &_pair[_accessIndex].value;
			}
		}
		if (std::is_class<_ValueType>::value) {
			_accessIndex = _size;
			resize(_size + 1);
			_pair[_accessIndex].key = key;
			return &_pair[_accessIndex].value;
		}
		_accessIndex = 0;
		return NULL;
	}

	const _ValueType *findRef(const _KeyType& key) const
	{
		return toConst().findRef(key);
	}

public:
	Map() :
		_pair(0),
		_size(0),
		_accessIndex(0)
	{
	}

	Map(const Map& other)
	{
		*this = other;
	}

	virtual ~Map()
	{
		clear();
	}

	Map& operator = (const Map& other)
	{
		return *dynamic_cast<Map *>(other.clone(this));
	}

	int compare(MapBase *other) const
	{
		if (*this > dynamic_cast<Map&>(*other))
			return 1;
		if (*this < dynamic_cast<Map&>(*other))
			return -1;
		return 0;
	}

	friend bool operator == (const Map& left, const Map& right)
	{
		if (left._size != right._size)
			return false;
		for (size_t i = 0; i < left._size; i++) {
			if (left._pair[i].key != right._pair[i].key)
				return false;
			if (common::compare(left._pair[i].value, right._pair[i].value) != 0)
				return false;			
		}
		return true;
	}

	friend bool operator < (const Map& left, const Map& right)
	{
		if (left._size < right._size)
			return true;
		if (left._size > right._size)
			return false;
		for (size_t i = 0; i < left._size; i++) {
			if (left._pair[i].key < right._pair[i].key)
				return true;
			if (common::compare(left._pair[i].value, right._pair[i].value) != -1)
				return false;
		}
		return false;
	}

	DECLARE_REL_OPERATORS(Map);

	void clear()
	{
		while (_size) {
			Pair *o = &_pair[--_size];
			o->~Pair();
		}
		free(_pair);
		_pair = nullptr;
		_accessIndex = 0;
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
		_accessIndex = _size;
		resize(_size + 1);
		_pair[_accessIndex].key = key;
		_pair[_accessIndex].value = value;
	}

	_ValueType& get(_KeyType key)
	{
		_ValueType *v = findRef(key);
		if (v)
			return *v;
		throw std::exception("property not initialized");
	}

	const _ValueType& get(_KeyType key) const
	{
		return toConst().get(key);
	}

	_ValueType& at(UINT index)
	{
		if (index >= _size)
			throw std::exception("bounderies exeeded");
		return _pair[index].value;
	}

	const _ValueType& at(UINT index) const
	{
		return toConst().at(index);
	}

	std::ostream& dump(std::ostream& os, const std::string& filter = "", int indent = 0) const
	{
		bool isMap = typeid(_ValueType) == typeid(MapBase *);
		for (size_t i = 0; i < _size; i++) {
			if (!isMap)
				os << _pair[i].key << " = ";
			common::dump(os, _pair[i].value, filter, indent + 4) << std::endl;
		}
		return os;

	}

	std::string toString() const
	{
		std::string ret;
		std::stringstream ss;
		for (size_t i = 0; i < _size; i++) {
			ss 
				<< _pair[i].key 
				//<< " = " << common::dump(ss, _pair[i].value) 
				<< std::endl;
		}
		return ss.str();
	}
};
}

#undef toConst