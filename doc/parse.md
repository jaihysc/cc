# Parse

Implementation details about the parser

## Expression tree

Expressions are converted into RPN using the Shunting yard algorithm, which is converted into a tree.

```
RPN to tree conversion example:
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

