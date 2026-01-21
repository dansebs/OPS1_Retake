// For N workers + 1 Main Thread/Accumulator
// Initialize barrier to N+1, not N!
pthread_barrier_init(&barrier, NULL, n_threads + 1);

// Worker Code
calculate_partial_area();
pthread_barrier_wait(&barrier); // Wait for everyone, including main

// Main Thread Code
pthread_barrier_wait(&barrier); // Wait for workers to finish calc
aggregate_results();            // Safe to aggregate now



// If we are not the first chunk, skip to the next newline 
// to avoid processing a partial line from the previous chunk.
if (idx > 0) {
    // Read and discard until newline
    getline(&line_buf, &line_len, fp); 
}