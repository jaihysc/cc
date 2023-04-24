// Checks that pointers and arrays are correctly passed
int f(int* p, int* val) {
    p[0] = *val;
    p[1] = *val + 1;
    p[2] = *val + 2;
}

int main(int argc, char** argv) {
    int a[10];
    f(a, &argc);
    return a[0] + a[1] + a[2];
}
