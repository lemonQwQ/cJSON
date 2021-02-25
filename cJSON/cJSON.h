#ifndef cJSON__h
#define cJSON__h
// 判断是否是以c++的标准
#ifdef __cplusplus
extern "C" //去寻找C标准的函数
{
#endif

#define cJSON_False  (1 << 0)
#define cJSON_True   (1 << 1)
#define cJSON_NULL   (1 << 2)
#define cJSON_Number (1 << 3)
#define cJSON_String (1 << 4)
// #define cJSON_Array  (1 << 5)
// #define cJSON_Object (1 << 6)

#define cJSON_IsReference 256
#define cJSON_StringIsConst 512

	typedef struct cJSON
	{
		// 双向链表
		struct cJSON *next;
		struct cJSON *prev;
		
		// 子结点
		struct cJSON *child;

		// 对象类型： False True NULL Number String Array Object
		int type;

		// 存储数据
		char *valuestring;
		int valueint;
		double valuedouble;

		// key键
		char *string;
	} cJSON;
	typedef struct cJSON_Hooks
	{
		void *(*malloc_fn)(size_t sz);
		void(*free_fn)(void *ptr);
	} cJSON_Hooks;

	extern const char *cJSON_GetErrorPtr(void);

	extern void cJSON_InitHooks(cJSON_Hooks* hooks);

	extern void cJSON_Delete(cJSON *c);

	extern cJSON *cJSON_ParseWithOpts(const char *value, const char **return_parse_end, int require_null_terminated);

	extern cJSON *cJSON_Parse(const char *value);

	extern char  *cJSON_Print(const cJSON *item);

	extern char  *cJSON_PrintUnformatted(const cJSON *item);

	extern char *cJSON_PrintBuffered(const cJSON *item, int prebuffer, int fmt);

	extern int	  cJSON_GetArraySize(const cJSON *array);

	extern cJSON *cJSON_GetArrayItem(const cJSON *array, int item);

	extern cJSON *cJSON_GetObjectItem(const cJSON *object, const char *string);
	extern int cJSON_HasObjectItem(const cJSON *object, const char *string);

//	extern cJSON *cJSON_CreateNull(void);
//	extern cJSON *cJSON_CreateTrue(void);
//	extern cJSON *cJSON_CreateFalse(void);
//	extern cJSON *cJSON_CreateBool(int b);
//	extern cJSON *cJSON_CreateNumber(double num);
//	extern cJSON *cJSON_CreateString(const char *string);
//	extern cJSON *cJSON_CreateArray(void);
//	extern cJSON *cJSON_CreateObject(void);
//
//	extern cJSON *cJSON_CreateIntArray(const int *numbers, int count);
//	extern cJSON *cJSON_CreateFloatArray(const float *numbers, int count);
//	extern cJSON *cJSON_CreateDoubleArray(const double *numbers, int count);
//	extern cJSON *cJSON_CreateStringArray(const char **strings, int count);
//
//	extern void cJSON_AddItemToArray(cJSON *array, cJSON *item);
//	extern void	cJSON_AddItemToObject(cJSON *object, const char *string, cJSON *item);
//	extern void	cJSON_AddItemToObjectCS(cJSON *object, const char *string, cJSON *item);	/* Use this when string is definitely const (i.e. a literal, or as good as), and will definitely survive the cJSON object */
//
//	extern void cJSON_AddItemReferenceToArray(cJSON *array, cJSON *item);
//	extern void	cJSON_AddItemReferenceToObject(cJSON *object, const char *string, cJSON *item);
//
//	extern cJSON *cJSON_DetachItemFromArray(cJSON *array, int which);
//	extern void   cJSON_DeleteItemFromArray(cJSON *array, int which);
//	extern cJSON *cJSON_DetachItemFromObject(cJSON *object, const char *string);
//	extern void   cJSON_DeleteItemFromObject(cJSON *object, const char *string);
//
//	extern void cJSON_InsertItemInArray(cJSON *array, int which, cJSON *newitem); /* Shifts pre-existing items to the right. */
//	extern void cJSON_ReplaceItemInArray(cJSON *array, int which, cJSON *newitem);
//	extern void cJSON_ReplaceItemInObject(cJSON *object, const char *string, cJSON *newitem);
//
//	extern cJSON *cJSON_Duplicate(const cJSON *item, int recurse);

//	extern void cJSON_Minify(char *json);
//
//#define cJSON_AddNullToObject(object,name) cJSON_AddItemToObject(object, name, cJSON_CreateNull())
//#define cJSON_AddTrueToObject(object,name) cJSON_AddItemToObject(object, name, cJSON_CreateTrue())
//#define cJSON_AddFalseToObject(object,name) cJSON_AddItemToObject(object, name, cJSON_CreateFalse())
//#define cJSON_AddBoolToObject(object,name,b) cJSON_AddItemToObject(object, name, cJSON_CreateBool(b))
//#define cJSON_AddNumberToObject(object,name,n) cJSON_AddItemToObject(object, name, cJSON_CreateNumber(n))
//#define cJSON_AddStringToObject(object,name,s) cJSON_AddItemToObject(object, name, cJSON_CreateString(s))
//
//#define cJSON_SetIntValue(object,val) ((object) ? (object)->valueint = (object)->valuedouble = (val) : (val))
//#define cJSON_SetNumberValue(object,val) ((object) ? (object)->valueint = (object)->valuedouble = (val) : (val))
//
//#define cJSON_ArrayForEach(pos, head) for(pos = (head)->child; pos != NULL; pos = pos->next)

#ifdef __cplusplus
}
#endif

#endif