// Chained prefix, postfix increment and decrement

int main(int argc, char** argv) {
    int a = argc;
    int b = ++++++argc;
    int c = argc++++++;
    int d = argc;
    int e = ----argc;
    int f = argc----;
    int g = argc;

    return a + b + c + d + e + f + g;
}
