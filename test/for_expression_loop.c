// For loop with expression in place of declaration
int main(int argc, char** argv) {
    int count = 0;
    int i = -20;
    for (i = 0; i < argc; ++i) {
        ++count;
    }
    return count;
}
