#if !defined(MAIN)
#define MAIN

#define BLACK "\x1b[38;5;0m"
#define RED "\x1b[38;5;1m"
#define GREEN "\x1b[38;5;2m"
#define YELLOW "\x1b[38;5;3m"
#define BLUE "\x1b[38;5;4m"
#define MAGENTA "\x1b[38;5;5m"
#define CYAN "\x1b[38;5;6m"
#define WHITE "\x1b[38;5;7m"
#define GRAY "\x1b[38;5;8m"
#define BRIGHT_RED "\x1b[38;5;9m"
#define CLEAR "\x1b[0m"

#define ENABLE_LOGGING
#define ENABLE_DEBUG_LOGGING

const bool ENABLE_VALIDATION_LAYER = false;
const bool ENABLE_DEBUG_MESSENGER = true;

const uint64_t MAX_FRAMES_IN_FLIGHT = 2;

#endif