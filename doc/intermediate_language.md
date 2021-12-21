# Intermediate language

Specification for the intermediate language used by the compiler

## Symbols

Symbols with prefix `__` are predefined, the following characters until the next underscore is its group. e.g., (`__str_1` is a predefined symbol with group str)

## Types

```
<fundamental-type><*(optional)>
```
\* denotes pointer, similar to C, chain them together for pointer to pointer, e.g., i8\*\*

| Type   | Summary                                                                                                                                            |
| ------ | -------------------------------------------------------------------------------------------------------------------------------------------------- |
| i8     | 8 bit integer                                                                                                                                      |
| i16    | 16 bit integer                                                                                                                                     |
| i32    | 32 bit integer                                                                                                                                     |

## Instructions

```
<il-instruction> <arg0>,<arg1>,<arg2>,...
<il-scope-instruction> <arg0>,<arg1>,<arg2>,... {
    <scope-contents> ...
}
```
- An instruction and its arguments is listed on one line
- If the instruction requires contents from its own scope, a { follows at the end of arguments with a } to end the scope
- Arguments are separated by commas **without** spaces
- Arguments are referenced starting from 0, i.e., arg0, arg1, arg2 ...

| Instruction | Args | Summary                                                                                                                                     |
| ----------- | ---- | ------------------------------------------------------------------------------------------------------------------------------------------- |
| #           |      | Comment, rest of line ignored                                                                                                               |
| mov         |    2 | Perform move from `<arg1>si` into `<arg0>s`                                                                                                 |
| func        |   2+ | Start function scope, name `<arg0>s`, return type `<arg1>t`, parameters `<arg2+>ts` type0 name0, type1 name1, ...                           |
| ret         |  0,1 | Return `<arg0>` from function (if required by function signature), otherwise just returns from function                                     |
| asm         |      | Start of assembly scope, contents is passed onto the assembler                                                                              |
| call        |  1+n | Call function with name `<arg0>s`, arguments for call `<arg1+>si`                                                                           |

- `0,1` Means 0 or 1
- `+` Means variable number of arguments
- `2+` Means 2 required arguments plus variable number afterwards

- `s` after `<arg#>` means symbol
- `i` after `<arg#>` means immediate (e.g., 0x55, 51, 0b11, 01123)
- `t` after `<arg#>` means type

### func

The signature `int main(int, char**)` is recognized as the entry point of the program

## Examples

```
# Return 5 from main, main's signature is a special function which is recognized as the entry point
func main,int,int,char** {
    ret 5
}
```

```
# Make a system call to print string and return
func print,void,const char* msg { <-- Declare print with 1 parameter
    asm {                           <-- Section of assembly
        mov rax, rdi                <-- Assembly block passed onto the assembler
    }                               <-- End section of assembly
    ret 0                           <-- Return 0
}                                   <-- End function declaration

func main,int,int,char** {   <-- Declare main with 2 parameters
    call print, __str_1         <-- Call function print with predefined symbol __str_1
    ret 0                       <-- Return 0
}                               <-- End function declaration
```

