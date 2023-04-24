int main(int argc, char** argv) {
    char a = 1;
    unsigned char b = 2;
    short c = 3;
    unsigned short d = 4;
    int e = 5;
    unsigned f = 6;
    long g = 7;
    unsigned long h = 8;
    long long i = 9;
    unsigned long long j = 10;

    a += j;
    b -= i;
    c *= h;
    d /= g;
    e %= f;

    int z = a = b = c = d = e = f = g = h = i = j;
    return z;
}
