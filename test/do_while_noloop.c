// do while which runs once and exits
int main(int argc, char** argv) {
    do ++argc; while (argc < 0);
    return argc;
}
