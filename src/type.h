/* Represents a data type in the program */
#ifndef TYPE_H
#define TYPE_H

/* long needs to map to its own type to correctly calculate implicit
   conversions, hence the i32_ and u32_
   Same for long double and f64_ */
#define TYPE_SPECIFIERS  \
    TYPE_SPECIFIER(void) \
    TYPE_SPECIFIER(i8)   \
    TYPE_SPECIFIER(i16)  \
    TYPE_SPECIFIER(i32)  \
    TYPE_SPECIFIER(i32_) \
    TYPE_SPECIFIER(i64)  \
    TYPE_SPECIFIER(u8)   \
    TYPE_SPECIFIER(u16)  \
    TYPE_SPECIFIER(u32)  \
    TYPE_SPECIFIER(u32_) \
    TYPE_SPECIFIER(u64)  \
    TYPE_SPECIFIER(f32)  \
    TYPE_SPECIFIER(f64)  \
    TYPE_SPECIFIER(f64_)
#define TYPE_SPECIFIER(name__) ts_ ## name__,
/* ts_none for indicating error */
typedef enum {ts_none = -1, TYPE_SPECIFIERS ts_count} TypeSpecifiers;
#undef TYPE_SPECIFIER

/* Converts a type specifier to string */
const char* type_specifiers_str(TypeSpecifiers typespec);

typedef struct {
    /* 0 = Standard types, e.g., int, float, long long*
       1 = Function */
    int category;

    TypeSpecifiers typespec;
    int pointers;

    /* Arrays */
    int dimension; /* Dimension of array, e.g., [123][321] has dimension 2 */
    int size[1]; /* Number of elements for each dimension */
    /* FIXME assume dimension 1 arrays for now to get something implemented */
} Type;

/* Returned by relational, equality, logical and, logical or as required by
   6.5.8.6
   6.5.9.3
   6.5.13.3
   6.5.14.3 */
extern Type type_int;
/* Type for labels */
extern Type type_label;
/* Type for offsetting pointers */
extern Type type_ptroffset;
extern Type type_ptrdiff;

/* Constructs a type at given location */
void type_construct(Type* type, TypeSpecifiers ts, int pointers);

/* Constructs a function type at the given location */
void type_constructf(Type* type, const Type* ret_type);

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
Type type_point_to(const Type* type);

/* Returns the dimension of this type */
int type_dimension(const Type* type);

/* Adds a dimension to the type
   size: Number of elements the dimension can hold in total
         e.g., int[10][60] has count 600 */
void type_add_dimension(Type* type, int size);

/* Returns the number of elements held in the dimension at index i */
int type_dimension_size(const Type* type, int i);


/* Function types */


/* Returns 1 if given type is a function, 0 if not */
int type_is_function(const Type* type);

/* Returns the return type of given function type */
Type type_return(const Type* type);

/* Converts a string into a type */
Type type_from_str(const char* str);

/* Number of bytes this type takes up
   For arrays, the element count are part of the type's size */
int type_bytes(Type type);

/* Return 1 if both types are equal, 0 if not */
int type_equal(Type lhs, Type rhs);

/* Returns the integer conversion rank for a given integer type */
int type_rank(TypeSpecifiers typespec);

/* Returns 1 if provided type is an array type, 0 if not */
int type_array(const Type* type);

/* Returns the type of the array / pointer 's element */
Type type_element(const Type* type);

/* Converts an array of type to pointer to type
   e.g., int[] -> int*, int[2][10] -> int (*)[10] */
Type type_array_as_pointer(const Type* type);

/* Returns 1 if given type is a pointer type */
int type_is_pointer(const Type* type);

/* Returns 1 if given type is an arithmetic type */
int type_is_arithmetic(const Type* type);

/* Returns 1 if the provided type is an integer type, 0 if not */
int type_integral(Type type);

/* Returns 1 if the provided type is signed, 0 if not */
int type_signed(TypeSpecifiers typespec);

/* Returns 1 if the provided type is unsigned, 0 if not */
int type_unsigned(TypeSpecifiers typespec);

/* Returns 1 if the given signed type can represent to given unsigned type */
int type_signed_represent_unsigned(
        TypeSpecifiers sign, TypeSpecifiers unsign);

/* Returns the unsigned type for the given signed type */
TypeSpecifiers type_unsign_signed(TypeSpecifiers typespec);

/* Helper for type_common to calculate the common TypeSpecifiers */
TypeSpecifiers type_common_ts(
        TypeSpecifiers lhs, TypeSpecifiers rhs);

/* Applies the C 6.3.1.8 Usual arithmetic conversions to obtain a common real
   type for the operands and result */
Type type_common(Type type1, Type type2);

/* Applies the C 6.3.1.1 integer promotions on the provided type
   if applicable and returns the new type, or the old type if no promotion  */
Type type_promotion(Type type);

#endif
