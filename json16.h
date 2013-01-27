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
		parsed(parsed)
	{
	}
	
	uint16_t GetCount() const;
	const char* GetName(uint16_t idx) const;
	Type GetValueType(uint16_t idx) const;
	const char* GetString(uint16_t idx) const;
	double GetNumber(uint16_t idx) const;
	ObjectReader GetObject(uint16_t idx) const;
	ArrayReader GetArray(uint16_t idx) const;
	
protected:
	virtual uint16_t getValueOffset(uint16_t idx) const;
	uint16_t getValue(uint16_t idx) const;
	uint16_t offset;
	const char* src;
	const uint16_t* parsed;
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
	const char* GetName(uint16_t idx) const { return ""; }
	uint16_t getValueOffset(uint16_t idx) const;
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
