#ifndef UTIL_H_
#define UTIL_H_

// ascii escape colors

#define RED	"\e[31;1m"
#define MAGENTA	"\e[35;1m"
#define GREY	"\e[90;1m"
#define WHITE	"\e[97;1m"
#define RESET	"\e[0m"

// printf style type checking

#if defined(__clang__) || defined(__GNUC__)
#define PRINTF(fmt, first) __attribute__((format (printf, fmt, first)))
#else
#define PRINTF(fmt, first)
#endif

#endif //UTIL_H_