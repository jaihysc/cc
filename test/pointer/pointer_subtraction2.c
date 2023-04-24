int main(int argc, char** argv) {
    long long a[1000];
    for (int i = 0; i < 1000; ++i) {
        a[i] = i * argc;
    }

    long long* p1 = a;
    long long* p2 = a + 100;
    long long* p3 = a + (p2 - p1) - 99;
    return *p3;
}
