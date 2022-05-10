// Tests arithmetic operations

int main(int argc, char** argv) {
    int a = 500;
    a = 0;
    argc += 60001;
    argc -= 4;
    argc *= 100;
    argc /= 2;
    argc %= 255;
    return argc + a;
}
