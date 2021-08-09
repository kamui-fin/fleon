#define LENGTH(array) (sizeof(array) / sizeof(*array))

#define RESET "\033[0m"
#define BLACK "\033[30m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN "\033[36m"
#define WHITE "\033[37m"

#define LOG(fd, color, level, msg, ...)                                        \
    fprintf(fd,                                                                \
            __FILE__ ":%d"                                                     \
                     " " color level ":" RESET " " msg "\n",                   \
            __LINE__, ##__VA_ARGS__);

// TRACE
#define TLOG(msg, ...) LOG(stdout, WHITE, "TRACE", msg, ##__VA_ARGS__);

// INFO
#define ILOG(msg, ...) LOG(stdout, GREEN, "INFO", msg, ##__VA_ARGS__);

// DEBUG
#define DLOG(msg, ...) LOG(stdout, BLUE, "DEBUG", msg, ##__VA_ARGS__);

// WARN
#define WLOG(msg, ...) LOG(stderr, YELLOW, "WARN", msg, ##__VA_ARGS__);

// ERROR
#define ELOG(msg, ...) LOG(stderr, MAGENTA, "ERROR", msg, ##__VA_ARGS__);

// FATAL
#define FLOG(msg, ...) LOG(stderr, RED, "FATAL", msg, ##__VA_ARGS__);
