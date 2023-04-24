// Handling of &p[]
int main(int argc, char** argv) {
    int a[5];
    a[0] = 1;
    a[1] = 2;
    a[2] = 3;
    a[3] = 4;
    a[4] = 5;
    int* p1 = a;
    int* p2 = &p1[4];
    *p2 = 10;
    return a[4];
}
