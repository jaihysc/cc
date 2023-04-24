// Tests arrangements of boolean operators
int main(int argc, char** argv) {
    int a = 1;
    int b = !a;
    int c = !(argc - 1);
    int d = !(argc - 2);

    int e = a && b || c && d || !(argc - 3);
    int f = a && b && c && d || e;
    return a + b * 2 + c * 4 + d * 8 + e * 16 + f * 32;
}
