// Constant sized array on the stack
int main(int argc, char** argv) {
    int a[5];
    a[0] = 5;
    a[1] = 4;
    a[2] = 3;
    a[3] = 2;
    a[4] = 1;

    return a[argc];
}
