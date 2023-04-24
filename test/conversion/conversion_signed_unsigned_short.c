int main(int argc, char** argv) {
    unsigned short a = 1;
    short b = -2;
    // signed unsigned short to unsigned short
    // unsigned short and int to int
    // Common type is int
    if (a + b < 0) {
        return 1;
    }
    return 0;
}
