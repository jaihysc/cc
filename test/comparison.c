int main(int argc, char** argv) {
    int a = argc == 2 == 1;
    int b = argc != 2;
    int c = argc < 2 < 2;
    int d = argc <= 2;
    int e = argc > 2;
    int f = argc >= 2;

    100 < argc;
    100 <= argc;
    100 > argc;
    100 >= argc;

    return a + b * 2 + c * 4 + d * 8 + e * 16 + f * 32;
}
