// Handling of &*p
int main(int argc, char** argv) {
    int a = 5;
    int* p1 = &a;
    int* p2 = &*p1;
    *p2 = 10;
    return a;
}
