# Assembly generator

The assembly generator converts the intermediate language (IL) into X86 assembly, the various stages are listed below:

```
                                          -------------
                    Register preferences  | Register  |
                ------------------------> | Allocator |
                |                         -------------
                |                           |
                |                           | Register allocations
                |                           v
    ---------------                       -------------
IL  | Instruction | Pseudo-assembly       | Code      | Assembly
--> | Selector    | --------------------> | Generator | -------->
    ---------------                       -------------
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

### Program graph

The program graph is currently represented as a control flow graph, in the future it can be a program dependency graph (PDG) or static single assignment (SSA) graph to aid instruction scheduling and optimizations. It contains data and control flow information together, with the goal of giving the instruction selector a broader view of the program under compilation to generate better assembly.

Blocks in the control flow graph are used to build expression trees. By using a tree, it reduces various arrangements of IL instructions into a single form, making it easier for the instruction selector to recognize patterns. An expression tree is formed from IL instructions which are dependent on each other, with the top node being the result of calculations using the leaves. When an IL instruction is encountered which does not fit in an expression tree (not dependent on or by anything in the tree), it is put into a new expression tree.

The control flow graph is built in 2 phases. The first phase creates the blocks plus its contents and links blocks to blocks immediately following it, the second phase links jumps from blocks to labels in other blocks. The expression trees at built one at a time as required by the instruction selector, accomplished by scanning IL instructions in a block and adding it to the tree until an instruction is encountered which does not fit in the tree (instruction is not dependent on or by anything in the tree).

### Register preferences

The goal of register preferences is to improve assembly quality by choosing optimal registers such that the register benefits the current variable with better generated assembly, while also avoiding disadvantaging others by worsening their generated assembly. For example `idiv` prefers others not use `eax` and `edx` as it is needed for the instruction, otherwise it needs to save and restore those registers which leads to extra operations.

To quantify preferences, there are two sources of register preference scores, variables and statements. Each preference score has a score for each register, positive scores means the register is preferred, and a negative score means the register is avoided. On the `idiv` example, the statement has a negative register preference score on eax and edx.

Certain instructions prefer using the same register as another instruction, for example `z = a - b - c` translates to IL:

```
sub t1,a,b   (1) t1 = a - b
sub z,t1,c   (2) z = t1 - c
```

The first sub prefers t1 be in the same register as a, as it can emit the single X86 instruction `sub ra,rb` where ra and rb corresponds to the registers for a and b respectively. Otherwise, it has to emit additional an instruction such as `mov rt1,ra`, where rt1 corresponds to the register for t1.

To model the preference for some variable $v$, it has the attribute $v_p$. When calculating the net register preference score for $v$, an adjustable addition is made to the score for the register which $v_p$ resides in.

### Net register preference score

The net register preference score $S$ for a register $r$ of variable $v$ which prefers to have the same register as $v_p$, and there exists other variables $v_o$ and statements $s_o$ is as follows. Let $\operatorname{p(a,b)}$ be the register preference score of some variable $a$ for some register $b$:

$$ S = \operatorname{p}(v,r) - \sum_i \operatorname{p}(v_{oi},r) + \sum_i \operatorname{p}(s_{oi},r) + v_p$$

If $v_p$ is not applicable ($r$ is not the same as the register chosen for $v_p$), it is 0. $v$ will prefer taking registers others do not want, and avoid taking registers others do want.

### Pseudo-assembly

Pseudo-assembly (pasm) is a low level IR, target specific language which closely resembles assembly. Its difference from assembly is unlimited registers, each variable is seen as a register, written `%name`.

On X86, do not confuse the % for AT&T syntax, pseudo-assembly on X86 is in Intel syntax and the % here represents a virtual register for the variable. For example, targeting X86, the following pseudo-assembly may be generated from the intermediate language:

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

## Instruction Selector

The instruction selector performs tree rewriting of expression trees in the program graph to select the optimal pseudo-assembly. Each pattern contains information on cost, register preferences, and pseudo-assembly replacement. Cost guides the search for the optimal pattern, register preferences are added to the variables' respective register preference scores, pseudo-assembly replacement maps the IL instruction(s) to pseudo-assembly and creates additional temporaries where necessary. Different patterns exist to cover different cases of how the operands are stored, i.e., in register, in memory, is immediate. See below for an example.

To prepare for this stage, the assembly generator makes a pass through the intermediate language to load the symbol table and generate a program graph.

The matching of patterns to rewrite the expression tree is performed bottom up from the leaves, applying the largest possible pattern at each opportunity. Patterns are stored as an arrangement of nodes which the pattern matcher compares against, if a match is made, the matched nodes of the tree is replaced and pseudo-assembly is emitted.

More complex optimizations which are not possible with expression trees such as sub-expression elimination are the job of the optimizer.

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

## Register Allocator

The register allocator is similar to a Chaitin-style allocator with some modifications, the following numbered subsections correspond to a phase in the register allocator.

Spill code generation is not part of the register allocator and is generated by the code generator where necessary.

### 1. Renumber

Each block in the control flow graph is walked backwards to generate liveness use/def information for a block. Then, the control flow graph is walked backwards from the end to generate live range information per block.

### 2. Build

Using liveness information, an interference graph is built for all the variables. The graph is built by scanning statements backwards from the end of each block and joining two nodes (each node representing a variable) with an edge if both variables are live at the same time. This is also done for the live variables exiting the block.

### 3. Coalesce

Planned: Attempts to reduce the number of live ranges by merging two live ranges if one is a copy of the other and they do not interfere.

### 4. Spill cost

The performance cost of having a variable in memory is calculated for each interference graph node by iterating through each block. For each use of a variable, the operation cost is 1 and the total cost for the use (counted towards the spill cost) is $c\times 10^d$ where $c$ is the operation cost and $d$ the loop nesting depth [1].

### 5-6. Simplify and select

Nodes in the interference graph are iterated in descending spill cost (highest spill cost first) for register assignment (coloring) or spilling. For a node N, if more than one register is available, it will choose the register with the highest net register preference score. If node N cannot be assigned a register, node N is spilled as it has the lowest spill cost (as this is the latest node, the earlier nodes have higher spill cost because of the order in iterating the interference graph). No additional actions are required after all the nodes are iterated, as it is not possible for spilled variables to be reassigned a register since: 1) Variables assigned a register never give the register away and 2) Unassigned neighbor variables when the spilled occurred would either be assigned or spilled, offering no openings in registers.

The algorithm above minimizes unnecessary spilling, prioritizes the allocation of registers to those with the highest spill cost, and prioritizes the assignment of registers to those which can make best use of them.

## Code Generator

The code generator converts the pseudo-assembly to assembly by replacing the variables with actual registers determined by the register allocator. Where necessary, the code generator inserts spill code for any spilled variables in instructions. For spill reloads on X86, it will always push the b register because b must be live (as all registers are taken, causing the spill). The reason for choosing the b register is an arbitrary choice. See below for an example.

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

## References

[1] Preston Briggs, Keith D. Cooper and Linda Torczon, "Improvements to Graph Coloring Register Allocation," Rice University, May 1994. DOI: 10.1145/177492.177575
