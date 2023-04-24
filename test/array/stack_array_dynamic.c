// Constant sized array on the stack
int main(int argc, char** argv) {
    int a[argc];
    a[argc - 1] = argc;

    return a[argc - 1];
}
