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

Backtracking is performed by the two functions, `get_parse_location` stores all the state information necessary to backtrack to this point later. `set_parse_location` performs the backtracking.

```
parse_location get_parse_location()
void set_parse_location(parse_location*)
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

The layout of each parse node (for a nonterminal) is given below. `type` indicates the what terminal/nonterminal is stored at the node. If type is negative, it indicates the current node is a token and -(type+1) yields the index into the token buffer for the token.

```
parse_node
  parse_node* child[];
  symbol_type type;
```

## Expression tree

Expression precedence is handled by converting them to postfix notation using the Shunting yard algorithm, which is converted into a tree.

```
Postfix to tree conversion example:
Original expression: a + b * c - d / e

Output buffer:
a b c * d e / - +

a p_bc d e / - +     p_bc      references the tree node b * c
a p_bc p_de - +      p_de      references the tree node d / e
a p_bc_de +          p_bc_de   references the tree node p_bc - p_de, which is: (b * c) - (d / e)
p_a_bc_de            p_a_bc_de references the tree node a + p_bc_de, which is: a + ((b * c) - (d / e))
```

The output buffer for holding the operands is an array of integers. Integers \>= 0 are indices into the token buffer, negative integers are indices into the tree node buffer after negating them and subtracting 1.

```
Buffer example:
Number in output buffer corresponds to labelled element in buffer

Output buffer:
0 -1 5 -2 -3

Token buffer:
v a l 1 \0 v a l 2 \0
^          ^
0          5

Tree node buffer:
node1 node2 node3
^     ^     ^
-1    -2    -3
```

The tree node structure is as follows, if left/right is pointing to a node, the left/right node is stored in the tree node buffer. If pointing to a token, the token is stored in the token buffer.

```
Tree node structure
  left
  right
  operator_id
  flags        Indicate whether left and right is a pointer to node, pointer to token, etc
```

```
Tree example:
(a / b + c) * (d - e)

*
├ +
| ├ /
| | ├ a
| | └ b
| └ c
└ -
  ├ d
  └ e
```

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

