// Checks that function parameters are scoped correctly
int f(int a, int b) {
    return 2 * a + b;
}

int g(int a, int b) {
    return a + 20 + 2 * b;
}

int main(int a, char** b) {
    return f(g(a, a + 5), g(a, a + 10));
}
