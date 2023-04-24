// Double indirection pointer
int main(int argc, char** argv) {
    int* p1 = &argc;
    int** p2 = &p1;

    p2[0][0] = 100;
    **p2 = 99;
    return argc;
}
