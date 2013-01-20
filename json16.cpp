
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
#include "json16_scanner.h"
#include "assert.h"

#include <stdlib.h>

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
	if (v.isContainer) {
		ContainerHeader c = *(ContainerHeader*) &v;
		return c.isObject ? Type_object : Type_array;
	}else {
		NonContainerHeader n = *(NonContainerHeader*) & v;
		const char* p = src + n.srcPos;
		switch (json16::Scan(p)) {
		case TOKEN_TRUE: return Type_true;
		case TOKEN_FALSE: return Type_false;
		case TOKEN_NULL: return Type_null;
		case TOKEN_STRING: return Type_string;
		case TOKEN_NUMBER: return Type_number;
		}
	}
	return Type_array;
}

static inline
const char* getString(uint16_t num, const char* src)
{
	NonContainerHeader v = *(NonContainerHeader*) &num;
	assert(!v.isContainer);
	const char* op = src + v.srcPos;
	const char* p = op;
	TokenType tt = json16::Scan(p);
	assert(tt == TOKEN_STRING);
	return op;
}

static inline
double getNumber(uint16_t num, const char* src)
{
	NonContainerHeader v = *(NonContainerHeader*) &num;
	assert(!v.isContainer);
	const char* op = src + v.srcPos;
	const char* p = op;
	TokenType tt = json16::Scan(p);
	assert(tt == TOKEN_NUMBER);
	char buff[64];
	memcpy(buff, op, p-op);
	buff[p-op] = 0;
	char* endptr;
	return strtod(buff, &endptr);
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
const char* ObjectReader::GetName(uint16_t idx) const { return getString(parsed[offset + 1 + idx * 2] << 1, src); }
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

Parser::Parser(const char* json, uint16_t len, uint16_t* work)
	:
	json(json),
	work(work)
{
	ErrorMessage = 0;
	uint32_t objectBits = 0;
	uint16_t containerPositions[16];
	uint16_t memberCounts[16];
	size_t posIdx = 0;
	const char* p = json;
	uint16_t* pWork = work;
	json16::TokenType tt = TOKEN_NULL;
	enum Mode {
		Mode_BeginBit = 0x10,
		Mode_EndBit = 0x20,
		Mode_None = 0,
		Mode_Object_LEFT_BRACE = 1,
		Mode_Object_NAME = 2,
		Mode_Object_COLON = 3|Mode_BeginBit,
		Mode_Object_VALUE = 4|Mode_EndBit,
		Mode_Object_COMMA = 5,
		Mode_Object_RIGHT_BRACE = 6|Mode_EndBit,
		Mode_Array_LEFT_BRACKET = 7|Mode_BeginBit,
		Mode_Array_VALUE = 8|Mode_EndBit,
		Mode_Array_COMMA = 9|Mode_BeginBit,
		Mode_Array_RIGHT_BRACKET = 10|Mode_EndBit,
	} mode = Mode_None;
	do {
		const char* op = p;
		tt = json16::Scan(p);
		if (tt & TOKEN_VALUE) {
			if (mode == Mode_Object_LEFT_BRACE) {
				if (tt == TOKEN_STRING) {
					mode = Mode_Object_NAME;
					*pWork++ = op - json;
				}else {
					ErrorMessage = "non-string value after {";
					return;
				}
			}else if (mode & Mode_BeginBit) {
				switch (mode) {
				case Mode_Object_COLON:
					mode = Mode_Object_VALUE;
					break;
				case Mode_Array_LEFT_BRACKET:
				case Mode_Array_COMMA:
					mode = Mode_Array_VALUE;
					break;
				}
				NonContainerHeader hdr;
				hdr.isContainer = false;
				hdr.srcPos = op - json;
				*pWork++ = *(const uint16_t*)&hdr;
				++memberCounts[posIdx-1];
			}else {
				ErrorMessage = "value in invalid position";
				return;
			}
		}else {
			switch (tt) {
			case TOKEN_COLON:
				if (mode != Mode_Object_NAME) {
					ErrorMessage = ": not after object name";
					return;
				}
				mode = Mode_Object_COLON;
				break;
			case TOKEN_COMMA:
				if (!(mode & Mode_EndBit)) {
					ErrorMessage = ", not after value";
					return;
				}
				mode = (objectBits & 1) ? Mode_Object_COMMA : Mode_Array_COMMA;
				break;
			case TOKEN_LEFT_BRACE:
				mode = Mode_Object_LEFT_BRACE;
				objectBits = (objectBits<<1)|1;
				containerPositions[posIdx] = pWork - work;
				memberCounts[posIdx] = 0;
				++pWork;
				++posIdx;
				break;
			case TOKEN_RIGHT_BRACE:
				if (!(mode & Mode_EndBit)) {
					ErrorMessage = "} not after value";
					return;
				}else {
					mode = Mode_Object_RIGHT_BRACE;
					objectBits >>= 1;
					--posIdx;
					uint16_t pos = containerPositions[posIdx];
					ContainerHeader hdr;
					hdr.isContainer = 1;
					hdr.isObject = 1;
					hdr.count = memberCounts[posIdx];
					work[pos] = *(const uint16_t*) &hdr;
				}
				break;
			case TOKEN_LEFT_BRACKET:
				mode = Mode_Array_LEFT_BRACKET;
				objectBits <<= 1;
				containerPositions[posIdx] = pWork - work;
				memberCounts[posIdx] = 0;
				++pWork;
				++posIdx;
				break;
			case TOKEN_RIGHT_BRACKET:
				if (!(mode & Mode_EndBit)) {
					ErrorMessage = "] not after value";
					return;
				}else {
					mode = Mode_Array_RIGHT_BRACKET;
					objectBits >>= 1;
					--posIdx;
					uint16_t pos = containerPositions[posIdx];
					ContainerHeader hdr;
					hdr.isContainer = 1;
					hdr.isObject = false;
					hdr.count = memberCounts[posIdx];
					work[pos] = *(const uint16_t*) &hdr;
				}
				break;
			}
		}
		int hoge = 0;
	}while (tt != json16::TOKEN_OTHER);
}

} // namespace json16
