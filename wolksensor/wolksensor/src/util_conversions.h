/*
 * util_conversions.h
 *
 * Created: 01/07/2016 11:09:49
 *  Author: sstankovic
 */ 


#ifndef UTIL_CONVERSIONS_H_
#define UTIL_CONVERSIONS_H_


bool is_string_numeric(const char *str);
bool is_string_decimal_numeric(const char *str);
void hexstring_to_asciistring( unsigned char* dest, const unsigned char *text );
void asciistring_to_hexstring( unsigned char* dest, const unsigned char *text );
bool is_string_hex(char *text);


#endif /* UTIL_CONVERSIONS_H_ */