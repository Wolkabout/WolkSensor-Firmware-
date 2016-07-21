#ifndef COMMAND_PARSER_H_
#define COMMAND_PARSER_H_

#include "commands.h"

bool extract_command_from_string_buffer(circular_buffer_t* command_string_buffer, command_t* command);
uint8_t extract_commands_from_string_buffer(circular_buffer_t* command_string_buffer, circular_buffer_t* command_buffer);

#endif /* COMMAND_PARSER_H_ */
