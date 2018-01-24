#pragma once
#include <type_traits>
#define toConst() (*const_cast<Property<Type> *>(this))
namespace common {
#ifndef DECLARE_CONVERT
#define DECLARE_CONVERT(NAME, TYPE) \
		const TYPE NAME() const	{ return convert < TYPE >(); }
#endif

class PropertiesBase;
template<typename Type>
class Property: public PropertyBase
{
	Type _value;	
protected:

public:

	Property()
	{
		if (std::is_class<Type>::value)
			_state = StateInitted;
	}

	Property(const Property& other)
	{
		set(other._value);
		_value = other._value;
		_state = other._state;
		_owner = other._owner;
	}

	explicit Property(const Type& value)
	{
		set(_value);
	}

	virtual ~Property()
	{
	}

	operator Type&()
	{
		return get();
	}

	operator const Type&() const
	{
		return get();
	}

	bool operator == (const Property<Type>& right) const
	{
		return _value == right._value;
	}

	bool operator < (const Property<Type>& right) const
	{
		return _value < right._value;
	}

	DECLARE_REL_OPERATORS(Property<Type>)

	/**
	*	checks whether the property was modified
	*	modified will be set on modification after the initial set
	*/
	bool modified() const
	{
		return (_state & StateModified) != 0;
	}

	/**
	*	checks whether the property is valid
	*	
	*/
	bool valid() const
	{
		return (_state & StateInitted) != 0;
	}

	bool hasValue() const
	{
		return (_state & StateSetted) != 0;
	}

	void clearModified()
	{
		_state &= ~StateModified;
	}

	Property<Type>& setThrowWhenNotSetted(bool enable)
	{
		if (enable)
			_state |= StateThrowWhenNotSetted;
		else
			_state &= ~StateThrowWhenNotSetted;
		return *this;
	}

	Property<Type>& setBehavior(const Behavior& behavoir)
	{
		_behavior = &behavoir;
		return *this;
	}

	void set(const Type& value)
	{
		_state |= _state & StateSetted && !(_value == value) ? StateModified : _state;
		_state |= StateInitted | StateSetted;
		_value = value;
		onPropertySet();
	}

	const Type& operator = (const Type& value)
	{
		set(value);
		return value;
	}

	const Property<Type>& operator = (const Property<Type>& other)
	{
		set(other._value);
		return *this;
	}

	Type& getRef()
	{
		return _value;
	}

	Type& get()
	{
		onPropertyGet();
		if (_state & StateThrowWhenNotSetted && hasValue() == false) {
			onPropertyNotAssigned();
			// check if the prop was setted in the onPropertyNotAssigned handler
			if (hasValue() == false)
				throw PropertyNotAssignedException();
		}
		return _value;
	}

	Type& get(const Type& def)
	{
		if (valid() == false || hasValue() == false)
			set(def);
		return get();
	}

	const Type& get() const
	{
		return toConst().get();
	}

	const Type& get(const Type& def) const
	{
		return toConst().get(def);
	}

	std::string dump() const
	{
		if (_state & StateDumping)
			return "recursion detected";
		_state |= StateDumping;
		return toString();
	}

	void dump(std::ostream& os) const override
	{
		os << _value;
	}

	template<class Q = Type>
	typename std::enable_if<!std::is_enum<Q>::value, void>::type
	dump(std::ostream& os) const
	{
		os << _value;
	}

	template<class Q = Type>
	typename std::enable_if<std::is_enum<Q>::value>::type
	dump(std::ostream& os) const
	{
		os << static__cast<int>(_value);
	}
	
	friend std::ostream& operator << (std::ostream& os, const Property<Type>& prop)
	{
		os << prop.dump();
		//os << (Type)prop;
		return os;
	}

};

template<typename Type, class OwnerClass>
class Property_public : public Property<Type>
{
public:
	friend OwnerClass;
};

template<typename Type, typename Friend>
class Property_protected : public Property<Type>
{
public:
	// this allows full access from withn the declartion class/struct
	friend Friend;
protected:
	using Property<Type>::clearModified;
	using Property<Type>::set;
	using Property<Type>::getRef;
	//using Property<Type>::get;
public:
	const Type& get() const
	{
		return Property<Type>::get();
	}
};
}
#undef toConst
#undef DECLARE_CONVERT