// Function call but arguments have an expression
int f(int a, int b, int c) {
    return a + b + c;
}

int main(int argc, char** argv) {
    return f(argc + 1, argc + 2, argc + 3);
}
