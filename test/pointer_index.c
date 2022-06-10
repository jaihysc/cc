int main(int argc, char** argv) {
    int a = 0;
    char* p = (char*)&a;

    p[1] = argc;
    p[0] = p[1];
    return a;
}
