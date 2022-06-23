// Function call involving registers only
// Check that the caller and callee saves the appropriate registers

int f(char a, short b, int c, long d, long long e) {
    int v1 = a;
    int v2 = b;
    int v3 = c;
    int v4 = d;
    int v5 = e;
    int v6 = d;
    int v7 = c;
    int v8 = b;
    int v9 = a;
    int v10 = b;
    int v11 = c;
    int v12 = d;
    int v13 = e;
    int v14 = d;
    int v15 = c;
    int v16 = b;
    int z = v1 + v2 + v3 + v4 + v5 + v6 + v7 + v8 + v9 + v10 + v11 + v12 + v13 + v14 + v15 + v16;
    return z + a + b + c + d + e;
}

int main(int argc, char** argv) {
    int a = argc + 1;
    int b = argc + 2;
    int c = argc + 3;
    int d = argc + 4;
    int e = argc + 5;

    int v1 = 10;
    int v2 = 11;
    int v3 = 12;
    int v4 = 13;
    int v5 = 14;
    int v6 = 15;
    int v7 = 16;
    int v8 = 17;
    int v9 = 18;
    int v10 = 19;
    int v11 = 20;
    int v12 = 21;
    int v13 = 22;
    int v14 = 23;
    int v15 = 24;
    int v16 = 25;

    int y = argc + f(a, b, c, d, e);
    int z = v1 + v2 + v3 + v4 + v5 + v6 + v7 + v8 + v9 + v10 + v11 + v12 + v13 + v14 + v15 + v16;
    return a + b + c + d + e + y + z;
}
