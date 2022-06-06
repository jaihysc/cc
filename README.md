# CC - C Compiler in C

The end goal is to create a self compiling compiler.

See [here](https://github.com/jaihysc/cc/wiki/Devlog) milestones on the compiler development.

See `doc/` information about the project and compiler design.

## Usage

Currently, gcc, NASM, and ld are required. (gcc for preprocessing, NASM for assembling, ld for linking)

To build all stages of the compiler, run 'build.sh' (located in the root project directory):

```bash
build.sh [debug|release] # Example: ./build.sh debug
```

To run the compiler through all stages from preprocessing to binary output, run `cc.sh` (located in `out/` after building):

```bash
cc.sh file_name [flags] # Example: Compiling test.c : ./cc.sh test.c
                        # Flags for each stage are located in doc/
```

