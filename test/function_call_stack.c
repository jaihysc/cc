// Calls a function with many arguments
int f(
        int v1,  int v11, int v21, int v31, int v41,
        int v2,  int v12, int v22, int v32, int v42,
        int v3,  int v13, int v23, int v33, int v43,
        int v4,  int v14, int v24, int v34, int v44,
        int v5,  int v15, int v25, int v35, int v45,
        int v6,  int v16, int v26, int v36, int v46,
        int v7,  int v17, int v27, int v37, int v47,
        int v8,  int v18, int v28, int v38, int v48,
        int v9,  int v19, int v29, int v39, int v49,
        int v10, int v20, int v30, int v40, int v50) {
    return v1 + v2 + v3 + v4 + v5 + v6 + v7 + v8 + v9 + v10 + v11 + v12 + v13 +
        v14 + v15 + v16 + v17 + v18 + v19 + v20 + v21 + v22 + v23 + v24 + v25 +
        v26 + v27 + v28 + v29 + v30 + v31 + v32 + v33 + v34 + v35 + v36 + v37 +
        v38 + v39 + v40 + v41 + v42 + v43 + v44 + v45 + v46 + v47 + v48 + v49 +
        v50;
}


int main(int argc, char** argv) {
    return f(
        argc + 1,  argc + 11, argc + 21, argc + 31, argc + 41,
        argc + 2,  argc + 12, argc + 22, argc + 32, argc + 42,
        argc + 3,  argc + 13, argc + 23, argc + 33, argc + 43,
        argc + 4,  argc + 14, argc + 24, argc + 34, argc + 44,
        argc + 5,  argc + 15, argc + 25, argc + 35, argc + 45,
        argc + 6,  argc + 16, argc + 26, argc + 36, argc + 46,
        argc + 7,  argc + 17, argc + 27, argc + 37, argc + 47,
        argc + 8,  argc + 18, argc + 28, argc + 38, argc + 48,
        argc + 9,  argc + 19, argc + 29, argc + 39, argc + 49,
        argc + 10, argc + 20, argc + 30, argc + 40, argc + 50) % 255;
}
