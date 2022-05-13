// Checks scopes of if is handled correctly
int main(int argc, char** argv) {
    int a = 5;
    if (argc == 1) {}

    if (argc == 2)
        a = 7;

    if (argc == 3) {
        a = 8;
    }
    if (argc == 4) {
        int a = 9;
        a = 10;
    }

    return a;
}
