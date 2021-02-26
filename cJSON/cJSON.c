#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <float.h>
#include <limits.h>
#include <ctype.h>
#include "cJSON.h"
#pragma warning(disable:4996)

// ʹ��Ԥ������ȷ��������λ��
#if INT_MAX == 32767
	#define INTEGER_SIZE 0x0010
#elif INT_MAX == 2147483647
	#define INTEGER_SIZE 0x0020
#elif INT_MAX = 9223372036854775807
	#define INTEGER_SIZE 0x1000
#else
	#error "Failed to determine the size of an integer"
#endif 

// ������
static const char *global_ep;

const char *cJSON_GetErrorPtr(void) {
	return global_ep;
}

// �����ִ�Сд�ַ����Ա�
static int cJSON_strcasecmp(const char *s1, const char *s2) {
	if (!s1) {
		return (s1 == s2) ? 0 : 1; 
	}
	if (!s2) {
		return 1;
	}
	for (; tolower(*(const unsigned char *)s1) == tolower(*(const unsigned char *)s2); s1++, s2++) {
		if (*s1 == 0) {
			return 0;
		}
	}

	return tolower(*(const unsigned char *)s1) - tolower(*(const unsigned char *)s2);
}

static void *(*cJSON_malloc)(size_t) = malloc;
static void(*cJSON_free)(void*) = free;

static char* cJSON_strdup(const char* str) {
	size_t len;
	char* copy;
	len = strlen(str) + 1;
	if (!(copy = (char*)cJSON_malloc(len))) {
		return 0;
	}
	memcpy(copy, str, len);
	return copy;
}

void cJSON_InitHooks(cJSON_Hooks* hooks) {
	if (!hooks) {
		cJSON_malloc = malloc;
		cJSON_free = free;
		return;
	}
	cJSON_malloc = (hooks->malloc_fn) ? hooks->malloc_fn : malloc;
	cJSON_free = (hooks->free_fn) ? hooks->free_fn : free;
}

// �ڲ����캯��
static cJSON *cJSON_New_Item(void) {
	cJSON* node = (cJSON*)cJSON_malloc(sizeof(cJSON));
	if (node) {
		memset(node, 0, sizeof(cJSON));
	}
	return node;
}

// �ڲ���������
void cJSON_Delete(cJSON *c) {
	cJSON *next;
	while (c) {
		next = c->next;
		if (!(c->type & cJSON_IsReference) && c->child) {
			cJSON_Delete(c->child);
		}
		if (!(c->type & cJSON_IsReference) && c->valuestring) {
			cJSON_free(c->valuestring);
		}
		if (!(c->type & cJSON_StringIsConst) && c->string) {
			cJSON_free(c->string);
		}
		cJSON_free(c);
		c = next;
	}
}

// �����ı����֣��������䵽����item��
static const char *parse_number(cJSON *item, const char *num) {
	double n = 0;
	double sign = 1;
	double scale = 0;
	int subscale = 0;
	int signsubscale = 1;

	// ȷ������
	if (*num == '-')
	{
		sign = -1;
		num++;
	}
	// �ж��Ƿ�Ϊ0
	if (*num == '0')
	{
		num++;
	}
	// �ж��Ƿ�Ϊ����
	if ((*num >= '1') && (*num <= '9'))
	{
		do
		{
			n = (n * 10.0) + (*num++ - '0');
		} while ((*num >= '0') && (*num <= '9'));
	}
	// �ж��Ƿ�ΪС��
	if ((*num == '.') && (num[1] >= '0') && (num[1] <= '9'))
	{
		num++;
		do
		{
			n = (n  *10.0) + (*num++ - '0');
			scale--;
		} while ((*num >= '0') && (*num <= '9'));
	}
	// �Ƿ�Ϊָ��e
	if ((*num == 'e') || (*num == 'E'))
	{
		num++;
		// �ж�ָ������
		if (*num == '+')
		{
			num++;
		}
		else if (*num == '-')
		{
			signsubscale = -1;
			num++;
		}
		// �ж�ָ���Ƿ�Ϊ����
		while ((*num >= '0') && (*num <= '9'))
		{
			subscale = (subscale * 10) + (*num++ - '0');
		}
	}

	/* number = +/- number.fraction * 10^+/- exponent */
	n = sign * n * pow(10.0, (scale + subscale * signsubscale));

	item->valuedouble = n;
	item->valueint = (int)n;
	item->type = cJSON_Number;

	return num;
}

// ��ȡ��һ����x���2�η���
static int pow2gt(int x) { 
	--x;  
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
#if INTEGER_SIZE & 0x1110 
	x |= x >> 8;
#endif
#if INTEGER_SIZE & 0x1100 
	x |= x >> 16;
#endif
#if INTEGER_SIZE & 0x1000 
	x |= x >> 32;
#endif
	return x+1;
}

// ��ӡ����
typedef struct {
	char *buffer;
	int length;
	int offset; // ��ռ�ÿռ�
} printbuffer;

// ���·��仺��ռ䣬 ������Ҫneeded���ֽ�
static char* ensure(printbuffer *p, int needed) {
	char *newbuffer;
	int newsize;
	if (!p || !p->buffer)
	{
		return 0;
	}
	needed += p->offset;
	if (needed <= p->length)
	{
		return p->buffer + p->offset;
	}

	newsize = pow2gt(needed);
	newbuffer = (char*)cJSON_malloc(newsize);
	if (!newbuffer)
	{
		cJSON_free(p->buffer);
		p->length = 0;
		p->buffer = 0;

		return 0;
	}
	if (newbuffer)
	{
		memcpy(newbuffer, p->buffer, p->length);
	}
	cJSON_free(p->buffer);
	p->length = newsize;
	p->buffer = newbuffer;

	return newbuffer + p->offset;
}

// ����printbuffer���ַ������³���
static int update(const printbuffer *p) {
	char *str;
	if (!p || !p->buffer) {
		return 0;
	}
	str = p->buffer + p->offset;
	return p->offset + strlen(str);
}

// ����������Ⱦ�ɴ�ӡ�ı�
static char *print_number(const cJSON *item, printbuffer *p) {
	char *str = 0;
	double d = item->valuedouble;
	// ��ǰ����Ϊ 0 
	if (d == 0) {
		if (p) {
			str = ensure(p, 2);
		} else {
			str = (char *)cJSON_malloc(2);
		}
		if (str) {
			strcpy(str, "0");
		}

	} else if ((fabs(((double)item->valueint) - d) <= DBL_EPSILON) && (d <= INT_MAX) && (d >= INT_MIN)) { 	
		// ��ǰ���������� 
		// ����21λ������Ϊ2^64-1�� Ϊ 21λ ����Ҫ21���ַ�
		if (p) {
			str = ensure(p, 21);
		} else {
			str = (char*)cJSON_malloc(21);
		}
		if (str) {
			sprintf(str, "%d", item->valueint);
		}
	} else {
		// ��ǰ����Ϊ������
		if (p) {
			str = ensure(p, 64);
		} else {
			str = (char*)cJSON_malloc(64);
		}
		if (str) {
			// �������С�����������
			if ((d * 0) != 0) {
				sprintf(str, "null");
			} else if ((fabs(floor(d) - d) <= DBL_EPSILON) && (fabs(d) < 1.0e60)) {
				// ��ֵС�� 1.0e60 ������ʾ
				sprintf(str, "%.0f", d);
			} else if ((fabs(d) < 1.0e-6) || (fabs(d) > 1.0e9)) {
				// ��ֵ̫С������ֵ���� ����Ҫ���ÿ�ѧ������
				sprintf(str, "%e", d);
			} else {
				sprintf(str, "%f", d);
			}
		}
	}
	return str;
}

// ����4λ16������
static unsigned parse_hex4(const char *str) {
	unsigned h = 0;
	if ((*str >= '0') && (*str <= '9')) {
		h += (*str) - '0';
	} else if ((*str >= 'A') && (*str <= 'F')) {
		h += (*str) - 'A' + 10;
	} else if ((*str >= 'a') && (*str <= 'f')) {
		h += (*str) - 'a' + 10;
	} else {
		// ��Ч���ַ�
		return 0;
	}

	h = h << 4;
	str++;
	if ((*str >= '0') && (*str <= '9')) {
		h += (*str) - '0';
	}
	else if ((*str >= 'A') && (*str <= 'F')) {
		h += (*str) - 'A' + 10;
	}
	else if ((*str >= 'a') && (*str <= 'f')) {
		h += (*str) - 'a' + 10;
	}
	else {
		// ��Ч���ַ�
		return 0;
	}

	h = h << 4;
	str++;
	if ((*str >= '0') && (*str <= '9')) {
		h += (*str) - '0';
	}
	else if ((*str >= 'A') && (*str <= 'F')) {
		h += (*str) - 'A' + 10;
	}
	else if ((*str >= 'a') && (*str <= 'f')) {
		h += (*str) - 'a' + 10;
	}
	else {
		// ��Ч���ַ�
		return 0;
	}

	h = h << 4;
	str++;
	if ((*str >= '0') && (*str <= '9')) {
		h += (*str) - '0';
	}
	else if ((*str >= 'A') && (*str <= 'F')) {
		h += (*str) - 'A' + 10;
	}
	else if ((*str >= 'a') && (*str <= 'f')) {
		h += (*str) - 'a' + 10;
	}
	else {
		// ��Ч���ַ�
		return 0;
	}

	return h;
}

// �������ȵ�UTF8����ĵ�һ�ֽ�
static const unsigned char firstByteMark[7] = { 0x00, 0x00, 0xC0, 0xE0,	0xF0, 0xF8, 0xFC };

// �����ı��ַ������������䵽����item��
static const char *parse_string(cJSON *item, const char *str, const char **ep) {
	const char *ptr1 = str + 1;
	const char *end_ptr = str + 1;
	char *ptr2;
	char *out;
	int len = 0;
	unsigned uc1;
	unsigned uc2;

	if (*str != '\"') {
		*ep = str;
		return 0;
	}

	// �����ַ�������
	while ((*end_ptr != '\"') && *end_ptr && ++len) {
		if (*end_ptr++ == '\\') {
			if (*end_ptr == '\0') {
				return 0;
			}
			end_ptr++;
		}
	}
	
	out = (char*)cJSON_malloc(len + 1);
	if (!out) {
		return 0;
	}
	item->valuestring = out;
	item->type = cJSON_String;

	ptr1 = str + 1;
	ptr2 = out;
	while (ptr1 < end_ptr) {
		if (*ptr1 != '\\') {
			*ptr2++ = *ptr1++;
		}
		else {
			// ת���ַ�
			ptr1++;
			switch (*ptr1)
			{
				case 'b':
					*ptr2++ = '\b';
					break;
				case 'f':
					*ptr2++ = '\f';
					break;
				case 'n':
					*ptr2++ = '\n';
					break;
				case 'r':
					*ptr2++ = '\r';
					break;
				case 't':
					*ptr2++ = '\t';
					break;
				case 'u':
					uc1 = parse_hex4(ptr1 + 1); // ��ȡ4λ16������
					ptr1 += 4;
					if (ptr1 >= end_ptr) {
						*ep = str;
						return 0;
					}
					// ��� uc1 ��Χ�Ƿ���Ч
					if (((uc1 >= 0xDC00) && (uc1 <= 0xDFFF)) || (uc1 == 0)) {
						// Low Surrogates �� uc1Ϊ0
						*ep = str;
						return 0;
					}

					// UTF16 Surrogates pairs
					if ((uc1 >= 0xD800) && (uc1 <= 0xDBFF)) {
						// ��Ҫ����Low Surrogates �������
						if ((ptr1 + 6) > end_ptr) {
							*ep = str;
							return 0;
						}
						if ((ptr1[1] != '\\') || (ptr1[2] != 'u')) {
							*ep = str;
							return 0;
						}
						uc2 = parse_hex4(ptr1 + 3);
						ptr1 += 6;
						if ((uc2 < 0xDC00) || (uc2 > 0xDFFF)) {
							*ep = str;
							return 0;
						}
						uc1 = 0x10000 + (((uc1 & 0x3FF) << 10) | (uc2 & 0x3FF));
					}


					len = 4;
					// ȷ���ֽڸ���
					if (uc1 < 0x80) {
						// ��ͨASCIIֵ�� ����Ϊ 0xxxxxxx 1���ֽ�
						len = 1;
					} else if (uc1 < 0x800) {
						// �����ֽ� ����Ϊ 110xxxxx 10xxxxxx 
						len = 2;
					} else if (uc1 < 0x10000) {
						len = 3;
					}
					ptr2 += len;
					
					switch (len)
					{
						case 4:
							*--ptr2 = ((uc1 | 0x80) & 0xBF);
							uc1 >>= 6;
						case 3:
							*--ptr2 = ((uc1 | 0x80) & 0xBF);
							uc1 >>= 6;
						case 2:
							*--ptr2 = ((uc1 | 0x80) & 0xBF);
							uc1 >>= 6;
						case 1:
							*--ptr2 = (uc1 | firstByteMark[len]);
					}
					ptr2 += len;
					break;
				default:
					*ptr2++ = *ptr1++;
					break;
			}
			ptr1++;
		}
	}
	*ptr2 = '\0';
	if (*ptr1 == '\"') {
		ptr1++;
	}
	return ptr1;
}

// ���ַ�������Ⱦ�ɴ�ӡ�ı�
static char *print_string_ptr(const char *str, printbuffer *p) {
	const char *ptr1;
	char *ptr2;
	char *out;
	int len = 0;
	int flag = 0;
	unsigned char token;

	// ���ַ���
	if (!str) {
		if (p) {
			// 3���ַ� ˫����+'\0'
			out = ensure(p, 3);
		} else {
			out = (char*)cJSON_malloc(3);
		}
		if (!out) {
			return 0;
		}
		strcpy(out, "\"\"");
		
		return out;
	} 

	// ���������ַ����ո񡢻س��ȵȣ���flag����Ϊ1
	for (ptr1 = str; *ptr1; ptr1++) {
		flag |= (((*ptr1 > 0) && (*ptr1 < 32))) || (*ptr1 == '\"') || (*ptr1 == '\\') ? 1 : 0;
	}
	// �������ַ�
	if (!flag) {
		len = ptr1 - str;
		if (p) {
			out = ensure(p, len + 3);
		} else {
			out = (char*)cJSON_malloc(len + 3);
		}
		if (!out) {
			return 0;
		}

		ptr2 = out;
		*ptr2++ = '\"';
		strcpy(ptr2, str);
		ptr2[len] = '\"';
		ptr2[len + 1] = '\0';
		
		return out;
	}
	
	ptr1 = str;
	// �����ַ������ܳ���
	while ((token = *ptr1) && ++len) {
		if (strchr("\"\\\b\f\n\r\t", token)) {
			len++;
		} else if (token < 32) {
			len += 5;	// \uXXXX
		}
		ptr1++;
	}

	if (p) {
		out = ensure(p, len + 3);
	} else {
		out = (char*)cJSON_malloc(len + 3);
	}
	if (!out) {
		return 0;
	}

	ptr2 = out;
	ptr1 = str;
	*ptr2++ = '\"';
	// �����ַ���
	while (*ptr1) {
		if (((unsigned char)*ptr1 > 31) && (*ptr1 != '\"') && (*ptr1 != '\\')) {
			*ptr2++ = *ptr1++;
		} else {
			*ptr2++ = '\\';
			switch (token = *ptr1) {
				case '\\':
					*ptr2++ = '\\';
					break;
				case '\"':
					*ptr2++ = '\"';
					break;
				case '\b':
					*ptr2++ = 'b';
					break;
				case '\f':
					*ptr2++ = 'f';
					break;
				case '\n':
					*ptr2++ = 'n';
					break;
				case '\r':
					*ptr2++ = 'r';
					break;
				case '\t':
					*ptr2++ = 't';
					break;
				default:
					sprintf(ptr2, "u%04x", token);
					ptr2 += 5;
					break;
			}
		}
	}
	*ptr2++ = '\"';
	*ptr2++ = '\0';

	return out;
}

// ��item�������print_string_ptr
static char *print_string(cJSON *item, printbuffer *p) {
	return print_string_ptr(item->valuestring, p);
}

// ��������
static const char *parse_value(cJSON *item, const char *value, const char **ep);
static char *print_value(const cJSON *item, int depth, int fmt, printbuffer *p); // fmt
//static const char *parse_array(cJSON *item, const char *value, const char **ep);
//static char *print_value(const cJSON *item, int depth, int fmt, printbuffer *p);
//static const char *parse_object(cJSON *item, const char *value, const char **ep);
//static char *print_object(const cJSON *item, int depth, int fmt, printbuffer *p);

// ������ָ�롢null�� �޷���ӡ�ַ�
static const char *skip(const char *in) {
	while (in && *in && ((unsigned char)*in <= 32)) {
		in++;
	}
	return in;
}

// �������󣬴�������㲢���
cJSON *cJSON_ParseWithOpts(const char *value, const char **return_parse_end, int require_null_terminated) {
	const char *end = 0;
	// ��δʹ���ض�����ָ�룬��ʹ��ȫ�ִ���ָ��
	const char **ep = return_parse_end ? return_parse_end : &global_ep;
	cJSON *c = cJSON_New_Item();
	*ep = 0;
	if (!c) {
		return 0;
	}

	end = parse_value(c, skip(value), ep);
	if (!end) {
		// ����ʧ��
		cJSON_Delete(c);
		return 0;
	}
	// ����Ҫ��βΪ��ֵ��cJSON����û������ֵ�� ��������ֵ�ټ������ַ�
	if (require_null_terminated) {
		end = skip(end);
		if (*end) {
			cJSON_Delete(c);
			return 0;
		}
	}
	if (return_parse_end) {
		*return_parse_end = end;
	}

	return c;
}

// cJSON_Parse Ĭ��ѡ��
cJSON *cJSON_Parse(const char *value) {
	return cJSON_ParseWithOpts(value, 0, 0);
}

// ��ȾcJSONΪ�ı���
char *cJSON_Print(const cJSON *item) {
	return print_value(item, 0, 1, 0);
}

char  *cJSON_PrintUnformatted(const cJSON *item) {
	return print_value(item, 0, 0, 0);
}

char *cJSON_PrintBuffered(const cJSON *item, int prebuffer, int fmt) {
	printbuffer p;
	p.buffer = (char*)cJSON_malloc(prebuffer);
	if (!p.buffer) {
		return 0;
	}
	p.length = prebuffer;
	p.offset = 0;

	return print_value(item, 0, fmt, &p);
}

// �������� ���� ��ͬ�ı����ж�Ӧ����
static const char *parse_value(cJSON *item, const char *value, const char **ep) {
	if (!value) {
		return 0;
	}

	if (!strncmp(value, "null", 4)) {
		item->type = cJSON_NULL;
		return value + 4;
	}
	if (!strncmp(value, "false", 5)) {
		item->type = cJSON_False;
		return value + 5;
	}
	if (!strncmp(value, "true", 4)) {
		item->type = cJSON_True;
		item->valueint = 1;
		return value + 4;
	}
	if (*value == '\"') {
		return parse_string(item, value, ep);
	}
	if ((*value == '-') || ((*value >= '0') && (*value <= '9'))) {
		return parse_number(item, value);
	}
	/*
	if (*value == '[') {
		return parse_array(item, value, ep);
	}
	if (*value == '{') {
		return parse_object(item, value, ep);
	}
	*/
	*ep = value;
	return 0;
}

// ��Ⱦitem�����ı�
static char *print_value(const cJSON *item, int depth, int fmt, printbuffer *p) {
	char *out = 0;
	if (!item) {
		return 0;
	}
	if (p) {
		switch ((item->type) & 0xFF)
		{
			case cJSON_NULL:
				out = ensure(p, 5);
				if (out) {
					strcpy(out, "null");
				}
				break;
			case cJSON_False:
				out = ensure(p, 6);
				if (out) {
					strcpy(out, "false");
				}
				break;
			case cJSON_True:
				out = ensure(p, 5);
				if (out) {
					strcpy(out, "true");
				}
				break;
			case cJSON_Number:
				out = print_number(item, p);
				break;
			case cJSON_String:
				out = print_string(item, p);
				break;
			/*case cJSON_Array:
				out = print_array(item, depth, fmt, p);
				break;
			case cJSON_Object:
				out = print_object(item, depth, fmt, p);
				break;*/
		} 
	} else {
		switch ((item->type) & 0xFF)
		{
		case cJSON_NULL:
			out = cJSON_strdup("null");
			break;
		case cJSON_False:
			out = cJSON_strdup("false");
			break;
		case cJSON_True:
			out = cJSON_strdup("true");
			break;
		case cJSON_Number:
			out = print_number(item, 0);
			break;
		case cJSON_String:
			out = print_string(item, 0);
			break;
			/*case cJSON_Array:
				out = print_array(item, depth, fmt, 0);
				break;
			case cJSON_Object:
				out = print_object(item, depth, fmt, 0);
				break;*/
		}
	}

	return out;
}

// ͨ���ı���������

// ��Ⱦ���鵽�ı�

// ͨ���ı���������

// ��Ⱦ�����ı�

// ��ȡ ���鳤�ȡ�cJSON����item
int	  cJSON_GetArraySize(const cJSON *array) {
	cJSON *c = array->child;
	int len = 0;
	while (c) {
		len++;
		c = c->next;
	}
	return len;
}

// ���ݻ�ȡ��item��cJSON����
cJSON *cJSON_GetArrayItem(const cJSON *array, int item) {
	cJSON *c = array ? array->child : 0;
	while (c && item > 0) {
		item--;
		c = c->next;
	}
	return c;
}

// ��ȡkeyֵΪstring��cJSON����
cJSON *cJSON_GetObjectItem(const cJSON *object, const char *string) {
	cJSON *c = object ? object->child : 0;
	while (c && cJSON_strcasecmp(c->string, string)) {
		c = c->next;
	}
	return c;
}

// �ж�keyֵΪstring��cJSON�Ƿ����
int cJSON_HasObjectItem(const cJSON *object, const char *string) {
	return cJSON_GetObjectItem(object, string) ? 1 : 0;
}

// ���鴦����
static void suffix_object(cJSON *prev, cJSON *item) {
	prev->next = item;
	item->prev = prev;
}


// ������գ�reference������ key/value
static cJSON *create_reference(const cJSON *item) {
	cJSON *ref = cJSON_New_Item();
	if (!ref) {
		return 0;
	}
	memcpy(ref, item, sizeof(cJSON));
	ref->string = 0;
	ref->type |= cJSON_IsReference;
	ref->next = ref->prev = 0;
	return ref;
}

// ��array��������Ӷ���item
void cJSON_AddItemToArray(cJSON *array, cJSON *item) {
	cJSON *c = array->child;
	if (!item) {
		return;
	}
	if (!c) {
		array->child = item;
	} else {
		while (c->next) {
			c = c->next;
		}
		suffix_object(c, item);
	}
}

// string(key)/item(value) ��ӵ�object������
void cJSON_AddItemToObject(cJSON *object, const char *string, cJSON *item) {
	if (!item) {
		return;
	}
	// �ͷŵ�item�ľ�keyֵ
	if (item->string) {
		cJSON_frre(item->string);
	}
	item->string = cJSON_strdup(string);

	cJSON_AddItemToArray(object, item);
}

void cJSON_AddItemToObjectCS(cJSON *object, const char *string, cJSON *item) {
	if (!item) {
		return;
	}
	
	if (!(item->type & cJSON_StringIsConst) && item->string) {
		cJSON_free(item->string);
	}
	item->string = (char*)string;
	item->type |= cJSON_StringIsConst;
	cJSON_AddItemToArray(object, item);
}

void cJSON_AddItemReferenceToArray(cJSON *array, cJSON *item) {

}