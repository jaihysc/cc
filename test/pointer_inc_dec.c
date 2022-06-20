int main(int argc, char** argv) {
    short a[10];
    a[0] = 1;
    a[1] = 2;
    a[2] = 3;
    a[3] = 4;
    a[4] = 5;
    a[5] = 5;
    a[6] = 6;

    short* p1 = a;
    short* p2 = ++p1;
    short* p3 = p1++;
    short* p4 = p1;
    short* p5 = --p1;
    short* p6 = p1--;
    short* p7 = p1;

    if (*p1 == 1 && *p2 == 2 && *p3 == 2 && *p4 == 3 && *p5 == 2 && *p6 == 2 && *p7 == 1) {
        return 1;
    }
    return 0;
}
