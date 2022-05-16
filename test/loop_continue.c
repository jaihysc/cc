// Checks correct implementation of continue in loops
int main(int argc, char** argv) {
    int count = 0;
    for (int i = 0; i < 30; ++i) {
        for (int j = 0; j < 40; ++j) {
            if (j % 2 == 0) continue;
            ++count;
        }
        if (i % 2 == 0) continue;
    }

    int i = 0;
    while (++i < 10) {
        if (i % 4 == 0) continue;
        ++count;
    }

    i = 0;
    do {
        if (i % 5 == 0) continue;
        ++count;
    } while (++i < 15);

    return count;
}
