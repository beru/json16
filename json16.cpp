
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
	uint16_t position : 15;
	uint16_t isContainer : 1;
};

struct ContainerHeader
{
	uint16_t count : 14;
	uint16_t isObject : 1;
	uint16_t isContainer : 1;
	uint16_t size;
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
		const char* p = src + v.position;
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
	ValueHeader v = *(ValueHeader*) &num;
	assert(!v.isContainer);
	const char* op = src + v.position;
	const char* p = op;
	TokenType tt = json16::Scan(p);
	assert(tt == TOKEN_STRING);
	return op;
}

static inline
double getNumber(uint16_t num, const char* src)
{
	ValueHeader v = *(ValueHeader*) &num;
	assert(!v.isContainer);
	const char* op = src + v.position;
	const char* p = op;
	TokenType tt = json16::Scan(p);
	assert(tt == TOKEN_NUMBER);
	char buff[64];
	memcpy(buff, op, p-op);
	buff[p-op] = 0;
	char* endptr;
	return strtod(buff, &endptr);
}

uint16_t ObjectReader::GetCount() const
{
	return getCount(parsed[offset]);
}

uint16_t ObjectReader::readValue() const
{
	return parsed[readOffset];
}

void ObjectReader::moveNext()
{
	ValueHeader vh = *(const ValueHeader*) &parsed[readOffset];
	if (vh.isContainer) {
		ContainerHeader ch = *(const ContainerHeader*) &parsed[readOffset];
		readOffset += ch.size;
	}else {
		++readOffset;
	}
}

const char* ObjectReader::ReadName()
{
	return getString(parsed[readOffset++], src);
}

Type ObjectReader::GetValueType() const
{
	uint16_t val = readValue();
	return getValueType(val, src, parsed);
}

const char* ObjectReader::ReadString()
{
	uint16_t val = readValue();
	moveNext();
	return getString(val, src);
}

double ObjectReader::ReadNumber()
{
	uint16_t val = readValue();
	moveNext();
	return getNumber(val, src);
}

ObjectReader ObjectReader::ReadObject()
{
	uint16_t offset = readOffset;
	moveNext();
	return ObjectReader(offset, src, parsed);
}

ArrayReader ObjectReader::ReadArray()
{
	uint16_t offset = readOffset;
	moveNext();
	return ArrayReader(offset, src, parsed);
}

Type Parser::GetValueType() const
{
	return getValueType(*work, json, work);
}

const char* Parser::GetString() const
{
	return getString(*work, json);
}

double Parser::GetNumber() const
{
	return getNumber(*work, json);
}

ObjectReader Parser::GetObject() const
{
	return ObjectReader(0, json, work);
}

ArrayReader Parser::GetArray() const
{
	return ArrayReader(0, json, work);
}

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
		Mode_Object_COMMA = 5|Mode_BeginBit,
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
			if (mode == Mode_Object_LEFT_BRACE || mode == Mode_Object_COMMA) {
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
				ValueHeader hdr;
				hdr.isContainer = false;
				hdr.position = op - json;
				*pWork++ = *(const uint16_t*)&hdr;
				++memberCounts[posIdx];
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
				memberCounts[posIdx+1] = 0;
				pWork += 2;
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
					++memberCounts[posIdx];
					uint16_t pos = containerPositions[posIdx];
					ContainerHeader hdr;
					hdr.isContainer = 1;
					hdr.isObject = 1;
					hdr.count = memberCounts[posIdx+1];
					hdr.size = pWork - work - pos;
					*(ContainerHeader*) &work[pos] = hdr;
				}
				break;
			case TOKEN_LEFT_BRACKET:
				mode = Mode_Array_LEFT_BRACKET;
				objectBits <<= 1;
				containerPositions[posIdx] = pWork - work;
				memberCounts[posIdx+1] = 0;
				pWork += 2;
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
					++memberCounts[posIdx];
					uint16_t pos = containerPositions[posIdx];
					ContainerHeader hdr;
					hdr.isContainer = 1;
					hdr.isObject = 0;
					hdr.count = memberCounts[posIdx+1];
					hdr.size = pWork - work - pos;
					*(ContainerHeader*) &work[pos] = hdr;
				}
				break;
			}
		}
		int hoge = 0;
	}while (tt != json16::TOKEN_OTHER);
}

} // namespace json16
