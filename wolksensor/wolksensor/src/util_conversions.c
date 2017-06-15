/*
 * util_conversions.c
 *
 * Created: 01/07/2016 11:08:50
 *  Author: sstankovic
 */ 

#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>


bool is_string_numeric(const char *str)
{
	if (*str == '-')
	++str;

	if (!*str)
	return false;

	while (*str)
	{
		if (!isdigit(*str))
		return false;
		else
		++str;
	}

	return true;
}

void hexstring_to_asciistring( unsigned char* dest, const unsigned char *text )
{
	unsigned int ch;

	for( ; sscanf( (const char*)text, "%2x", &ch ) == 1 ; text += 2 )
	{
		if(ch == 0)
			ch = 48;
		*dest++ = ch ;
	}
	*dest = 0;
}

void asciistring_to_hexstring( unsigned char* dest, const unsigned char *text )
{
	int i;
	int loop_counter = strlen(text);

	for( i=0; i<loop_counter; i++)
	{
		if(*text == 48)
			strncpy(text, NULL, 2);
		sprintf(dest, "%02x", *text++);
		dest += 2;
	}

	dest -= (2*loop_counter);
	strupr(dest);
}

bool is_string_hex(char *text)
{
	int i;
	int len_input_string = strlen(text);
	static const char characters[17] = "0123456789ABCDEF";

	for(i=0; i<len_input_string; i++)
	{
		if(strchr(characters, text[i])==NULL)
			return false;
	}

	return true;
}
