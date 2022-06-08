int main(int argc, char** argv) {
    int count = 0;

    // Zero extended to int
    unsigned char a1 = 0xFFCDEFAB;
    int a2 = a1;
    if (a2 == 0xAB) {
        ++count;
    }

    // Sign extended to int
    char b1 = 0xbafedcfa;
    int b2 = b1;
    if (b2 == 0xFFFFFFFA) {
        ++count;
    }

    // Zero extended to unsigned
    unsigned char c1 = 0XFEDCBABC;
    unsigned c2 = c1;
    if (c2 == 0XBC) {
        ++count;
    }

    // Zero extended to unsigned
    char d1 = 0XABCDEFFC;
    unsigned d2 = d1;
    if (d2 == 0XFC) {
        ++count;
    }

    return count;
}
