// Checks correct implementation of break in loops
int main(int argc, char** argv) {
    int count = 0;
    for (int i = 1; i < 30; ++i) {
        for (int j = 1; j < 40; ++j) {
            if (j % 2 == 0) break;
            ++count;
        }
        if (i % 2 == 0) break;
    }

    int i = 1;
    while (++i < 10) {
        if (i % 4 == 0) break;
        ++count;
    }

    i = 1;
    do {
        if (i % 5 == 0) break;
        ++count;
    } while (++i < 15);

    return count;
}
