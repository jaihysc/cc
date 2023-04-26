/* Represents a data type in the program */
#ifndef TYPE_H
#define TYPE_H

#include "errorcode.h"

/* Maximum length of declaration specifier string,
   no null terminator */
#define TS_STR_MAX_LEN 22

/* char is signed
   schar = signed char
   uchar = unsigned char */
#define TYPE_SPECIFIERS       \
	TYPE_SPECIFIER(void)      \
	TYPE_SPECIFIER(char)      \
	TYPE_SPECIFIER(schar)     \
	TYPE_SPECIFIER(uchar)     \
	TYPE_SPECIFIER(short)     \
	TYPE_SPECIFIER(ushort)    \
	TYPE_SPECIFIER(int)       \
	TYPE_SPECIFIER(uint)      \
	TYPE_SPECIFIER(long)      \
	TYPE_SPECIFIER(ulong)     \
	TYPE_SPECIFIER(longlong)  \
	TYPE_SPECIFIER(ulonglong) \
	TYPE_SPECIFIER(float)     \
	TYPE_SPECIFIER(double)    \
	TYPE_SPECIFIER(ldouble)
#define TYPE_SPECIFIER(name__) ts_##name__,
/* ts_none for indicating error */
typedef enum
{
	ts_none = -1,
	TYPE_SPECIFIERS ts_count
} TypeSpecifiers;
#undef TYPE_SPECIFIER

/* Converts a type specifier to string */
const char* ts_str(TypeSpecifiers typespec);

/* Converts string to type specifier
   ts_none if invalid */
TypeSpecifiers ts_from_str(const char* str);

typedef struct Type
{
	enum
	{
		TypeCategory_standard,
		TypeCategory_function,
	} category;

	union
	{
		struct
		{
			TypeSpecifiers typespec;
		} standard;
		struct
		{
			struct Type* return_type;
		} function;
	} data;
	int pointers;

	/* Arrays */
	int dimension; /* Dimension of array, e.g., [123][321] has dimension 2 */
	int size[1];   /* Number of elements for each dimension */
				   /* FIXME assume dimension 1 arrays for now to get something implemented */
} Type;

/* Constructs a type at given location */
ErrorCode type_construct(Type* type, TypeSpecifiers ts, int pointers);

/* Constructs a function type at the given location */
ErrorCode type_constructf(Type* type, const Type* ret_type, int pointers);

void type_destruct(Type* type);

/* Copys provided type into destination */
ErrorCode type_copy(const Type* type, Type* dest);

/* Returns type specifiers for Type */
TypeSpecifiers type_typespec(const Type* type);

/* Sets type specifiers for Type */
void type_set_typespec(Type* type, TypeSpecifiers typespec);

/* Returns number of pointers for Type */
int type_pointer(const Type* type);

/* Sets number of pointers for Type */
void type_set_pointer(Type* type, int pointer);

/* Increments the number of pointers for Type, returns the new number of
   pointers */
int type_inc_pointer(Type* type);

/* Sets the current type to the type obtained from dereferencing the current
   type (thus decreasing one level of indirection) */
void type_dec_indirection(Type* type);

/* Returns Type of what this pointer points to */
ErrorCode type_point_to(const Type* type, Type* dest);

/* Returns the dimension of this type */
int type_dimension(const Type* type);

/* Adds a dimension to the type
   size: Number of elements the dimension can hold in total
		 e.g., int[10][60] has count 600 */
void type_add_dimension(Type* type, int size);

/* Returns the number of elements held in the dimension at index i */
int type_dimension_size(const Type* type, int i);

/* Returns 1 if given type is a standard type, 0 if not */
int type_is_standard(const Type* type);

/* Returns 1 if given type is a function, 0 if not */
int type_is_function(const Type* type);

/* Returns the return type of given function type */
Type* type_return(Type* type);

/* Number of bytes this type takes up
   For arrays, the element count are part of the type's size */
int type_bytes(const Type* type);

/* Return 1 if both types are equal, 0 if not */
int type_equal(const Type* lhs, const Type* rhs);

/* Returns the integer conversion rank for a given integer type */
int type_rank(TypeSpecifiers typespec);

/* Returns 1 if provided type is an array type, 0 if not */
int type_array(const Type* type);

/* Returns the type of the array / pointer 's element */
ErrorCode type_element(const Type* type, Type* dest);

/* Converts an array of type to pointer to type
   e.g., int[] -> int*, int[2][10] -> int (*)[10] */
ErrorCode type_array_as_pointer(const Type* type, Type* dest);

/* Returns 1 if given type is a pointer type */
int type_is_pointer(const Type* type);

/* Returns 1 if given type is an arithmetic type */
int type_is_arithmetic(const Type* type);

/* Returns 1 if the provided type is an integer type, 0 if not */
int type_integral(const Type* type);

/* Returns 1 if the provided type is signed, 0 if not */
int type_signed(TypeSpecifiers typespec);

/* Returns 1 if the provided type is unsigned, 0 if not */
int type_unsigned(TypeSpecifiers typespec);

/* Returns 1 if the given signed type can represent to given unsigned type */
int type_signed_represent_unsigned(TypeSpecifiers sign, TypeSpecifiers unsign);

/* Returns the unsigned type for the given signed type */
TypeSpecifiers type_unsign_signed(TypeSpecifiers typespec);

/* Helper for type_common to calculate the common TypeSpecifiers */
TypeSpecifiers type_common_ts(TypeSpecifiers lhs, TypeSpecifiers rhs);

/* Applies the C 6.3.1.8 Usual arithmetic conversions to obtain a common real
   type for the operands and result */
ErrorCode type_common(const Type* type1, const Type* type2, Type* dest);

/* Applies the C 6.3.1.1 integer promotions on the provided type
   if applicable and returns the new type, or the old type if no promotion  */
ErrorCode type_promotion(const Type* type, Type* dest);

#endif
