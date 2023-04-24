int main(int argc, char** argv) {
    long long a = 0;
    long long b = 1;

    // Test result of of operators is int

    int result = 0;
    if (((a < b) + 2147483647 + 2147483647 + 1) == 0) {
        ++result;
    }

    if (((a != b) + 2147483647 + 2147483647 + 1) == 0) {
        ++result;
    }

    if (((a || b) + 2147483647 + 2147483647 + 1) == 0) {
        ++result;
    }
    return result;
}
