
#include <stdio.h>

#include "json16.h"
#include "json16_scanner.h"

#include <string.h>
#include <vector>

static inline
size_t getFileSize(FILE* file)
{
	fseek(file, 0, SEEK_END);
	int length = ftell(file);
	fseek(file, 0, SEEK_SET);
	return length;
}

std::string toString(const char* str)
{
	const char* strEnd = str;
	json16::Scan(strEnd);
	char buff[256];
	size_t len = strEnd-str;
	memcpy(buff, str, len);
	buff[len] = 0;
	return buff;
}

template <typename ReaderT>
void printValue(json16::Type type, const ReaderT& reader, int i)
{
	switch (type) {
	case json16::Type_string:
		printf("string %s\n", toString(reader.GetString(i)).c_str());
		break;
	case json16::Type_number:
		printf("double %f\n", reader.GetNumber(i));
		break;
	case json16::Type_object:
		printf("object\n");
		printObject(reader.GetObject(i));
		break;
	case json16::Type_array:
		printf("array\n");
		printArray(reader.GetArray(i));
		break;
	case json16::Type_true:
		printf("true\n");
		break;
	case json16::Type_false:
		printf("false\n");
		break;
	case json16::Type_null:
		printf("null\n");
		break;
	}
}

void printObject(const json16::ObjectReader& reader)
{
	uint16_t cnt = reader.GetCount();
	for (uint16_t i=0; i<cnt; ++i) {
		const char* name = reader.GetName(i);
		printf("name = %s\n", toString(name).c_str());
		json16::Type type = reader.GetValueType(i);
		printValue(type, reader, i);
	}
}

void printArray(const json16::ArrayReader& reader)
{
	uint16_t cnt = reader.GetCount();
	for (uint16_t i=0; i<cnt; ++i) {
		json16::Type type = reader.GetValueType(i);
		printValue(type, reader, i);
	}
}

int main(int argc, char* argv[])
{
	uint16_t work[512];

	if (argc < 2) {
		return 0;
	}

	FILE* f = fopen(argv[1], "rb");
	size_t sz = getFileSize(f);
	std::vector<char> buff(sz);
	fread(&buff[0], 1, sz, f);
	fclose(f);
	
	json16::Parser parser(&buff[0], sz, work);

	json16::Type type = parser.GetValueType();
	json16::ObjectReader reader = parser.GetObject();
	printObject(reader);
	return 0;
}

