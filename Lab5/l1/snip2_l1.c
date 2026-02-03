#include "macros.h"

/* Returns an integer from an environment variable, or a default value if missing */
int get_env_int(const char *name, int default_val) {
    char *env = getenv(name);
    if (env) return atoi(env);
    return default_val;
}

/* Safely set an environment variable with the ERR macro */
void set_env_safe(const char *name, const char *value) {
    if (setenv(name, value, 1)) {
        if (errno == EINVAL) ERR("setenv - invalid name (contains '=')");
        ERR("setenv");
    }
}

/* Simple template for parsing options with getopt */
// Use this in main: while ((c = getopt(argc, argv, "n:h")) != -1)