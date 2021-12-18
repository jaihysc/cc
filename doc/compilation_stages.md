# Compilation stages

The compilation is broken down into 2 main stages with substages:

1. Frontend
    1. Preprocess
    2. Parse
2. Backend
    1. Assembly generation
    2. Object file generation
    3. Executable generation

### Preprocess

C source -> Preprocessed C

For now, the preprocessor used is provided by gcc. Input c source file is read in and output (imm1) is generated.

### Parse

Preprocessed C -> Intermediate language

The provided preprocessed source file (imm1) is scanned, and an intermediate output (imm2) is generated.

### Assembly generation

Preprocessed C -> Intermediate language

The intermediate output (imm2) is read in and x86-64 assembly (imm3) in Intel syntax is generated.

### Object file generation

For now, the assembler used is NASM. Intermediate output (imm3) is read in and object file (imm4) is generated.

### Executable generation

For now, the linker used is ld. Object file (imm4) is read and the final executable is generated.

