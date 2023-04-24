int main(int argc, char** argv) {
    char a = 255;
    // a widened to int, thus b = 256
    int b = ++a;
    return b - 1;
}
