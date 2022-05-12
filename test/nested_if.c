int main(int argc, char** argv) {
    if (argc % 2 == 0) {
        if (argc % 3 == 0) {
            if (argc % 4 == 0) {
                if (argc % 5 == 0)
                    return 1;
                else return 2;
            } else return 3;
        } else return 4;
    } else return 5;
}

