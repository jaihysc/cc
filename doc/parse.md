# Parse

The parser utilizes recursive descent with 1 token lookahead to determine the production rule to apply. Parsed terminals and nonterminals are attached to form a parse tree, where the parser may choose to either backtrack or emit intermediate code. If the parser determines that a production rule applied was incorrect, it will backtrack on the parse tree to when the production rule was applied. If the parser completed certain production rules such as function-definitions or is at certain points in the program, the parser will generate intermediate code and clear the parse tree.

## Options

Pass these on the command line, if running using `cc.sh`, prefix with `-P` to indicate the option is for the parser, e.g., `-Pdprint-symtab`.

### Debug options

| Flag | Description |
|-|-|
| `-dprint-cg-recursion` | Shows the recursive walk through the parse tree while generating intermediate language |
| `-dprint-buffers` | Prints out parser's buffers while running |
| `-dprint-parse-recursion` | Shows the recursive matching of language productions as the input C source file is parsed |
| `-dprint-parse-tree` | Prints out the parse tree when it is about to be cleared |
| `-dprint-symtab` | Prints out the symbol table when it about to be cleared |

## Concepts

### Output symbols

The parser prepends `_Z<scope-number>` for all symbols within scopes other than the global (file) scope to identify shadowed symbols. Globals have no prefixes as constants are placed in global scope and passed directly to the assembler, thus they cannot have prefixes. Parser generated temporaries are of the form `_Z<scope-number>__t<temporary-number>`, labels `_Z<scope-number>__t<label-number>`.

### Output file formats (Unimplemented)

The parser generates multiple output files with prefix `imm2`, ending with `_<name>` depending on the data listed below:

- Function body (fun)
- Strings (str)

For example the file for all strings is `imm2_str`.

Using multiple files, it is easier for the assembly generator to parse `imm2`, as all the information the assembly generator needs when generating each section (.text, .rodata, ...) is provided in a file instead of having to parse through one large file.

Symbols:

- Symbols with prefix `__` are predefined, the following characters until the next underscore is its group. e.g., (`__str_1` is a predefined symbol with group str)
- Symbols are null terminated strings

## Processes

### Parse tree generation

The parser builds a parse tree in order to understand semantics for intermediate code generation.

Each function which parses a nonterminal attaches nodes onto the parent node if successful, indicating success via returning 1 if successful and 0 if not. The attached nodes may be additional nonterminal nodes or tokens. The parse tree is used to hold parsed nodes until sufficient information is available to generate intermediate code. The parse functions possesses the following signature:

```
int parse_<nonterminal-name>(Parser*, ParseNode*);
```

Each of the parse functions use the macros `PARSE_FUNC_START` and `PARSE_FUNC_END` to reduce duplicate code. `PARSE_FUNC_START` stores the current parse tree state to allow backtracking if the current production rule does not match. `PARSE_FUNC_END` performs backtracking if necessary and returns whether or not a match was made. `PARSE_MATCH` indicates a match was made in the current production rule. `PARSE_CURRENT_NODE` is a pointer to the node for the current nonterminal. `PARSE_TRIM_TREE` deletes the current current nonterminal and all its children.

The layout of each parse node is given below. `type` indicates the what terminal/nonterminal is stored at the node. If type is negative, it indicates the current node is a token and -type-1 yields the index into the token buffer for the token.

```
ParseNode
  ParseNode* child[];
  SymbolType type;
```

### Backtracking

Sometimes the matched production is later determined to be invalid, requiring backtracking to an earlier state.

Backtracking is performed by the two functions, `parser_get_parse_location` stores all the state information necessary to backtrack to this point later. `parser_set_parse_location` performs the backtracking.

```
ParseLocation parser_get_parse_location(Parser*, ParseNode*)
void parser_set_parse_location(Parser*, ParseLocation*)
```

Backtracking example:

```
Caret is lookahead, the production rules are given below:
function-definition -> declaration-specifiers declarator
     declaration-list(optional) compound-statement
declaration -> declaration-specifiers init-declarator-list(optional) ;

Assumed production is function-declaration:

int a = 1;
^ declaration-specifiers
int a = 1;
    ^ declarator
int a = 1;
      ^ Not matching production rule for function-declaration, backtrack here

Assumed production is declaration:

int a = 1;
^ declaration-specifiers
int a = 1;
    ^ init-declarator-list
int a = 1;
      ^ init-declarator-list
int a = 1;
        ^ init-declarator-list
int a = 1;
         ^ ;
```

### Value categories

The expression `*p` where p is a pointer takes on different meanings depending on where it appears in an expression. To correctly generate IL instructions based on how p is used, the value category of symbols are examined. When `*p` is used in the left hand side of an assignment, it notices `*p` is a Lvalue and generates IL to assign to a memory address. When `*p` is used in the right hand right hand side of an assignment, the assignment operator will first convert operands to a non Lvalue, during which IL will be generated to dereference p.

The rules for value category conversions are specified in the C standard, which are obeyed.

### Intermediate code generation

Intermediate code generation function expects a valid parse tree and a `ParseNode` with a symbol type matching the respective function, e.g., a node of type function-definition for a function generating function definition. Generation functions which return void emit intermediate code, generation functions which return `SymbolId` do not emit code, the returned `SymbolId` is the symbol holding the result of the identifier/expression/etc.

```
void cg_<nonterminal-name>(Parser*, ParseNode*)
SymbolId cg_<nonterminal-name>(Parser*, ParseNode*)
```

To add new symbols into the symbol table, a special function `cg_extract_symbol(Parser*, ParseNode*, ParseNode*)` is used as a symbol requires type and identifier which come from different nodes. This special function takes in both nodes and performs the necessary operations. The standard `cg_<nonterminal-name>` functions such as `cg_identifier` can be used for accessing symbols as they return the symbol's `SymbolId`.

Intermediate code is generated at the following locations. They are chosen as it has been determined that the parser will never need to backtrack to a point prior to the location, allowing for the parse tree to be cleared and intermediate code to be generated as it cannot change anymore.

- function-definition, prior to compound-statement
- block-item, when production complete
- selection-statement, rule 1, if ( expression ) `GENERATE` statement `GENERATE`
- selection-statement, rule 2, if ( expression ) `GENERATE` statement `GENERATE` else statement `GENERATE`
- iteration-statement, while, while ( expression `GENERATE` ) statement `GENERATE`
- iteration-statement, do while, do statement `GENERATE` while ( expression `GENERATE`) ;
- iteration-statement, for rule 2, for ( declaration `GENERATE` expression-2 `GENERATE` ; expression-3 ) statement `GENERATE`

selection-statement, iteration-statement must generate after expression as statement may generate code (otherwise the instructions are in the wrong order).

