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

Ershov numbers are assigned to reduce register usage, then a interference graph is built of the remaining candidates for register allocation.

### 3. Coalesce

Attempts to reduce the number of live ranges by merging two live ranges if one is a copy of the other and they do not interfere.

### 4. Spill cost

Spill costs are computed based on how each variable will be used if it is in memory. If the variable in memory must be reloaded into a register for a x86 instruction, it has an associated cost. Otherwise, if the variable in memory can be used without reloading into a register (as is the case for some x86 instructions, e.g., imul r32, r/m32), it has a lower cost (non-zero because of cost for memory access). The cost is weighted with $c\times 10^d$ where $c$ is the operation cost and $d$ the loop nesting depth [1].

### 5. Simplify

Nodes in the interference graph are chosen either for register assignment (coloring) or spilling. Given $k$ registers and some node $l_i$, if $\operatorname{neighbor}(l_i) < k$, $l_i$ is marked for coloring and $l_i$ with all of its edges are removed from the graph. Otherwise, a node $m_i$ is marked for spilling and $m_i$ with all of its edges are removed from the graph such that $l_i$ can be trivially colored.

### 7. Select

Registers are assigned to the nodes of the interference graph first based on constraints from x86 instructions which require specific registers. The remaining nodes are then assigned registers by placing them back in the graph one by one (last node removed is first node placed back) giving each a register distinct from its neighbors as it is placed.

### References

[1] Preston Briggs, Keith D. Cooper and Linda Torczon, "Improvements to Graph Coloring Register Allocation," Rice University, May 1994. DOI: 10.1145/177492.177575

