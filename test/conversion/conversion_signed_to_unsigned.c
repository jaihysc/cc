int main(int argc, char** argv) {
    // Keep only the lower 8 bits
    unsigned char a = -1;
    int b = a;

    // Check high bits are zeroed by shifting some down
    for (int i = 0; i < argc; ++i) {
        b /= 2;
    }
    return b;
}
