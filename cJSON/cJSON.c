#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <float.h>
#include <limits.h>
#include <ctype.h>
#include "cJSON.h"

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
static const char *ep;

const char *cJSON_GetErrorPtr(void) {
	return ep;
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
		memset(node, 0, sizeof(node));
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