# Parse

Implementation details about the parser.

The parser utilizes recursive descent with 1 token lookahead to determine the production rule to apply. Parsed terminals nonterminals are attached to form a parse tree, where the parser may choose to either backtrack or emit intermediate code. If the parser determines that a production rule applied was incorrect, it will backtrack on the parse tree to when the production rule was applied. If the parser completed certain production rules such as function-definitions, the parser will generate intermediate code and clear the parse tree.

## Tokens

Tokens are stored in a token buffer alongside its attributes, the buffer used changes depending on scope. A token buffers points to sub-buffers to allow multiple sub-scopes within a scope. When adding a token into the buffer it checks if the token already exists, if a token exists the index of the existing token is returned, otherwise the token is added and its index is returned. Keywords are special tokens which are not added into the buffer.

TODO lookahead to separate tokens such as > and >=.

The token's structure is as follows. Lexeme is a null terminated c string.

```
token
  char lexeme[];
```

## Backtracking

Backtracking is performed by the two functions, `parser_get_parse_location` stores all the state information necessary to backtrack to this point later. `parser_set_parse_location` performs the backtracking.

```
parse_location parser_get_parse_location(parser*, parse_node*)
void parser_set_parse_location(pareser*, parse_location*)
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

## Parse tree

For each function which parses a nonterminal, it will attach the parsed nonterminal onto the given parent node if successful, indicating success via returning 1 if successful and 0 if not. They possesses the following signature:

```
int parse_<nonterminal-name>(parser*, parse_node*);
```

Each of the parse functions use the macros `PARSE_FUNC_START` and `PARSE_FUNC_END` to reduce duplicate code. `PARSE_FUNC_START` stores the current parse tree state to allow backtracking if the current production rule does not match. `PARSE_FUNC_END` performs backtracking if necessary and returns whether or not a match was made. `PARSE_MATCH` indicates a match was made in the current production rule. `PARSE_CURRENT_NODE` is a pointer to the node for the current nonterminal.

The nodes of the parse tree are linked and stored in a buffer, the buffer grows as nodes are appended to the parse tree. To discard nodes, the nodes to be kept are unlinked from the nodes to be discarded and the integer indicating the end of the buffer is subtracted.

The layout of each parse node (for a nonterminal) is given below. `type` indicates the what terminal/nonterminal is stored at the node. If type is negative, it indicates the current node is a token and -type-1 yields the index into the token buffer for the token.

```
parse_node
  parse_node* child[];
  symbol_type type;
```

## Intermediate code generation

Intermediate code generation function expects a valid parse tree and a `parse_node` with a symbol type matching the respective function, e.g., a node of type function-definition for a function generating function definition. Generation functions which return void emit intermediate code, generation functions which return `token_id` do not emit code, the returned `token_id` is the token holding the result of the identifier/expression/etc.

```
void cg_<nonterminal-name>(parser*, parse_node*)
token_id cg_<nonterminal-name>(parser*, parse_node*)
```

During generation, lexemes are added to the symbol table alongside its attributes to form a token, indexed via a `token_id`. A token's attributes are as follows:

```
Token attributes
  Index of lexeme in parse token buffer
  Type
```

To add new symbols into the symbol table, a special function `cg_extract_symbol(parser*, parse_node*, parse_node*)` is used as a symbol requires type and identifier which come from different nodes. This special function takes in both nodes and performs the necessary operations. The standard `cg_<nonterminal-name>` functions such as `cg_identifier` can be used for accessing symbols as they return the symbol's `token_id`.

## Output file formats

The parser generates multiple output files with prefix `imm2`, ending with `_<name>` depending on the data listed below:

- Function body (fun)
- Strings (str)

For example the file for all strings is `imm2_str`.

Using multiple files, it is easier for the assembly generator to parse `imm2`, as all the information the assembly generator needs when generating each section (.text, .rodata, ...) is provided in a file instead of having to parse through one large file.

Symbols:

- Symbols with prefix `__` are predefined, the following characters until the next underscore is its group. e.g., (`__str_1` is a predefined symbol with group str)
- Symbols are null terminated strings

### Strings

