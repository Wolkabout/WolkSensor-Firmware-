#ifndef COMMUNICATION_MODULE_H_
#define COMMUNICATION_MODULE_H_

#include "platform_specific.h"
#include "system.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef bool (*communication_module_process_handle_t)(void);

typedef struct  
{
	communication_module_process_handle_t (*sendd)(uint8_t* data_in, uint16_t data_in_size);
	communication_module_process_handle_t (*receive)(uint8_t* buffer_out, uint16_t buffer_out_size, uint16_t* received_data_size_out);
	communication_module_process_handle_t (*stop)(void);
	communication_module_type_data_t (*get_communication_result)(void);
	void (*get_status)(char* status, uint16_t status_length);
}
communication_module_t;

extern communication_module_t communication_module;

#ifdef __cplusplus
}
#endif

#endif /* COMMUNICATION_MODULE_H_ */
