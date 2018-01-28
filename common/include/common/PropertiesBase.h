#pragma once
#include <Windows.h>
#include "Map.h"
#include <sstream>
#include <string>
#include <map>
#include "Behaviors.h"
#include <memory>
#include <Poco/Data/TypeHandler.h>

#define DECLARE_EXCEPTION(NAME) \
class NAME : public std::exception \
{ \
public: \
	NAME() : \
		std::exception() \
		{ \
		} \
	 \
	NAME(const char *name) : \
		std::exception(name) \
		{ \
		} \
};

DECLARE_EXCEPTION(PropertyNotAssignedException);
DECLARE_EXCEPTION(PropertyNotInitializedException);
namespace common {
	class PropertyBase;
	class PropertiesBase;
}
namespace Poco {
	namespace Data {
		//#define T ::Data
		//template <class T, bool = std::is_base_of<common::PropertiesBase, T>::value>
		template<>
		class TypeHandler<common::PropertiesBase>;
	}
}
namespace common {

	class PropertiesBase
	{
		friend class PropertyBase;
		friend class Poco::Data::TypeHandler<PropertiesBase>;
	protected:

	protected:
		Map<type_info *, MapBase *> _typeMap;		// this map holds the properties for each data type 
		int _dumpIndent;
		std::shared_ptr<Behavior> _behavior;		///< default behaviors for all properties
	public:
		PropertiesBase() :
			_dumpIndent(0),
			_behavior(std::make_shared<Behavior>())
		{

		}
	protected:

		PropertiesBase(const PropertiesBase& other) :
			_dumpIndent(0)
		{
			this->_typeMap = other._typeMap;
			//cpDebug("Properties Copy Constructor");
		}

		std::map<std::string, std::string> toStringMap()
		{
			for (size_t i = 0; i < _typeMap.size(); i++) {
				//for(int item = 0; item < _typeMap.at(i)
			}
		}

		const char *findPropertyName(PropertyBase *property)
		{
			for (size_t i = 0; i < _typeMap.size(); i++) {
				void *key = _typeMap.at(i)->findByValueRef(property);
				if (key)
					return *(const char **)key;
			}
			return "";
		}

		/**
		override this in the inherited class to support specific default behaviors
		*/
		virtual const Behavior& getBehavior()
		{
			return *_behavior.get();
		}

		virtual Behavior mergeBehavior(const Behavior& behavior)
		{
			return Behavior();
		}

		/**
		onPropertyHandlers will called direct in the property
		*/
		virtual void onPropertyNotInitialized(PropertyBase *property)
		{
			onPropertyNotInitialized(findPropertyName(property));
		}

		virtual void onPropertyNotAssigned(PropertyBase *property)
		{
			onPropertyNotAssigned(findPropertyName(property));
		}

		virtual void onPropertyGet(PropertyBase *property)
		{
			onPropertyGet(findPropertyName(property));
		}
		virtual void onPropertySet(PropertyBase *property)
		{
			onPropertySet(findPropertyName(property));
		}

		/**
		onPropertyGet will called only from this base class when access to the property happens
		*/
		virtual void onPropertyGet(const char *name)
		{
			return;
		}

		virtual void onPropertySet(const char *name)
		{
			return;
		}

		void onPropertyNotFound(const char *name)
		{
			throw PropertyNotAssignedException(name);
		}

		virtual void onPropertyNotInitialized(const char *name)
		{
			throw PropertyNotInitializedException(name);
		}

		virtual void onPropertyNotAssigned(const char *name)
		{
			throw PropertyNotAssignedException(name);
		}
	public:
		~PropertiesBase()
		{
			//cpDebug("~Properties");
			for (UINT i = 0; i < _typeMap.size(); i++)
				delete _typeMap.at(i);
		}
		typedef std::map<std::string, std::string> KeyValueMap;
		operator KeyValueMap();
	};

	class PropertyBase
	{
	protected:
		// set when the property is changed from its first set
		// means the first assignment will not indicated as modified
		static const int StateModified = 0x0001;

		// set when the property well initialized
		// classes with default constructors are always initialized
		// POD types gets Innited when the first set is made
		static const int StateInitted = 0x0002;

		// this state is setted when the property gets it first assignment 
		static const int StateSetted = 0x0004;

		// this state is setted when the property is in dump to avoid recursive dumps 
		static const int StateDumping = 0x0008;

		// when setted then get throws an PropertyNotSetException
		static const int StateThrowWhenNotSetted = 0x0010;

		// avoids recursion in event handlers
		static const int StateInOnSetHandler = 0x20;
		static const int StateInOnGetHandler = 0x40;

		static const int StateOnSetEvent	= 0x1000;
		static const int StateOnGetEvent	= 0x2000;
	protected:
		const Behavior *_behavior;
		PropertiesBase *_owner;
		mutable USHORT _state;

		PropertyBase() :
			_behavior(nullptr),
			_owner(nullptr),
			_state(0)
		{
		}

		virtual void onPropertyGet()
		{
			// here avoid recursive calling of onHandler
			// that could happens when u call in a onGethandler the property.get method
			if (_owner 
				&& (_state & StateInOnGetHandler) == false 
				&& _behavior
				&& (_behavior->fireEvents() & Events::get)) {
				_state |= StateInOnGetHandler;
				_owner->onPropertyGet(this);
			}
			_state &= ~StateInOnGetHandler;
		}

		virtual void onPropertySet()
		{
			if (_owner 
				&& _behavior 
				&& (_behavior->fireEvents() & Events::set) 
				&& (_state & StateInOnSetHandler) == false) 
			{
				_state |= StateInOnSetHandler;
				_owner->onPropertySet(this);
			}
			_state &= ~StateInOnSetHandler;
		}

		virtual void onPropertyNotInitialized()
		{

		}

		virtual void onPropertyNotAssigned()
		{
			if (_owner
				&& _behavior
				&& (_behavior->fireEvents() & Events::not_assigned))
			{
				_owner->onPropertyNotAssigned(this);
			}
		}

	public:

		void setOwner(PropertiesBase *owner)
		{
			_owner = owner;
		}

		virtual void dump(std::ostream& os) const = 0;

		template<typename TargetType>
		const TargetType convert() const
		{
			std::stringstream iostr;
			dump(iostr);
			TargetType target;
			iostr >> target;
			return target;
		}

#define DECLARE_CONVERT(NAME, TYPE) const TYPE NAME() const	{ return convert < TYPE >(); }

		DECLARE_CONVERT(toDouble, double);
		DECLARE_CONVERT(toFloat, float);
		DECLARE_CONVERT(toInt, __int32);
		DECLARE_CONVERT(toInt8, __int8);
		DECLARE_CONVERT(toInt16, __int16);
		DECLARE_CONVERT(toInt32, __int32);
		DECLARE_CONVERT(toInt64, __int64);
		//DECLARE_CONVERT(toInt128, __int128);
		DECLARE_CONVERT(toChar, char);
		DECLARE_CONVERT(toString, std::string);
		//DECLARE_CONVERT(toWString, std::wstring);
		DECLARE_CONVERT(toBool, bool);

#undef DECLARE_CONVERT
	};


}
