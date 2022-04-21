# Parse

Implementation details about the parser

## Expression tree

Expressions are expressed in a tree structure:

- End nodes are values
- Connecting nodes are operators (+, -, ==, !=, etc)

Evaluation is performed by level, starting at the highest level (bottom of tree). e.g., starting at level 2, code is generated to evaluate level 2 nodes, then code generated to evaluate level 1 nodes, then level 0 nodes.

Order of operations is handled by remembering the last operator, generating nodes by comparing the current operator precedence with the last operator's precedence, for example:

```
Parser scans left to right.

a / b + c * d
  ^ Scanned operator (do nothing until next operator seen)

a / b + c * d
      ^ Scanned operator (last operator is /)

Since division has higher precedence than addition, the node a / b is generated.
 /[/]\
[a] [b]

a / b + c * d
          ^ Scanned operator (last operator is +)

Since multiplication has higher precedence than addition, the node c * d is generated, then a node for the last operator.
    /[+] \
   /      \
 /[/]\  /[*]\
[a] [b][c] [d]
```


Tree example:

```c
(a / b + c) * (d - e)
```

```
Level 0         /[*] \
               /      \
Level 1      /[+]\  /[-]\
            /   [c][d] [e]
Level 2   /[/]\
         [a] [b]
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

