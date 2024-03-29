# Assembly generator

The assembly generator converts the intermediate language (IL) into x86 assembly, the various stages are listed below:

```
                          ---------------
    Register preferences  | Register    |
           -------------> | Allocator   |
           |              ---------------
           |                     |
           |                     | Register allocations
           |                     v
    ---------------       ---------------       -------------
IL  | Instruction | PAsm  | Instruction | PAsm  | Code      | Asm
--> | Selector    | ----> | Selector 2  | ----> | Generator | --->
    ---------------       ---------------       -------------

IL: Intermediate Language
PAsm: Pseudo-assembly
Asm: Assembly
```

Planned
- Optimizer after the instruction selector
- Optimizer after the code generator

## Options

Pass these on the command line, if running from `cc.sh`, prefix with `-S` to indicate the option is for the assembly generator, e.g., `-Sdprint-symtab`.

### Debug options

| Flag | Description |
|-|-|
| `-dprint-cfg` | Prints out the control flow graph for each function |
| `-dprint-ig` | Prints out the interference graph used for register allocation for each function |
| `-dprint-info` | Prints out debug information while running |
| `-dprint-symtab` | Prints out the symbol table after assembly generation for each function |

## Concepts

These concepts are referenced in this document.

### Symbols

Symbols are stored alongside its attributes (token, type, ...) in the symbol table (symtab for short). The symbol table is function scope, all variables are present from the beginning to the end of the function to avoid having to handle scoping rules.

To reduce extra code which must be written to handle constants, the symbol table has special handling for constants. Constants can be added to symbol tables multiple times, looking up a constant in a symbol table will add the constant to the table if it does not exist and return the symbol, or if it does exist, return the existing symbol. This allows existing code for working with symbols such as `symbol_type` and `symbol_bytes` to be reused for constants as well.

### Program graph

The program graph is currently represented as a control flow graph, in the future it can be a program dependency graph (PDG) or static single assignment (SSA) graph to aid instruction scheduling and optimizations. It contains data and control flow information together, with the goal of giving the instruction selector a broader view of the program under compilation to generate better assembly.

Blocks in the control flow graph hold IL instructions. In the future a graph form can be chosen to reduce various arrangements of IL instructions into a single form, making it easier for the instruction selector to recognize patterns.

The control flow graph is built in 2 phases. The first phase creates the blocks plus its contents and links blocks to blocks immediately following it, the second phase links jumps from blocks to labels in other blocks.

### Register preferences

The goal of register preferences is to improve assembly quality by choosing optimal registers such that the register benefits the current variable with better generated assembly, while also avoiding disadvantaging others by worsening their generated assembly. For example `idiv` prefers others not use `eax` and `edx` as it is needed for the instruction, otherwise it needs to save and restore those registers which leads to extra operations.

To quantify preferences, each variable holds preference scores for each register, positive scores means the register is preferred, and a negative score means the register is avoided. On the `idiv` example, the statement has a negative register preference score on eax and edx.

### Pseudo-assembly

Pseudo-assembly (pasm) is a low level IR which closely resembles assembly of the target machine. Its difference from assembly is its two addressing modes which is a symbol, and dereferencing of a symbol. Avoiding encoding addressing modes such as [base+index*scale+displacement] greatly simplifies the logic used to analyze and manipulate pasm and eases retargeting of the compiler. Instead, specific addressing modes are implemented as separate instructions, for example `a = b[c]` where b is an array in assembly is:

```asm
mov rax, [rbx+rcx]
```

Which is expressed in pasm as:

```asm
mov_so %a, %b, %c
```

Each variable is written `%name` and referred to as virtual registers. In `mov_so`, the `s` stands for symbol, interpreting `%a` as a symbol and `o` stands for offset, interpreting `%b as a base and %c` as an offset.

On x86, do not confuse the % for AT&T syntax, pseudo-assembly on x86 is in Intel syntax and the % here represents a virtual register for the variable or a memory location. For example, targeting x86, the following pseudo-assembly may be generated from the intermediate language:

```
Intermediate language

mov a,  argc
div t0, argc, argc
mul t1, t0,   argc
sub t2, t1,   1
ret
```

```asm
Pseudo-assembly

mov  %a,   %argc
push eax
push edx
xor  edx,  edx
mov  eax,  %argc
idiv %argv
mov  %t0,  eax
pop  edx
pop  eax
mov  %t1,  %t0
imul %t1,  %argc
mov  %t2,  %t1
sub  %t2,  1
mov  eax,  %t2
ret
```

### x86 assembly instruction expressed as pseudo-assembly

Each register is given its own operand, some examples:

```asm
; Assembly above, pseudo-assembly below
mov rax, [rbx+rcx+3]
mov_so %a, %b, %c ; a is rax, b is at [rbx+3], c is rcx
                  ; A reminder that s stands for symbol, o stands for offset, i.e., b offset c
mov rax, rbx
mov_ss %a, %b ; a is rax, b is rbx

mov [rbp-30], rbx
mov_ss %a, %b ; a is at [rbp-30], b is rbx

mov [rax+rbx+1], rcx
mov_os %a, %b, %c ; a is at [rax+1], b is rbx, c is rcx
```

## Instruction Selector

The instruction selector uses macro expansion of the program graph to select the optimal pseudo-assembly. Each pattern contains information on cost and pseudo-assembly replacement. Cost guides the search for the optimal pattern, pseudo-assembly replacement maps the IL instruction(s) to pseudo-assembly and creates additional temporaries where necessary. Different patterns exist to cover different cases of how the operands are stored, i.e., in register, in memory, is immediate. See below for an example.

To prepare for this stage, the assembly generator makes a pass through the intermediate language to load the symbol table and generate a program graph.

More complex optimizations which are not possible with macro expansion such as sub-expression elimination are the job of the optimizer.

Possible ideas to explore in the future:
- Ershov numbers

Example for creation of additional temporaries:

```
Intermediate language

cl b, 2, a
```

```asm
Pseudo-assembly

mov  %t0, 2
cmp  %t0, %a  ; cmp instruction requires a register for operand 1
setl %b
```

## Instruction Selector 2

Certain instructions require register allocations to be converted to assembly, an example is the IL `call` instruction, which requires certain registers be caller saved, and arguments copied into appropriate registers as required by the calling convention. Instruction selector 2 runs after register allocations are complete, replacing special pseudo-assembly instructions with the appropriate pseudo-assembly. For example:

```
Intermediate language

call z,f,a,b,c,d
     ~ ~ ~~~~~~~
     ^ ^ Arguments
     ^ Function name
     Returned value
```

```asm
Pseudo-assembly after instruction selector

call_param %a   | Special instructions recognized by instruction selector 2
call_param %b   |
call_param %c   |
call_param %d   |
call %f
call_cleanup %z |
```

```asm
Pseudo-assembly after instruction selector 2

mov %rdi, %a
mov %rsi, %b
mov %rdx, %c
mov %rcx, %d
call %f
mov %z, %rax

Where %rdi, %rsi, %rdx, %rcx, %rax are symbols assigned the register as indicated in its name.
```

The special pseudo-assembly instructions is used for liveness analysis when preparing for register allocations, looking only at call %f, it cannot determine what the arguments are which must be kept alive, or the definition of the function call's result which may allow a %z defined earlier to have its lifetime terminated.

## Register Allocator

The register allocator is similar to a Chaitin-style allocator with some modifications, the following numbered subsections correspond to a phase in the register allocator.

Spill code generation is not part of the register allocator and is generated by the code generator where necessary.

### 1. Renumber

Each block in the control flow graph is walked backwards to generate liveness use/def information for a block. Then, the control flow graph is walked backwards from the end to generate live range information per block.

### 2. Build

Using liveness information, an interference graph is built for all the variables. The graph is built by scanning statements backwards from the end of each block and joining two nodes (each node representing a variable) with an edge if both variables are live at the same time. This is also done for the live variables exiting the block.

### 3. Precolor

Variables whose address gets taken, arrays are automatically spilled.

### 4. Coalesce

Attempts to eliminate copy instructions by performing aggressive coalescing.

### 5. Register preference

Each block in the control flow graph is walked to calculate register preference scores. Currently, preference scores are calculated to eliminate register save + restores. Register save + restores are identified by the ranges formed by instructions which have the behavior of pushing and popping from the stack, the live variables in between the push and pop have their preference decremented to disfavor the register pushed and popped.

### 6. Spill cost

The performance cost of having a variable in memory is calculated for each interference graph node by iterating through each block. For each use of a variable, the operation cost is 1 and the total cost for the use (counted towards the spill cost) is $c\times 10^d$ where $c$ is the operation cost and $d$ the loop nesting depth [1].

### 7-8. Simplify and select

Nodes in the interference graph are iterated in descending spill cost (highest spill cost first) for register assignment (coloring) or spilling. For a node N, if more than one register is available, it will choose the register with the first highest register preference score. If node N cannot be assigned a register, node N is spilled as it has the lowest spill cost (as this is the latest node, the earlier nodes have higher spill cost because of the order in iterating the interference graph). No additional actions are required after all the nodes are iterated, as it is not possible for spilled variables to be reassigned a register since: 1) Variables assigned a register never give the register away and 2) Unassigned neighbor variables when the spilled occurred would either be assigned or spilled, offering no openings in registers.

The algorithm above minimizes unnecessary spilling, prioritizes the allocation of registers to those with the highest spill cost, and prioritizes the assignment of registers to those which can make best use of them.

## Code Generator

The code generator converts the pseudo-assembly to assembly by replacing the variables with actual registers determined by the register allocator. Where necessary, the code generator inserts spill code for any spilled variables in instructions. For spill reloads on x86, it will always push a register because it must be live (as all registers are taken, causing the spill).

The code generator also performs some basic optimizations while generating the final assembly as it is more convenient to do so here. They are the removal of unnecessary instructions when it was revealed during register allocation that the variables are actually in the same locations, or that a register is not live to avoid saving and restoring a register. See below for an example.

Example if variables z and y were spilled, a register is used to temporarily hold the value:

```asm
Pseudo-assembly

mov %z, %y
```

```asm
Assembly

push ebx
mov  ebx,             DWORD [rbp-0x4]
mov  DWORD [rbp-0x8], ebx
pop  ebx
```

Example of unnecessary instructions being removed as t0, t1 were assigned register eax and it was discovered register d is not live:

```asm
Pseudo-assembly

push edx
mov  %t0, eax
mov  %t1, %t0
imul %t1, %argc
pop  edx
```

```asm
Assembly

imul eax, edi
```

## Implementation of language features

### Arrays

Arrays are always precolored to the stack and have assembly generation patterns specific to them as they are handled differently from other automatic variables. Referencing each element of an array is translated to a reference to a location the stack. When generating assembly, the size directive emitted is not the size of the array, but the size of the array's element, e.g., int p[] gets a size directive DWORD.

## References

[1] Preston Briggs, Keith D. Cooper and Linda Torczon, "Improvements to Graph Coloring Register Allocation," Rice University, May 1994. DOI: 10.1145/177492.177575
