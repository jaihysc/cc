// Function call involving registers only

int f(char a, short b, int c, long d, long long e) {
    // Some registeres are callee saved
    return a + b + c + d + e;
}

int main(int argc, char** argv) {
    // Some registers are caller saved
    int a = argc + 1;
    int b = argc + 2;
    int c = argc + 3;
    int d = argc + 4;
    int e = argc + 5;
    int x = argc + 6;
    int y = argc + 7;
    int z = argc + f(a, b, c, d, e);
    return a + b + c + d + e + x + y + z;
}
