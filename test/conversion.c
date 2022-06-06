int main(int argc, char** argv) {
    char a = 1;
    unsigned char b = 2;
    short c = 3;
    unsigned short d = 4;
    int e = 5;
    unsigned f = 6;
    long long g = 7;
    unsigned long long h = 8;

    return a + b + c + d + e + f + g + h;
    /*     ~~~~~ <- int
               ~~~~~ <- int
                   ~~~~~ <- int
                       ~~~~~ <- int
                           ~~~~~ <- unsigned
                               ~~~~~ <- long long
                                   ~~~~~ <- unsigned long long */
}
