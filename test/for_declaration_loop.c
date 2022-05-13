// For loop with declaration
int main(int argc, char** argv) {
    int count = 0;
    for (int i = 0; i < argc; ++i) {
        ++count;
    }
    return count;
}
