#if defined(_WIN32) || defined(_WIN64)
#define OS_IS WINDOWS
#elif defined(__linux__) || defined(__APPLE__) || defined(__MACH__) ||         \
    defined(__unix__) || defined(__unix)
#define OS_IS LINUX_UNIX
#else
return 1;
#endif
