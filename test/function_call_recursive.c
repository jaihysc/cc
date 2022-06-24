// Recursive power
int pow(unsigned base, unsigned exponent) {
    if (exponent == 0) {
        return 1;
    }
    return base * pow(base, exponent - 1);
}

int main(int argc, char** argv) {
    return pow(2, argc);
}
