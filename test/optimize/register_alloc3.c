int main(int argc, char** argv) {
    int a = 1;
    int b = argc;
    int c = 3;

    // Attempt to coalesce the temporary from b + c into a
    // However a has an edge to argc (at a's definition)
    // Thus a cannot be coalesced into same register as argc
    a = b + c;
    return a;
}
