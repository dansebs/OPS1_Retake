void create_recursive(int current_depth, int max_depth) {
    printf("[PID %d] I am at depth %d\n", getpid(), current_depth);

    if (current_depth >= max_depth) {
        // Base case: Stop recursion
        return; 
    }

    // Logic: Create 2 children
    for (int i = 0; i < 2; i++) {
        pid_t pid = fork();
        if (pid < 0) ERR("fork");

        if (pid == 0) {
            // CHILD
            // Recurse! Child calls the function again with depth + 1
            create_recursive(current_depth + 1, max_depth);
            exit(EXIT_SUCCESS); // Important: Child exits after function returns
        }
    }

    // PARENT
    // Wait for the children I just created
    while(wait(NULL) > 0);
}