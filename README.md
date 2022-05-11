# CC - C Compiler in C

The end goal is to create a self compiling compiler.

See [here](https://github.com/jaihysc/cc/wiki/Devlog) milestones on the compiler development.

See `doc/` information about the project and compiler design.

## Usage

Currently, gcc and NASM is required. (gcc for preprocessing, NASM for final binary generation)

To run the compiler through all stages from preprocessing to binary output, run `cc.sh <file name>` (located in `src/`)

