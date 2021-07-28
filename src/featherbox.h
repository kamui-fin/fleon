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
#define ILOG(msg) _log(stdout, GREEN, "INFO", msg);

// DEBUG
#define DLOG(msg) _log(stdout, BLUE, "DEBUG", msg);

// WARN
#define WLOG(msg) _log(stderr, YELLOW, "WARN", msg);

// ERROR
#define ELOG(msg) _log(stderr, MAGENTA, "ERROR", msg);

// FATAL
#define FLOG(msg) _log(stderr, RED, "FATAL", msg);
