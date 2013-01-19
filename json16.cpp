
/*

uint16_t value format

string
number
true
false
null
	isContainer : 1 (false)
	srcPos : 15
	
object
	isContainer : 1 (true)
	isObject : 1 (true)
	count : 14
	string[]
	value[]
	
array
	isContainer : 1 (true)
	isObject : 1 (false)
	count : 14
	value[]

*/

#include "json16.h"
#include "assert.h"

namespace json16 {

struct ValueHeader
{
	uint16_t isContainer : 1;
	uint16_t remain : 15;
};

struct ContainerHeader
{
	uint16_t isContainer : 1;
	uint16_t isObject : 1;
	uint16_t count : 14;
};

struct NonContainerHeader
{
	uint16_t isContainer : 1;
	uint16_t srcPos : 15;
};


static inline
uint16_t getCount(uint16_t num)
{
	ContainerHeader v = *(ContainerHeader*) &num;
	assert(v.isContainer);
	return v.count;
}

static inline
Type getValueType(uint16_t num, const char* src, const uint16_t* parsed)
{
	ValueHeader v = *(ValueHeader*) &num;
	return Type_array;
}

static inline
const char* getString(uint16_t num, const char* src)
{
	NonContainerHeader v = *(NonContainerHeader*) &num;
	assert(!v.isContainer);
	return "";
}

static inline
double getNumber(uint16_t num, const char* src)
{
	NonContainerHeader v = *(NonContainerHeader*) &num;
	assert(!v.isContainer);
	return 1.0;
}

static inline
ObjectReader getObject(uint16_t num, uint16_t offset, const char* src, const uint16_t* parsed)
{
	ContainerHeader v = *(ContainerHeader*) &num;
	assert(v.isContainer);
	assert(v.isObject);
	return ObjectReader(offset, src, parsed);
}

static inline
ArrayReader getArray(uint16_t num, uint16_t offset, const char* src, const uint16_t* parsed)
{
	ContainerHeader v = *(ContainerHeader*) &num;
	assert(v.isContainer);
	assert(!v.isObject);
	return ArrayReader(offset, src, parsed);
}

uint16_t ObjectReader::getValueOffset(uint16_t idx) const { return offset + 1 + idx * 2 + 1; }
uint16_t ObjectReader::getValue(uint16_t idx) const  { return parsed[getValueOffset(idx)]; }
uint16_t ObjectReader::GetCount() const { return getCount(parsed[offset]); }
const char* ObjectReader::GetName(uint16_t idx) const { return src + (parsed[offset + 1 + idx * 2]); }
Type ObjectReader::GetValueType(uint16_t idx) const { return getValueType(getValue(idx), src, parsed); }
const char* ObjectReader::GetString(uint16_t idx) const { return getString(getValue(idx), src); }
double ObjectReader::GetNumber(uint16_t idx) const { return getNumber(getValue(idx), src); }
ObjectReader ObjectReader::GetObject(uint16_t idx) const { return getObject(getValue(idx), getValueOffset(idx), src, parsed); }
ArrayReader ObjectReader::GetArray(uint16_t idx) const { return getArray(getValue(idx), getValueOffset(idx), src, parsed); }

uint16_t ArrayReader::getValueOffset(uint16_t idx) const { return offset + 1 + idx; }
uint16_t ArrayReader::getValue(uint16_t idx) const { return parsed[getValueOffset(idx)]; }
uint16_t ArrayReader::GetCount() const { return getCount(parsed[offset]); }
Type ArrayReader::GetValueType(uint16_t idx) const { return getValueType(getValue(idx), src, parsed); }
const char* ArrayReader::GetString(uint16_t idx) const { return getString(getValue(idx), src); }
double ArrayReader::GetNumber(uint16_t idx) const { return getNumber(getValue(idx), src); }
ObjectReader ArrayReader::GetObject(uint16_t idx) const { return getObject(getValue(idx), getValueOffset(idx), src, parsed); }
ArrayReader ArrayReader::GetArray(uint16_t idx) const { return getArray(getValue(idx), getValueOffset(idx), src, parsed); }

Type Parser::GetValueType() const { return getValueType(*work, json, work); }
const char* Parser::GetString() const { return getString(*work, json); }
double Parser::GetNumber() const { return getNumber(*work, json); }
ObjectReader Parser::GetObject() const { return getObject(*work, 0, json, work); }
ArrayReader Parser::GetArray() const { return getArray(*work, 0, json, work); }

Parser::Parser(const char* json, uint16_t* work)
	:
	json(json),
	work(work)
{
	
}

} // namespace json16
