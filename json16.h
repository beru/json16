#pragma once

namespace json16 {

enum Type {
	Type_string,
	Type_number,
	Type_object,
	Type_array,
	Type_true,
	Type_false,
	Type_null,
};

struct ArrayReader;

struct ObjectReader
{
public:
	ObjectReader(uint16_t offset, const char* src, const uint16_t* parsed)
		:
		offset(offset),
		src(src),
		parsed(parsed),
		readOffset(offset+2)
	{
	}
	
	uint16_t GetCount() const;
	const char* ReadName();
	Type GetValueType() const;
	const char* ReadString();
	double ReadNumber();
	ObjectReader ReadObject();
	ArrayReader ReadArray();
	void MoveNext();
	
protected:
	uint16_t readValue() const;
	uint16_t offset;
	const char* src;
	const uint16_t* parsed;
	uint16_t readOffset;
};

struct ArrayReader : ObjectReader
{
public:
	ArrayReader(uint16_t offset, const char* src, const uint16_t* parsed)
		:
		ObjectReader(offset, src, parsed)
	{
	}
	
protected:
	const char* ReadName();
};

struct Parser
{
public:
	Parser(const char* json, uint16_t len, uint16_t* work);
	
	Type GetValueType() const;
	const char* GetString() const;
	double GetNumber() const;
	ObjectReader GetObject() const;
	ArrayReader GetArray() const;
	
	const char* ErrorMessage;
private:
	const char* json;
	uint16_t* work;
};

} // namespace json16
