// Tests the correct treatment of the indirection operator as an
// Lvalue and non Lvalue
int main(int argc, char** argv) {
    int a;
    int* p1 = &a;

    *p1 = argc;
    argc = *p1;

    return argc;
}
