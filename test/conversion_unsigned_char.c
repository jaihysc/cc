int main(int argc, char** argv) {
    unsigned char a = 0;
    unsigned char b = 1;

    // Common type is int
    // a, b is implicitly converted to int prior to a - b
    // Thus when compared to int 255, it is not equal
    // returns 0
    if (a - b == 255) {
        return 1;
    }
    return 0;
}
