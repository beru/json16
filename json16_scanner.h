#pragma once

namespace json16 {

enum TokenType {
	TOKEN_LEFT_BRACE,			// {
	TOKEN_RIGHT_BRACE,			// }
	TOKEN_COLON,				// :
	TOKEN_COMMA,				// ,
	TOKEN_LEFT_BRACKET,			// [
	TOKEN_RIGHT_BRACKET,		// ]
	TOKEN_SPACE,
	TOKEN_EOF,
	TOKEN_OTHER,
	
	TOKEN_VALUE = 0x10,
	TOKEN_TRUE = TOKEN_VALUE|0,		// true
	TOKEN_FALSE = TOKEN_VALUE|1,	// false
	TOKEN_NULL = TOKEN_VALUE|2,		// null
	TOKEN_STRING = TOKEN_VALUE|3,
	TOKEN_NUMBER = TOKEN_VALUE|4,
};

TokenType Scan(const char*& p);

} // namespace json16
