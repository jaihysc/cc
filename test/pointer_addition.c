int main(int argc, char** argv) {
    int a[6];
    a[0] = 1;
    a[1] = 2;
    a[2] = 3;
    a[3] = 4;
    a[4] = 5;
    a[5] = 6;

    int* b = 1 + a + 2;
    return *b;
}
