// Test register allocation system
int main(int argc, char** argv) {
    int a = argc;

    // a should NOT be dead when z defined
    int y = a;
    int z = a;

    return y + z;
}
