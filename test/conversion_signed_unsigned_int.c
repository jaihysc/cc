int main(int argc, char** argv) {
    unsigned a = 1;
    signed b = -2;
    // signed and unsigned has same rank, all converted
    // to unsigned, thus greater than 0
    if (a + b < 0) {
        return 1;
    }
    return 0;
}
