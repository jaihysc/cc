# Intermediate language

Specification for the intermediate language used by the compiler

## Symbols

The parser will generate additional symbols with prefix `__`, the following characters until the next underscore is its group, the number starts from 0. e.g., (`__str_1` is a symbol with group str)

| Group  | Summary                                                                                                                                            |
| ------ | -------------------------------------------------------------------------------------------------------------------------------------------------- |
| str    | String                                                                                                                                             |
| local  | Local variables created during intermediate calculations                                                                                           |

## Types

```
<fundamental-type><*(optional)><[array-size](optional)>
```

Types are in a format similiar to C:

- `*` denotes pointer, chain them together for pointer to pointer, e.g., `i8**`
- `[]` denotes array, e.g., `i32*[10][17]`

| Type   | Summary                                                                                                                                            |
| ------ | -------------------------------------------------------------------------------------------------------------------------------------------------- |
| void   | Void                                                                                                                                               |
| i8     | 8 bit integer                                                                                                                                      |
| i16    | 16 bit integer                                                                                                                                     |
| i32    | 32 bit integer                                                                                                                                     |
| i32_   | 32 bit integer                                                                                                                                     |
| i64    | 64 bit integer                                                                                                                                     |
| u8     | 8 bit unsigned integer                                                                                                                             |
| u16    | 16 bit unsigned integer                                                                                                                            |
| u32    | 32 bit unsigned integer                                                                                                                            |
| u64    | 64 bit unsigned integer                                                                                                                            |
| f32    | 32 bit floating-point                                                                                                                              |
| f64    | 64 bit floating-point                                                                                                                              |
| f64_   | 64 bit floating-point                                                                                                                              |

## Instructions

```
<il-instruction> <arg0>,<arg1>,<arg2>,...
```
- An instruction and its arguments is listed on one line
- An instruction and its arguments is separated by one space
- Arguments are separated by commas without spaces
- Arguments are referenced starting from 0, i.e., arg0, arg1, arg2 ...

Program format instructions
| Instruction | Args | Summary                                                                                                                                     |
| ----------- | ---- | ------------------------------------------------------------------------------------------------------------------------------------------- |
| #           |      | Comment, rest of line ignored                                                                                                               |
| func        |   2+ | Start function scope, name `<arg0>s`, return type `<arg1>t`, parameters `<arg2+>ts` type0 name0, type1 name1, ...                           |
| def         |    1 | Defines a new symbol with `<arg0>ts` type name                                                                                              |
| asm         |      | Contents after `asm ` until newline is passed onto the assembler                                                                            |

Control flow instructions
| Instruction | Args | Summary                                                                                                                                     |
| ----------- | ---- | ------------------------------------------------------------------------------------------------------------------------------------------- |
| ret         |  0,1 | Return `<arg0>s` from function if given, otherwise returns void                                                                             |
| call        |  1+n | Call function with name `<arg0>s`, return value placed in `<arg1>s` or left as void if no return, arguments for call `<arg1+>si`            |
| lab         |    1 | Create label with name `<arg0>`                                                                                                             |
| jmp         |    1 | jump to `<arg0>l`                                                                                                                           |
| jz          |    2 | jump to `<arg0>l` if `<arg1>si` == 0                                                                                                        |
| jnz         |    2 | jump to `<arg0>l` if `<arg1>si` != 0                                                                                                        |

Arithmetic and logical instructions
| Instruction | Args | Summary                                                                                                                                     |
| ----------- | ---- | ------------------------------------------------------------------------------------------------------------------------------------------- |
| add         |    3 | `<arg0>s` = `<arg1>si` + `<arg2>si`                                                                                                         |
| sub         |    3 | `<arg0>s` = `<arg1>si` - `<arg2>si`                                                                                                         |
| mul         |    3 | `<arg0>s` = `<arg1>si` * `<arg2>si`                                                                                                         |
| div         |    3 | `<arg0>s` = `<arg1>si` / `<arg2>si`                                                                                                         |
| mod         |    3 | `<arg0>s` = `<arg1>si` % `<arg2>si`                                                                                                         |
| not         |    2 | `<arg0>s` = !`<arg1>si`                                                                                                                     |
| cl          |    3 | `<arg0>l` = `<arg1>si` \< `<arg2>si`                                                                                                        |
| cle         |    3 | `<arg0>l` = `<arg1>si` \<= `<arg2>si`                                                                                                       |
| ce          |    3 | `<arg0>l` = `<arg1>si` == `<arg2>si`                                                                                                        |
| cne         |    3 | `<arg0>l` = `<arg1>si` != `<arg2>si`                                                                                                        |

Memory manipulation instructions
| Instruction | Args | Summary                                                                                                                                     |
| ----------- | ---- | ------------------------------------------------------------------------------------------------------------------------------------------- |
| mov         |    2 | `<arg0>s` = `<arg1>si`                                                                                                                      |
| mtc         |    2 | `<arg0>s` = (T)`<arg1>si` where T is the type of `<arg0>`, `<arg1>` is cast to T (Mov Type Cast)                                            |
| mad         |    2 | `<arg0>s` = &`<arg1>s` (Mov ADdress)                                                                                                        |
| mfi         |    2 | `<arg0>s` = `<arg1>si`\[`<arg2>si`\] where the index is in bytes (Mov From Index)                                                           |
| mti         |    2 | `<arg0>s`\[`<arg1>si`\] = `<arg2>si` where the index is in bytes (Mov To Index)                                                             |

- `0,1` Means 0 or 1
- `+` Means variable number of arguments
- `2+` Means 2 required arguments plus variable number afterwards

- `s` after `<arg#>` means symbol
- `i` after `<arg#>` means immediate (e.g., 0x55, 51, 0b11, 01123)
- `t` after `<arg#>` means type
- `l` after `<arg#>` means label

### func

The signature `i32 main(i32, i8**)` is recognized as the entry point of the program.

The signature `void _Global()` is recognized to perform global initialization.

## Examples

```
# The c snippet: int z = a + b / c - d / e;
def i32 z
def i32 a
def i32 b
def i32 c
def i32 d
def i32 e
def i32 __local_1
def i32 __local_2
def i32 __local_3

div __local_1,b,c
div __local_2,d,e
add __local_3_,a,__local_1,
sub z,__local_3,__local_2
```

```
# Return 5 from main, main's signature is a special function which is recognized as the entry point
func main,i32,i32,i8**
ret 5
```

```
# Make a system call to print string and return
func print,void,i8* msg      Declare print with 1 parameter
asm mov rax, rdi             Assembly passed onto the assembler
ret                          Return

func main,i32,i32,i8**       Declare main with 2 parameters
call print,void,__str_1      Call function print with predefined symbol __str_1, no return value
ret 0                        Return 0
```

