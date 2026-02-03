// Get int from env or return default
int get_env_int(const char *var_name, int default_val) {
    char *val = getenv(var_name);
    return val ? atoi(val) : default_val;
}

// Set env and check for '=' errors
void set_env_safe(const char *name, const char *value) {
    if (setenv(name, value, 1)) {
        if (errno == EINVAL) ERR("setenv: name contains '='");
        ERR("setenv");
    }
}