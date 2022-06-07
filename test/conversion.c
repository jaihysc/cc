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

    return a + b + c + d + e + f + g + h + i + j;
    /*     ~~~~~ <- int
               ~~~~~ <- int
                   ~~~~~ <- int
                       ~~~~~ <- int
                           ~~~~~ <- unsigned
                               ~~~~~ <- !!unsigned long!!
                                   ~~~~~ <- unsigned long
                                       ~~~~~ <- long long
                                           ~~~~~ <- unsigned long long*/
}
