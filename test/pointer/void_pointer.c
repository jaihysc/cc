int main(int argc, char** argv) {
    int a[5];
    void* p;

    p = a;
    *(int*)p = 5;
    return a[0];
}
