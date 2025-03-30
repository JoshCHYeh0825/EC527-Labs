#include <pthread.h>

/* Struct to pass parameters to thread function */
typedef struct {
    arr_ptr v;            // Pointer to the array
    int start_row;        // Starting row for this thread
    int end_row;          // Ending row for this thread
    double *total_change; // Pointer to total change 
    pthread_mutex_t *change_mutex; // Mutex for updating total change
    int *iterations;      // Pointer to iterations counter
} thread_data;

/* Stripe-based Threading SOR */
void SOR_threaded_stripe(arr_ptr v, int *iterations)
{
    long int rowlen = get_arr_rowlen(v);
    int num_threads = 4; // Adjust number of threads as needed
    pthread_t threads[num_threads];
    thread_data thread_args[num_threads];
    double total_change = 1.0e10;
    pthread_mutex_t change_mutex = PTHREAD_MUTEX_INITIALIZER;
    int iters = 0;

    while ((total_change / (double)(rowlen*rowlen)) > (double)TOL) {
        iters++;
        total_change = 0;

        // Create threads
        for (int t = 0; t < num_threads; t++) {
            thread_args[t].v = v;
            thread_args[t].start_row = 1 + t * ((rowlen-2) / num_threads);
            thread_args[t].end_row = (t == num_threads-1) ? rowlen-1 : 
                                      1 + (t+1) * ((rowlen-2) / num_threads);
            thread_args[t].total_change = &total_change;
            thread_args[t].change_mutex = &change_mutex;
            thread_args[t].iterations = &iters;

            pthread_create(&threads[t], NULL, stripe_thread_func, (void*)&thread_args[t]);
        }

        // Wait for all threads to complete
        for (int t = 0; t < num_threads; t++) {
            pthread_join(threads[t], NULL);
        }

        // Check for potential divergence
        if (abs(v->data[(rowlen-2)*(rowlen-2)]) > 10.0*(MAXVAL - MINVAL)) {
            printf("SOR_threaded_stripe: SUSPECT DIVERGENCE iter = %d\n", iters);
            break;
        }
    }

    *iterations = iters;
    printf("    SOR_threaded_stripe() done after %d iters\n", iters);
}

// Thread function for stripe-based threading
void* stripe_thread_func(void* arg)
{
    thread_data* data = (thread_data*)arg;
    arr_ptr v = data->v;
    long int rowlen = get_arr_rowlen(v);
    data_t *matrix = get_array_start(v);
    double local_total_change = 0;

    for (long int i = data->start_row; i < data->end_row; i++) {
        for (long int j = 1; j < rowlen-1; j++) {
            double change = matrix[i*rowlen+j] - .25 * (matrix[(i-1)*rowlen+j] +
                                                        matrix[(i+1)*rowlen+j] +
                                                        matrix[i*rowlen+j+1] +
                                                        matrix[i*rowlen+j-1]);
            matrix[i*rowlen+j] -= change * OMEGA;
            
            // Take absolute value of change
            if (change < 0) {
                change = -change;
            }
            
            // Use mutex to safely update total change
            pthread_mutex_lock(data->change_mutex);
            *data->total_change += change;
            pthread_mutex_unlock(data->change_mutex);
        }
    }

    return NULL;
}

/* Block-based Threading SOR */
void SOR_threaded_block(arr_ptr v, int *iterations)
{
    long int rowlen = get_arr_rowlen(v);
    int num_threads = 4; // Adjust number of threads as needed
    pthread_t threads[num_threads];
    thread_data thread_args[num_threads];
    double total_change = 1.0e10;
    pthread_mutex_t change_mutex = PTHREAD_MUTEX_INITIALIZER;
    int iters = 0;
    int block_size = (rowlen-2) / num_threads;

    // Ensure block_size is consistent with BLOCK_SIZE from original code
    if (block_size % BLOCK_SIZE != 0) {
        fprintf(stderr, "Block size must be multiple of BLOCK_SIZE\n");
        exit(-1);
    }

    while ((total_change/(double)(rowlen*rowlen)) > (double)TOL) {
        iters++;
        total_change = 0;

        // Create threads
        for (int t = 0; t < num_threads; t++) {
            thread_args[t].v = v;
            thread_args[t].start_row = 1 + t * block_size;
            thread_args[t].end_row = (t == num_threads-1) ? rowlen-1 : 
                                      1 + (t+1) * block_size;
            thread_args[t].total_change = &total_change;
            thread_args[t].change_mutex = &change_mutex;
            thread_args[t].iterations = &iters;

            pthread_create(&threads[t], NULL, block_thread_func, (void*)&thread_args[t]);
        }

        // Wait for all threads to complete
        for (int t = 0; t < num_threads; t++) {
            pthread_join(threads[t], NULL);
        }

        // Check for potential divergence
        if (abs(v->data[(rowlen-2)*(rowlen-2)]) > 10.0*(MAXVAL - MINVAL)) {
            printf("SOR_threaded_block: SUSPECT DIVERGENCE iter = %d\n", iters);
            break;
        }
    }

    *iterations = iters;
    printf("    SOR_threaded_block() done after %d iters\n", iters);
}

// Thread function for block-based threading
void* block_thread_func(void* arg)
{
    thread_data* data = (thread_data*)arg;
    arr_ptr v = data->v;
    long int rowlen = get_arr_rowlen(v);
    data_t *matrix = get_array_start(v);
    double local_total_change = 0;

    // Block-based iteration similar to SOR_blocked
    for (long int ii = data->start_row; ii < data->end_row; ii += BLOCK_SIZE) {
        for (long int jj = 1; jj < rowlen-1; jj += BLOCK_SIZE) {
            for (long int i = ii; i < ii+BLOCK_SIZE && i < rowlen-1; i++) {
                for (long int j = jj; j < jj+BLOCK_SIZE && j < rowlen-1; j++) {
                    double change = matrix[i*rowlen+j] - .25 * (matrix[(i-1)*rowlen+j] +
                                                                matrix[(i+1)*rowlen+j] +
                                                                matrix[i*rowlen+j+1] +
                                                                matrix[i*rowlen+j-1]);
                    matrix[i*rowlen+j] -= change * OMEGA;
                    
                    // Take absolute value of change
                    if (change < 0) {
                        change = -change;
                    }
                    
                    // Use mutex to safely update total change
                    pthread_mutex_lock(data->change_mutex);
                    *data->total_change += change;
                    pthread_mutex_unlock(data->change_mutex);
                }
            }
        }
    }

    return NULL;
}