// Preprocessed c file parser
// Generated output is imm2

#include <stdio.h>

int main(int argc, char** argv) {
    FILE *f = fopen("imm2", "w");
    if (f == NULL) {
        printf("Failed to open output file\n");
        return 1;
    }
    // Output something for now so the compilation can be tested
    fprintf(f, "%s\n", "func main, i32, i32 argc, u8** argv");
    fclose(f);
    return 0;
}

