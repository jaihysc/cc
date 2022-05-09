// Prefix, postfix increment and decrement

int main(int argc, char** argv) {
    int a = ++argc;
    int b = argc++;
    int c = --argc;
    int d = argc--;
    int e = 0; // Remains 0
    ++e;
    e++;
    --e;
    e--;
    return a + b + c + d + e + argc++;
}
