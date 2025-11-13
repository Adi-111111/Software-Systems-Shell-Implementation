#include "s3.h"

int main(int argc, char *argv[]){
    char line[MAX_LINE];
    char lwd[MAX_PROMPT_LEN-6];
    init_lwd(lwd);

    while (1) {
        read_command_line(line);
        (void)execute_batch(line, lwd);
    }
    return 0;
}
