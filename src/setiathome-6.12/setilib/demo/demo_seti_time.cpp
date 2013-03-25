#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include "seti_time.h"


int main(int argc, char ** argv) {

    char * ast_string = "0633305281069";

    if(argc > 1) {
        if(strcmp(argv[1], "-ast") == 0) {
            ast_string = argv[2];
        }
    }

    seti_time s_t(ast_string);
    s_t.print_internals();
    printf("ast_string: %s julian day : %.10lf\n", ast_string, s_t.jd().uval());
    printf("julian day : %s\n", s_t.printjd().c_str());
}
