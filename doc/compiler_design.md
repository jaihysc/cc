# Compiler Design

This document covers the details of the compiler design.

## Concepts

### Symbols

Symbols refer to any identifiers or constants in the program. Their token and type are stored in the symbol table. To resolve identifiers to symbols (references of variables to their symbols), the parser searches for the symbol starting from the current scope outwards. This means that a symbol can be accessed by its pointer for the duration of compilation, but can be only found by its token while in scope.

### Compilation stages

The compilation process is broken down into the following stages

```
Preprocessor -> Lexer -> Parser -> Assembly generator -> Assembler -> Linker
                ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
The underlined portions are implemented
```

The preprocessor is provided by gcc. Input c source file is read in and output imm1 is generated. The preprocessed source file imm1 is parsed and converted to the intermediate language, saved as imm2. The intermediate language imm2 is read in and x86-64 assembly in Intel syntax is generated, saved as imm3. The assembler used is NASM, intermediate output imm3 is read in and object file imm4 is generated. The linker used is ld, object file imm4 is read and the final executable is generated.

## Parse

The parser transforms tokens from the lexer into an Abstract Syntax Tree (AST) representation of the program.

### Abstract Syntax Tree generation

The parser utilizes recursive descent with 2 token lookahead to determine the production rule to apply. After the application of each rule, it attaches a `TNode` (Tree Node) onto the `Tree` (Abstract Syntax Tree) if necessary. Each `TNode` holds data depending on the type assigned to the node. As an example, a `TNodePostfixExpression` holds an enum on the type of operator to apply to its children.

The parser may generate redundant `TNode` as a result of limitations with two token lookahead. The parser trims the tree of redundant nodes occasionally at certain points in the parsing sequence, such as after parsing an expression.

### Parse functions

Functions for parsing are of the signature

```c
static ErrorCode parse_<nonterminal-name>(Parser* p, TNode* parent, int* matched)
```

`TNode` are attached onto the parent node, and `matched` is used to indicate the production rule was applied successfully. Each parse function begins with the macro `PARSE_FUNC_START` and ends with `PARSE_FUNC_END`. These macros are used to print out the parse functions for debugging.

## Intermediate Language 2 generator

The intermediate language 2 generator converts the Abstract Syntax Tree (AST), also known as intermediate language 1 to intermediate language 2. Intermediate language 2 consists of three address code, where an instruction may have at most three arguments.

### Translation process

Code generation functions expect a valid tree, traversing the tree to convert the nodes into IL2. IL2 is initially all stored in a single basic block in the control flow graph. Once code generation is complete, the control flow graph is processed, splitting the initial block into sub-blocks which reflects the program control flow.

### Loop control break, continue

Break and continue statements generates a jump to the end of the loop or the end of the loop body respectively. Loops are tracked in a stack, meaning the jump destination of the break and continue is the most recent loop. The stack is stored in the symbol table under symbol categories, i.e., `symtab_push_cat` and `symtab_pop_cat`.

### Value categories

The expression `*p` where p is a pointer takes on different meanings depending on where it appears in an expression. To correctly generate IL instructions based on how p is used, the value category of symbols are examined. When `*p` is used in the left hand side of an assignment, it notices `*p` is a Lvalue and generates IL to assign to a memory address. When `*p` is used in the right hand right hand side of an assignment, the assignment operator will first convert operands to a non Lvalue, during which IL will be generated to dereference p.

The rules for value category conversions are specified in the C standard, which are obeyed.
