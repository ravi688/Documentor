#include "XmlParser.h"
#include <bufferlib/buffer.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <limits.h>

#define UCHAR_MIN 0
#define USHRT_MIN 0
#define ULONG_MIN 0
#define ULLONG_MIN 0

#define TAB(_count_) do\
{\
	for(int i = 0; i < _count_; i++)\
		putch('\t');\
} while(false)

#define IN_RANGE(type_name, value) ((value <= type_name##_MAX) && (type_name##_MIN <= value))

static BUFFER* __XMLget_tag_buffer(const char* mem_buffer, char** out_mem_buffer); 
static bool __XMLattribute_value_parse(const char* value_string, XMLattribute_value* value, XMLattribute_value_type* value_type);
static void __XMLdestroy_tag(XMLtag* tag);

XMLdata XMLparse(const char* mem_buffer)
{
	XMLdata data; 
	BUFFER* tag_buffer = __XMLget_tag_buffer(mem_buffer, NULL);
	#ifdef DEBUG
	if(tag_buffer == NULL)
	{
		printf("[Error] XML::ParseError, no tag parsed\n");
	}
	else printf("[LOG] XML::ParseSuccess, Parsed Successfully\n");
	#endif
	data.tags = tag_buffer;
	return data;
}

static bool __XMLattribute_value_parse(const char* value_string, XMLattribute_value* value, XMLattribute_value_type* value_type)
{
	bool has_int = false;
	bool has_float_point = false;
	bool has_alpha = false;
	bool has_white_space = false;
	bool has_symbol = false;

	bool is_negative = false;
	bool is_hex = false; 
	bool is_oct = false;

	const char* cursor = value_string; 
	while(isspace(*cursor))
			++cursor;
	if((strstr(cursor, "0x") != NULL) || (strstr(cursor, "0X") != NULL))
		is_hex = true; 
	else
	{
		if(*cursor == '0')
		{
			cursor += 1;
			is_oct = true;
			while(*cursor != 0)
			{
				if(!isdigit(*cursor))
				{
					is_oct = false;
					break;
				}
			}
		}
	}


	while(*cursor != 0)
	{
		if(!has_symbol && ispunct(*cursor))
			has_symbol = true;
		else if(!has_int && isdigit(*cursor))
			has_int = true; 
		else if(!has_float_point && ((*cursor) == '.'))
			has_float_point = true;
		else if(!has_alpha && isalpha(*cursor))
			has_alpha = true;
		else if(!has_white_space && isspace(*cursor))
			has_white_space = true;
		else if(has_white_space && has_alpha && has_int && has_float_point && has_symbol)
		{
			#ifdef DEBUG
			printf("[Warning] XML::ValueParseError, string \"%s\" doesn't resemble to any fundamental value\n", value_string);
			#endif
			return false;
		}
		if((*cursor) == '-')
			is_negative = true;
		cursor += 1;
	}

	if(has_int && !has_float_point && !has_alpha && !has_white_space && !has_symbol)
	{
		if(!is_negative)
		{
			uint64_t  int_value; 
			sscanf(value_string, "%u", &int_value);
			if(IN_RANGE(UCHAR, int_value))
			{
				(*value).type_uint8 = (uint8_t)int_value;
				(*value_type) = TYPE_UNSIGNED_INT8;
			}
			else if(IN_RANGE(USHRT, int_value))
			{
				(*value).type_uint16 = (uint16_t)int_value;
				(*value_type) = TYPE_UNSIGNED_INT16;
			}
			else if(IN_RANGE(ULONG, int_value))
			{
				(*value).type_uint32 = (uint32_t)int_value;
				(*value_type) = TYPE_UNSIGNED_INT32;
			}
			else if(IN_RANGE(ULLONG, int_value))
			{
				(*value).type_uint64 = (uint64_t)int_value;
				(*value_type) = TYPE_UNSIGNED_INT64;
			}
			else
			{
				#ifdef DEBUG
				printf("[Warning] XML::ValueOutOfDomainError, value \"%s\" is out of bound of any integer type\n", value_string);
				#endif
				(*value_type) = TYPE_STRING;
				strcpy((*value).type_string, value_string);
			}
		}
		else
		{
			int64_t int_value;
			sscanf(value_string, "%d", &int_value);
			if(IN_RANGE(CHAR, int_value))
			{
				(*value).type_int8 = (int8_t)int_value;
				(*value_type) = TYPE_SIGNED_INT8;
			}
			else if(IN_RANGE(SHRT, int_value))
			{
				(*value).type_int16 = (int16_t)int_value;
				(*value_type) = TYPE_SIGNED_INT16;
			}
			else if(IN_RANGE(LONG, int_value))
			{
				(*value).type_int32 = (int32_t)int_value;
				(*value_type) = TYPE_SIGNED_INT32;
			}
			else if(IN_RANGE(LLONG, int_value))
			{
				(*value).type_int64 = (int64_t)int_value;
				(*value_type) = TYPE_SIGNED_INT64;
			}
			else
			{
				#ifdef DEBUG
				printf("[Warning] XML::ValueOutOfDomainError, value \"%s\" is out of bound of any integer type\n", value_string);
				#endif
				(*value_type) = TYPE_STRING;
				strcpy((*value).type_string, value_string);
			}
		}
	}
	else if(has_int && has_float_point && !has_alpha && !has_white_space && !has_symbol)
	{
		(*value_type) = TYPE_FLOAT;
		float float_value;
		sscanf(value_string, "%f", &float_value); 
		(*value).type_float = float_value;
	}
	else
	{
		(*value_type) = TYPE_STRING;
		strcpy((*value).type_string, value_string);
	}	
	
	return true;	
}

static BUFFER* __XMLget_tag_buffer(const char* mem_buffer, char** out_mem_buffer)
{
	BUFFER* tag_buffer = NULL;
	BUFFER* attribute_buffer = NULL;
	BUFFER* previous_buffer;
	char token_buffer[TOKEN_BUFFER_SIZE];
	char* open_bracket_ptr = NULL;
	const char* copy_mem_buffer = mem_buffer;


parse_next_tag:
	attribute_buffer = NULL;
	open_bracket_ptr = strchr(copy_mem_buffer, '<'); 
	if(open_bracket_ptr == NULL) 
	{
		if(tag_buffer != NULL)
		{
			previous_buffer = BUFget_binded_buffer();
			BUFbind(tag_buffer); 
			BUFfit();
			BUFbind(previous_buffer);
		}
		return tag_buffer; 
	} 

	open_bracket_ptr += 1; 

	while(isspace(*open_bracket_ptr))
		open_bracket_ptr += 1;

	if(*open_bracket_ptr == '/')
	 	return tag_buffer;

	char* _token_buffer = token_buffer;

	while(!isspace(*open_bracket_ptr) && (*open_bracket_ptr != '>'))
	{
		*_token_buffer = *open_bracket_ptr;
		++_token_buffer; 
		++open_bracket_ptr;
	}
	*_token_buffer = 0;
	if(tag_buffer == NULL)
		tag_buffer = BUFcreate(NULL, sizeof(struct XMLtag), 1, 0);
	XMLtag tag; 
	tag.childs = NULL;
	tag.attributes = NULL;
	tag.content = NULL;
	strncpy(tag.name, token_buffer, strlen(token_buffer) + 1);

	while(isspace(*open_bracket_ptr))
		++open_bracket_ptr;
parse_next_attribute:								//[GOTO LABEL: PARSE_NEXT_ATTRIBUTE]
	if(*open_bracket_ptr != '>')
	{
		if(attribute_buffer == NULL)
		 attribute_buffer = BUFcreate(NULL, sizeof(struct XMLattribute), 1, 0);
		XMLattribute attribute;
		memset(token_buffer, 0, TOKEN_BUFFER_SIZE);
		_token_buffer = token_buffer;
		while(!isspace(*open_bracket_ptr) && (*open_bracket_ptr != '='))
		{
			*_token_buffer = *open_bracket_ptr;
			++_token_buffer;
			++open_bracket_ptr; 
		}
		*_token_buffer = 0;

		strncpy(attribute.name, token_buffer, strlen(token_buffer) + 1);

		bool is_equal_sign_exist = false;
		while((*open_bracket_ptr == '=') || isspace(*open_bracket_ptr))
		{
			if(*open_bracket_ptr == '=')
				is_equal_sign_exist = true;
			++open_bracket_ptr; 
		}

		if(!is_equal_sign_exist)
		{
			#ifdef DEBUG
			printf("[Error] XML::SyntaxError, '=' sign expected after \"%s\"\n", token_buffer);
			#endif
			previous_buffer = BUFget_binded_buffer();
			BUFbind(attribute_buffer);
			BUFfree(); 
			BUFbind(tag_buffer);
			BUFfree();
			BUFbind(previous_buffer);
			return NULL;
		}

		if(*open_bracket_ptr == '"')
		{
			memset(token_buffer, 0, TOKEN_BUFFER_SIZE);
			_token_buffer = token_buffer;
			open_bracket_ptr += 1;
			while(*open_bracket_ptr != '"')
			{
				*_token_buffer = *open_bracket_ptr;
				++open_bracket_ptr;
				++_token_buffer;
			}
			open_bracket_ptr += 1;
			*_token_buffer = 0;
			XMLattribute_value value;
			XMLattribute_value_type value_type;
			if(!__XMLattribute_value_parse(token_buffer, &value, &value_type))
			{
				#ifdef DEBUG
				printf("[Error] XML::ValueParseError, value \"%s\" isn't valid\n", token_buffer);
				#endif
				previous_buffer = BUFget_binded_buffer();
				BUFbind(attribute_buffer);
				BUFfree(); 
				BUFbind(tag_buffer);
				BUFfree();
				BUFbind(previous_buffer);
				return NULL;
			}
			attribute.value = value;
			attribute.value_type = value_type;
			previous_buffer = BUFget_binded_buffer();
			BUFbind(attribute_buffer); 
			BUFpush(&attribute); 
			BUFunbind();
			BUFbind(previous_buffer);

			while(isspace(*open_bracket_ptr))
			open_bracket_ptr += 1;

			goto parse_next_attribute;
		}
		else 
		{
			#ifdef DEBUG
			printf("[Error] XML::SyntaxError, Value of \"%s\" isn't defined\n", token_buffer);
			#endif
			return NULL;
		}
	}
	open_bracket_ptr += 1;
	while(isspace(*open_bracket_ptr))
		open_bracket_ptr += 1;
	if(attribute_buffer != NULL)
	{
		previous_buffer = BUFget_binded_buffer();
		BUFbind(attribute_buffer);
		BUFfit();
		BUFbind(previous_buffer);
	}
	tag.attributes = attribute_buffer;
	bool is_tag_closed = false;

parse_closed_tag:												//LABEL
	if(*open_bracket_ptr == '<')
	{
		char* copy_open_bracket_ptr  = open_bracket_ptr;
		open_bracket_ptr += 1;
		while(isspace(*open_bracket_ptr))
			open_bracket_ptr += 1;
		if(*open_bracket_ptr == '/')
		{
			open_bracket_ptr += 1;
			//end of the tag
			while(isspace(*open_bracket_ptr))
				open_bracket_ptr += 1;
			_token_buffer = token_buffer;
			memset(token_buffer, 0, TOKEN_BUFFER_SIZE);
			while(!isspace(*open_bracket_ptr) && (*open_bracket_ptr != '>'))
			{
				*_token_buffer = *open_bracket_ptr;
				open_bracket_ptr += 1; 
				_token_buffer += 1;
			}
			*_token_buffer = 0; 

			while(isspace(*open_bracket_ptr))
				open_bracket_ptr += 1;

			if((*open_bracket_ptr == '>') && (strcmp(token_buffer, tag.name) == 0))
			{
				is_tag_closed = true;		//this is a valid tag
				open_bracket_ptr += 1;
			}
			else
			{
				#ifdef DEBUG
				printf("[Error] XML::SyntaxError, open tag name \"%s\" doesn't matched with closed tag name \"%s\"\n", tag.name, token_buffer);
				#endif
				return NULL; 
			}
		}
		else 
		{
			//collect all the child tags
			char* _open_bracket_ptr;
			tag.childs = __XMLget_tag_buffer(copy_open_bracket_ptr, &_open_bracket_ptr);
			open_bracket_ptr = _open_bracket_ptr;
			goto parse_closed_tag;
		}
	}
	else 
	{
		//open_bracket_ptr += 1;
		 while(isspace(*open_bracket_ptr))
		 	++open_bracket_ptr;
		{
			uint8_t bytes[BUF_BUFFER_OBJECT_SIZE];
			BUFFER* content_buffer = BUFcreate_object(bytes);
			BUFcreate(content_buffer, sizeof(char), 1, 0);
			previous_buffer = BUFget_binded_buffer();
			BUFbind(content_buffer);
			while(*open_bracket_ptr != '<')
			{
				BUFpush(open_bracket_ptr);
				open_bracket_ptr += 1;
			}
			BUFpush_char(0);
			BUFfit();
			tag.content = BUFget_ptr();
			BUFbind(previous_buffer);
		}
	 	goto parse_closed_tag;
	}
	previous_buffer = BUFget_binded_buffer(); 
	BUFbind(tag_buffer); 
	BUFpush(&tag); 
	BUFbind(previous_buffer);
	copy_mem_buffer = open_bracket_ptr;

	if(out_mem_buffer != NULL)
		*out_mem_buffer = open_bracket_ptr;
	goto parse_next_tag;
}

BUFFER* XMLdata_get_tag_buffer(XMLdata xml_data)
{
	return xml_data.tags;
}

XMLtag* XMLdata_get_tags(XMLdata xml_data)
{
	BUFFER* previous_buffer = BUFget_binded_buffer(); 
	BUFbind(xml_data.tags); 
	XMLtag* tag =  (XMLtag*)BUFget_ptr();
	BUFbind(previous_buffer); 
	return tag; 
}

void XMLdata_destroy(XMLdata xml_data)
{
	BUFFER* previous_buffer = BUFget_binded_buffer();
	BUFbind(xml_data.tags); 
	for(int i =0; i < BUFget_element_count(); i++)
	{
		XMLtag* tag = BUFgetptr_at(i); 
		__XMLdestroy_tag(tag);
	}
	BUFfree();
	BUFbind(previous_buffer);
	#ifdef DEBUG
	printf("[LOG] XMLdata successfully destroyed\n");
	#endif
}

static void __XMLdestroy_tag(XMLtag* tag)
{
	if(tag->attributes != NULL)
	{
		BUFFER* previous_buffer = BUFget_binded_buffer();
		BUFbind(tag->attributes); 
		BUFfree(); 
		BUFbind(previous_buffer);
	}
	if(tag->childs != NULL)
	{
		BUFFER* previous_buffer = BUFget_binded_buffer();
		BUFbind(tag->childs); 

		for(int i = 0; i < BUFget_element_count(); i++)
		{
			XMLtag* child_tag = BUFgetptr_at(i);
			__XMLdestroy_tag(child_tag);
		}
		BUFfree();
		BUFbind(previous_buffer);
	}
	if(tag->content != NULL)
		free(tag->content);
}

XMLattribute* XMLtag_get_attributes(XMLtag* tag)
{
	BUFFER* previous_buffer = BUFget_binded_buffer();
	BUFbind(tag->attributes); 
	XMLattribute* attribute = (XMLattribute*)BUFget_ptr();
	BUFbind(previous_buffer);
	return attribute;
}

XMLtag* XMLtag_get_childs(XMLtag* tag)
{
	BUFFER* previous_buffer = BUFget_binded_buffer();
	BUFbind(tag->childs); 
	XMLtag* childs =  (XMLtag*)BUFget_ptr();
	BUFbind(previous_buffer); 
	return childs; 
}

uint32_t XMLtag_get_child_count(XMLtag* tag)
{
	BUFFER* previous_buffer = BUFget_binded_buffer();
	BUFbind(tag->childs); 
	uint32_t count =  BUFget_element_count();
	BUFbind(previous_buffer); 
	return count;
}

uint32_t XMLtag_get_attribute_count(XMLtag* tag)
{
	BUFFER* previous_buffer = BUFget_binded_buffer(); 
	BUFbind(tag->attributes); 
	uint32_t count =  BUFget_element_count();
	BUFbind(previous_buffer); 
	return count;
}

void XMLtag_print(XMLtag* tag, int tab_count)
{
	TAB(tab_count); puts("[TAG]");
	TAB(tab_count); printf("Name: %s\n", tag->name);

	BUFFER* previous_buffer; 
	if(tag->attributes != NULL)
	{
		tab_count++;
		TAB(tab_count); puts("[ATTRIBUTES]");
		previous_buffer = BUFget_binded_buffer();\
		BUFbind(tag->attributes); 
		for(int i = 0; i < BUFget_element_count(); i++)
		{
			XMLattribute_print((XMLattribute*)BUFgetptr_at(i), tab_count);
		}
		BUFbind(previous_buffer);
		--tab_count;
	}
	if(tag->content != NULL)
	{
		TAB(tab_count); puts("[CONTENT]");
		TAB(tab_count); puts(tag->content);
	}
	if(tag->childs != NULL)
	{
		tab_count++;
		TAB(tab_count); puts("[CHILDS]");
		previous_buffer = BUFget_binded_buffer();
		BUFbind(tag->childs); 
		for(int i = 0; i < BUFget_element_count(); i++)
		{
			 XMLtag_print((XMLtag*)BUFgetptr_at(i), tab_count);
		}
		BUFbind(previous_buffer);
		--tab_count;
	}	
}

void XMLattribute_print(XMLattribute* attribute, int tab_count)
{ 
	TAB(tab_count);
	puts("[Atrribute]");
	if((attribute->value_type == TYPE_SIGNED_INT8) ||
	   (attribute->value_type == TYPE_SIGNED_INT16) ||
	   (attribute->value_type == TYPE_SIGNED_INT32) ||
	   (attribute->value_type == TYPE_SIGNED_INT64) ||
	   (attribute->value_type == TYPE_UNSIGNED_INT8) ||
	   (attribute->value_type == TYPE_UNSIGNED_INT16) ||
	   (attribute->value_type == TYPE_UNSIGNED_INT32) ||
	   (attribute->value_type == TYPE_UNSIGNED_INT64))
	{
		TAB(tab_count); printf("name: %s\n", attribute->name); 
		TAB(tab_count); printf("value: %d\n", (attribute->value).type_uint8);
	}
	else if(attribute->value_type == TYPE_STRING)
	{
		TAB(tab_count); printf("name: %s\n", attribute->name); 
		TAB(tab_count); printf("value: %s\n", (attribute->value).type_string);
	}
	else if(attribute->value_type == TYPE_FLOAT)
	{
		TAB(tab_count); printf("name: %s\n", attribute->name); 
		TAB(tab_count); printf("value: %f\n", (attribute->value).type_float);
	}
}