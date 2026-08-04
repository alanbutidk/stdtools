#if defined(_WIN32) || defined(_WIN64)
#define OS_IS 1
#elif defined(__linux__) || defined(__APPLE__) || defined(__MACH__) ||         \
    defined(__unix__) || defined(__unix)
#define OS_IS 2
#else
#define OS_IS 3
#endif
