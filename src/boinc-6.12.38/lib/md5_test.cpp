#include <cstdio>

#include "md5_file.h"

int main(int, char** argv) {
    char out[33];
    double nbytes;

    md5_file(argv[1], out, nbytes);
    printf("%s\n%f bytes\n", out, nbytes);

    return 0;
}

const char *BOINC_RCSID_c6f4ef0a81 = "$Id: md5_test.cpp 16069 2008-09-26 18:20:24Z davea $";
