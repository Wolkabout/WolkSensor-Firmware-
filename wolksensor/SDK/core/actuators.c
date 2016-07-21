#include "actuators.h"
#include "logger.h"

actuator_t actuators[NUMBER_OF_ACTUATORS];

int8_t get_index_of_actuator(char* id)
{
	int8_t i;
	for(i = 0; i < NUMBER_OF_ACTUATORS; i++)
	{
		if(strcmp(id, actuators[i].id) == 0)
		{
			return i;
		}
	}
	
	return -1;
}

