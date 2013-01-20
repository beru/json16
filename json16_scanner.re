#include "json16_scanner.h"

namespace json16 {

TokenType Scan(const char*& p)
{
const char* q = 0;
#define	YYCTYPE		char
#define	YYCURSOR	p
#define	YYLIMIT		p
#define	YYMARKER	q
#define	YYFILL(n)	0
/*!re2c
	ws = [ \h\t\v\f\r\n]+;
	four_hex_digits = [0-9a-fA-F]{4};
	escaped_char = "\\" ([\"\\/bfnrt] | ('u' four_hex_digits));
	unicode_char = [\040-\176]\[\"\\];
	double_quote = '"';
	string = double_quote (unicode_char|escaped_char)* double_quote;
	digit = [0-9];
	number = '-'? (([1-9] digit*)|'0') ('.' digit+)? ([eE][+-]?digit+)?;
	
	"{"			{return TOKEN_LEFT_BRACE;}
	"}"			{return TOKEN_RIGHT_BRACE;}
	":"			{return TOKEN_COLON;}
	","			{return TOKEN_COMMA;}
	"["			{return TOKEN_LEFT_BRACKET;}
	"]"			{return TOKEN_RIGHT_BRACKET;}
	ws			{return TOKEN_SPACE;}
	"true"		{return TOKEN_TRUE;}
	"false"		{return TOKEN_FALSE;}
	"null"		{return TOKEN_NULL;}
	string		{return TOKEN_STRING;}
	number		{return TOKEN_NUMBER;}
	[^]			{return TOKEN_OTHER;}
*/
}

} // namespace json16
