// Tests arithmetic operations with negative numbers

int main(int argc, char** argv) {
    int a = -2 * -2 + -1 + -1 + -1 + -1; // 0
    int b = -5 + (1 - - 1) - - 1; // -2

    return a + b + argc;
}
