// Checks scopes of while is handled correctly
int main(int argc, char** argv) {
    int i = 0;
    int count = 0;
    while (i < 10) {
        int count = 10;
        count *= 100;
        ++i;
    }
    return count;
}
