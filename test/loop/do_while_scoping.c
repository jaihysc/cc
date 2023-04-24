// Checks scopes of do while is handled correctly
int main(int argc, char** argv) {
    int i = 0;
    int count = 0;
    do {
        int count = 0;
        ++count;
        ++i;
    } while (i < 10);

    return count;
}
