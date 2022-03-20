
#ifndef XML_PARSER_H_
#define XML_PARSER_H_
#include <bufferlib/buffer.h>
#include "defs.h"

typedef _type_specifiers XMLattribute_value_type;

typedef union XMLattribute_value
{
	uint8_t		type_uint8;
	uint16_t	type_uint16;
	uint32_t	type_uint32;
	uint64_t	type_uint64;
	int8_t		type_int8;
	int16_t		type_int16;
	int32_t		type_int32;
	int64_t		type_int64;
	float		type_float;
	double		type_double;
	uint8_t 	type_string[TOKEN_BUFFER_SIZE];
} XMLattribute_value; 


typedef struct XMLattribute
{
	char name[TOKEN_BUFFER_SIZE];
	XMLattribute_value value;
	XMLattribute_value_type value_type;
} XMLattribute; 

typedef struct XMLtag
{
	char name[TOKEN_BUFFER_SIZE]; 
	BUFFER*  attributes;		//type XMLattribute
	BUFFER* childs; 			//type XMLtag
	char* content;			//type string buffer
} XMLtag; 

typedef struct XMLdata
{
	BUFFER* tags;
} XMLdata; 


XMLdata XMLparse(const char* mem_buffer);
void XMLtag_print(XMLtag* tag, int tab_count);
void XMLattribute_print(XMLattribute* attribute, int tab_count);

BUFFER* XMLdata_get_tag_buffer(XMLdata xml_data); 
XMLtag* XMLdata_get_tags(XMLdata xml_data);
void XMLdata_destroy(XMLdata xml_data); 
uint32_t XMLtag_get_child_count(XMLtag* tag); 
uint32_t XMLtag_get_attribute_count(XMLtag* tag);
XMLtag* XMLtag_get_childs(XMLtag* tag); 
XMLattribute* XMLtag_get_attributes(XMLtag* tag);


#endif