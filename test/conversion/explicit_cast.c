int main(int argc, char** argv) {
    // Keep only the lower 8 bits
    int a = (unsigned char)-1;
    // Check high bits are zeroed by shifting some down
    for (int i = 0; i < argc; ++i) {
        a /= 2;
    }
    return a;
}
