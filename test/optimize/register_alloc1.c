// Test register allocation system
int main(int argc, char** argv) {
    int a = argc;
    int b = 2;

    // a + b should be dead when z defined
    int y = a + b;
    int z = 3;

    return y + z;
}
