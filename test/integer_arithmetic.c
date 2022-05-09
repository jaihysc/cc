// Tests arithmetic operations

int main(int argc, char** argv) {
    int a = argc;
    int b = argc / argc * argc / argc * argc / argc * argc / argc - 1; // equal to 0
    int c = 2 - (3 + 4 * (5 - 6 * (7 + 8 * (9 - 10 * (11 - a) + a) - a) + a) - a);
    return (b + c + 20000) % 255; // Make value between 0 and 255
}
