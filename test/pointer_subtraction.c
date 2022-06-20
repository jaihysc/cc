int main(int argc, char** argv) {
    char** p1 = argv;
    char** p2 = argv + 5;

    char** p3 = argv + (p2 - p1) - 4;
    /*                 ~~~~~~~~
                       = 5 */
    return p3[0][3];
}
