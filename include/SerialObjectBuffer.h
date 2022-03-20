#ifndef _SERIAL_OBJECT_BUFFER_H_
#define _SERIAL_OBJECT_BUFFER_H_

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define SOB_SERIAL_OBJECT_BUFFER_SIZE 32;		//bytes

typedef struct __SERIAL_OBJECT_BUFFER SERIAL_OBJECT_BUFFER;

/*
	[SIZE: 8 + 8 + 8 + 8 = 32 bytes]
	struct __SERIAL_OBJECT_BUFFER
	{
		uint32_t object_count;
		uint32_t bytes_capacity;
		uint32_t bytes_filled;
		uint32_t meta_data;										// |BYTE 1| |BYTE 2| |BYTE 3| |BYTE 4|
																// bit index: 31 -> is_auto_double_resize, 0 means false, 1 means true
																// bit index: 30 -> 0 means heap_allocated_object , 1 means stack_allocated_object
																// bit index: 29 -> is_defragmented,  0 means false, 1 means true
		uint8_t* top_cursor;
		uint8_t* bytes;	
	};
*/

#ifdef __cplusplus
extern "C"
{
#endif


void SOBpush_nbytes(void* object, uint32_t size); 
void SOBpop_object(void* object);
/*Calling this loses the organization of the serial buffer*/
void SOBpop_nbytes(void* object, uint32_t nbytes);

void SOBfit();;
void SOBset_auto_double_resize(bool is_auto_double_resize);




#ifdef __cplusplus
}
#endif

#endif