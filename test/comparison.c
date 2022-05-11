int main(int argc, char** argv) {
    int a = argc == 2;
    int b = argc != 2;
    int c = argc < 2;
    int d = argc <= 2;
    int e = argc > 2;
    int f = argc >= 2;
    int g = a && b;
    int h = c || d;
    h = !h;
    return a + b * 2 + c * 4 + d * 8 + e * 16 + f * 32 + g * 64 + h * 128;
}
