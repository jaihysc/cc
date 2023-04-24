int main(int argc, char** argv) {
    int count = 0;
    for (int i = 1; i < 10; ++i) {
        count += argc;
    }

    int i = 1;
    while (i < 10) {
        ++i;
        count += argc;
    }
    return count;
}
