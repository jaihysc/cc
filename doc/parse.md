# Parse

Implementation details about the parser

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

