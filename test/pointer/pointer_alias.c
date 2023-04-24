int main(int argc, char** argv) {
    int a;
    int* p1 = &a;
    int* p2 = &a;
    int* p3 = &a;

    *p1 = argc;
    *p2 = argc + 1;
    *p3 = argc + 2;

    return a;
}
