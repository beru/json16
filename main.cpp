
#include <stdio.h>

#include "json16.h"
#include "json16_scanner.h"

#include <string.h>

int main(int argc, char* argv[])
{
	uint16_t work[512];

	const char* TEST = "{ \"a\\\\b\\u1234c\\\"\":-151231.333 }";
	json16::Parser parser(TEST, strlen(TEST), work);

	json16::Type type = parser.GetValueType();
	json16::ObjectReader rdr = parser.GetObject();
	uint16_t cnt = rdr.GetCount();
	for (uint16_t i=0; i<cnt; ++i) {
		const char* name = rdr.GetName(i);
		type = rdr.GetValueType(i);
		double v = rdr.GetNumber(i);
		int hoge = 0;
	}
	return 0;
}

