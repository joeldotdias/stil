#include "shared.h"

int main(int argc, char **argv) {
    if(argc < 2) {
        stil_fatal("Usage: stil <filename>");
    }

    stil_info("STIL");

    return 0;
}
