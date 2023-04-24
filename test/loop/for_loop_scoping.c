int main(int argc, char** argv) {
    int i = 0;
    for (int i = 0; i < argc; ++i) {
        ++i;
    }

    int count = 0;
    for (int i = 0; i < argc; ++i) {
        ++i;
        ++count;
    }
    return i + count;
}
