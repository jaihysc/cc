# Assembly generator

The assembly generator makes two passes through the intermediate language. The first pass currently is to load the symbol table, in the future this can also setup data structures. The second pass generates assembly.

## Control flow graph

The control flow graph is built in 2 phases. The first phase links blocks to blocks immediately following it, the second phase links jumps from blocks to labels in other blocks.

## Register allocation

The register allocator is similar to a Chaitin-style allocator with some modifications, the following numbered subsections correspond to a phase in the register allocator.

Spill code is delayed until code generation, as certain x86 instructions are capable of loading from memory.

Certain x86 instructions require the use of specific registers. Thus, to guide the assignment of registers, IL instructions contain constraint information indicating where the operands must be located and where the result is stored. For example, `loc_op1, reg_r, reg_rm` indicates that the result is placed in the location of operand 1, operand 1 is a register, and operand 2 is a register or memory.

### 1. Renumber

Each block is walked backwards to generate liveness use/def information for a block. Then, the flow graph is walked backwards from the end to generate live range information per block.

### 2. Build

Using liveness information, an interference graph is built for all the variables. The graph is built by scanning statements backwards from the end of each block and joining edges based on what is live at each statement and the live variables exiting the block.

### 3. Coalesce

Attempts to reduce the number of live ranges by merging two live ranges if one is a copy of the other and they do not interfere.

### 4. Spill cost

The cost is calculated for each interference graph node by iterating through each block. For each use of a variable, the operation cost is 1 and the total cost for the use (counted towards the spill cost) is $c\times 10^d$ where $c$ is the operation cost and $d$ the loop nesting depth [1].

### 5. Simplify

Nodes in the interference graph are iterated in descending spill cost (highest spill cost first) for register assignment (coloring) or spilling. If a node N is reached which cannot be assigned a register, choose to spill either a neighbor of N or node N itself according to the following algorithm:

- Sort the neighbors of N in order of ascending spill cost and find the first neighbor M1 which has a register assigned.
- If the neighbor M1 has a lower spill cost compared node N and its register can be used for node N, spill the neighbor M1, take the register of neighbor M1 for node N. Otherwise find the second neighbor M2 and spill it if it has a lower spill cost compared to node N and its register can be used for node N, and so on.
- If no suitable neighbors of node N are found, spill node N.

Afterwards, make one final pass through the spilled variables and see if they can be assigned a register after other variables were spilled.

The algorithm above minimizes unnecessary spilling and prioritizes the allocation of registers to those with the highest spill cost.

### 7. Select

Registers are assigned to the nodes of the interference graph first based on constraints from x86 instructions which require specific registers. The remaining nodes are then assigned registers by placing them back in the graph one by one (last node removed is first node placed back) giving each a register distinct from its neighbors as it is placed.

### References

[1] Preston Briggs, Keith D. Cooper and Linda Torczon, "Improvements to Graph Coloring Register Allocation," Rice University, May 1994. DOI: 10.1145/177492.177575

