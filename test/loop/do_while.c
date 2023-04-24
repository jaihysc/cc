int main(int argc, char** argv) {
    int count = 0;
    do {
        ++argc;
        ++count;
    } while (argc < 20);
    return count;
}
