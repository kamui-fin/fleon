#include <stdio.h>

#define LENGTH(array) (sizeof(array) / sizeof(array[0]))

#define RESET "\033[0m"
#define BLACK "\033[30m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN "\033[36m"
#define WHITE "\033[37m"

// INFO
#define ILOG(msg) _log(stdout, GREEN, msg);

// DEBUG
#define DLOG(msg) _log(stdout, BLUE, msg);

// WARN
#define WLOG(msg) _log(stdout, YELLOW, msg);

// ERROR
#define ELOG(msg) _log(stdout, MAGENTA, msg);

// FATAL
#define FLOG(msg) _log(stdout, RED, msg);

void _log(FILE *fd, char *color, char *msg) {
    fprintf(fd, "%s:%d %sWARN: " RESET "%s\n", __FILE__, __LINE__, color, msg);
}

int main(int argc, char *argv[]) { return 0; }
