int main(int argc, char** argv) {
    int a = 5;
    int* p = &a;

    return a * *p - argc;
}
