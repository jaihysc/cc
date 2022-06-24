int f(int x) {
    return x + 9;
}

int g(int x) {
    return x + 8;
}

int h(int x) {
    return x + 7;
}

int i(int x) {
    return x + 6;
}

int j(int x) {
    return x + 5;
}

int k(int x) {
    return x + 4;
}

int l(int x) {
    return x + 3;
}

int m(int x) {
    return x + 2;
}

int n(int x) {
    return x + 1;
}

int main(int argc, char** argv) {
    return f(g(h(i(j(k(l(m(n(argc)))))))));
}
