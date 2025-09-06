#include "file.h"

#include <real/data.h>
#include <util/lib.h>

struct file_info find_file(const char* filename) {
    struct file_info info;
    info.size = 0;
    info.data = 0;

    void* ptr = data_file_ptr;
    while (1) {
        if (!strcmp(ptr, "DATAEND")) {
            return info;
        }

        if (!strcmp(ptr, filename)) {
            ptr += strlen(ptr) + 1;
            info.size = *(uint32_t*) ptr;
            info.data = ptr + 4;
            return info;
        }

        ptr += strlen(ptr) + 1;
        ptr += (*(uint32_t*) ptr) + 4;
    }
}