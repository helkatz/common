#if 0
#include <gtest/gtest.h>
#include "PropertiesTest.h"
#include <iostream>
#include <array>
#include <new>
#include <string>
#include <Poco/Data/Session.h>
#include "Poco/Data/SQLite/Connector.h"
#include "Poco/Data/SQLite/Extractor.h"
class Data : public common::Properties < Data >
{
	//PROPERTIES_BEHAVIOR(Behavior().events(false));
	
	PROPERTY_LCC(int, Id, Behavior().dataFieldPos(0));
	PROPERTY_LCC(std::string, Name, Behavior().dataFieldPos(1));
	//PROPERTY_LCC(Poco::DateTime, Timestamp);
public:
	common::PropertiesBase& getBase() { return *this; }
};



TEST(misc, misc)
{

	}
namespace Poco {
	namespace Data {
		//#define T ::Data
		//template<typename T>
		//template <bool = std::is_base_of<common::PropertiesBase, T>::value>
		//class TypeHandler<T>;
		//template<>
		//class TypeHandler<common::PropertiesBase>;
		//template<>
		//class TypeHandler<::Data>;

	}
}
#if 1
namespace Poco {
	namespace Data {
//#define T ::Data
		//template <class T, bool = std::is_base_of<common::PropertiesBase, T>::value>
		template<>
		class TypeHandler<common::PropertiesBase>
		{
		public:
			static size_t size()
			{
				return 9;
			}

			static void bind(size_t pos, const common::PropertiesBase& obj, AbstractBinder* pBinder, AbstractBinder::Direction dir)
			{
				poco_assert_dbg(pBinder != 0);
				//TypeHandler<int>::bind(pos++, obj.getRowid(), pBinder);
			}

			static void prepare(size_t pos, const common::PropertiesBase& obj,
								AbstractPreparation::PreparatorPtr pPrepare)
			{
				poco_assert_dbg(pPrepare.isNull() == false);
				//obj.getBehavior
				//TypeHandler<int>::prepare(pos++, obj.getRowid(), pPrepare);
			}

			static void extract(size_t pos, common::PropertiesBase& obj,
								const common::PropertiesBase& defVal, AbstractExtractor::Ptr pExt)
								/// obj will contain the result, defVal contains values we should use when one column is NULL
			{
				//common::PropertiesBase::KeyValueMap map = obj;
				poco_assert_dbg(pExt.isNull() == false);
				//obj._typeMap
				//int rowid;

				//TypeHandler<int>::extract(pos++, rowid, defVal.getRowid(), pExt);

				//obj.setRowid(rowid);

			}
		};
	}
}

#endif

struct Person
{
	std::string lastName;
	std::string firstName;
	std::string address;
	int age;
	Person() { age = 0; }
	Person(const std::string& ln, const std::string& fn, const std::string& adr, int a) :lastName(ln), firstName(fn), address(adr), age(a)
	{
	}
	bool operator==(const Person& other) const
	{
		return lastName == other.lastName && firstName == other.firstName && address == other.address && age == other.age;
	}

	bool operator < (const Person& p) const
	{
		if (age < p.age)
			return true;
		if (lastName < p.lastName)
			return true;
		if (firstName < p.firstName)
			return true;
		return (address < p.address);
	}

	const std::string& operator () () const
		/// This method is required so we can extract data to a map!
	{
		// we choose the lastName as examplary key
		return lastName;
	}
};


namespace Poco {
	namespace Data {


		template <>
		class TypeHandler<Person>
		{
		public:
			static void bind(std::size_t pos, const Person& obj, AbstractBinder::Ptr pBinder, AbstractBinder::Direction dir)
			{
				// the table is defined as Person (LastName VARCHAR(30), FirstName VARCHAR, Address VARCHAR, Age INTEGER(3))
				poco_assert_dbg(!pBinder.isNull());
				pBinder->bind(pos++, obj.lastName, dir);
				pBinder->bind(pos++, obj.firstName, dir);
				pBinder->bind(pos++, obj.address, dir);
				pBinder->bind(pos++, obj.age, dir);
			}

			static void prepare(std::size_t pos, const Person& obj, AbstractPreparator::Ptr pPrepare)
			{
				// the table is defined as Person (LastName VARCHAR(30), FirstName VARCHAR, Address VARCHAR, Age INTEGER(3))
				poco_assert_dbg(!pPrepare.isNull());
				pPrepare->prepare(pos++, obj.lastName);
				pPrepare->prepare(pos++, obj.firstName);
				pPrepare->prepare(pos++, obj.address);
				pPrepare->prepare(pos++, obj.age);
			}

			static std::size_t size()
			{
				return 4;
			}


			static void extract(std::size_t pos, Person& obj, const Person& defVal, AbstractExtractor::Ptr pExt)
			{
				auto * ext = dynamic_cast<Poco::Data::SQLite::Extractor *>(pExt.get());
	
				poco_assert_dbg(!pExt.isNull());
				if (!pExt->extract(pos++, obj.lastName))
					obj.lastName = defVal.lastName;
				if (!pExt->extract(pos++, obj.firstName))
					obj.firstName = defVal.firstName;
				if (!pExt->extract(pos++, obj.address))
					obj.address = defVal.address;
				if (!pExt->extract(pos++, obj.age))
					obj.age = defVal.age;
			}

		private:
			TypeHandler();
			~TypeHandler();
			TypeHandler(const TypeHandler&);
			TypeHandler& operator=(const TypeHandler&);
		};


	}
} // namespace Poco::Data

TEST(PropertiesDataTest, bind)
{
	using namespace Poco::Data;
	using namespace Keywords;
	//Poco::Data::SQLite::Connector::registerConnector();
	Session sess("SQLite", "sample.db");
	sess << "drop table if exists Data", now;
	sess << "\
		create table Data \
		(id int, name varchar(50), `timestamp` datetime)",
		now;

	Data data;
	data
		.setId(1)
		.setName("item1")
	//	.setTimestamp(Poco::LocalDateTime().timestamp())
	;
	sess << "\
		insert into Data \
		(id, name, timestamp) values(?, ?, ?)",
		bind(data.getId()), 
		bind(data.getName()),
		//bind(data.getTimestamp()),
		now;
	Data data1;
	struct MyData
	{
		std::string _name;
		std::string operator () () const
		{ 
			return _name;
		}
		MyData()
		{

		}
	};
	MyData d;
	std::make_pair(d(), d);
	std::map<std::string, MyData> map;
	
	common::Properties<int> prop;
	sess << "\
		select * from Data",
		//into(map),
		//into(data1.getBase()),
		//into(data1.Timestamp().get()),
		now;
	cpDebug(data1.dump());
	class V
	{
	public:
		//std::string operator ();
		Poco::UInt64 operator()() const
			/// we need this operator to return the key for the map and multimap
		{
			return 0;
		}
	};

	std::map<std::string, Person> m;
	sess << "\
		select * from Data",
		into(m),
		now;
}
#endif