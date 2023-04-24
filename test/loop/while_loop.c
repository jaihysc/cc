int main(int argc, char** argv) {
    int count = 0;
    while (argc < 20) {
        argc += 3;
        ++count;
    }
    return count;
}
