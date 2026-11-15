#include "complaints.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static volatile unsigned long long benchmark_sink;

static double seconds_between(clock_t start, clock_t finish)
{
    return (double)(finish - start) / (double)CLOCKS_PER_SEC;
}

static void run_benchmark(size_t count, size_t repetitions)
{
    ComplaintStore store = {0};
    size_t i;
    clock_t start;
    clock_t finish;
    unsigned long long local_sink = 0;

    store.items = (Complaint *)calloc(count, sizeof(*store.items));
    if (store.items == NULL) {
        fputs("Could not allocate benchmark records.\n", stderr);
        exit(EXIT_FAILURE);
    }
    store.count = count;
    store.capacity = count;
    for (i = 0; i < count; ++i) {
        store.items[i].id = (int)i + 1;
        store.items[i].priority = (ComplaintPriority)(i % 4);
        store.items[i].status =
            (i % 5 == 0) ? COMPLAINT_IN_PROGRESS : COMPLAINT_PENDING;
        store.items[i].submitted_order = (unsigned long long)i + 1;
    }

    start = clock();
    for (i = 0; i < repetitions; ++i) {
        const Complaint *found = complaint_find(&store, (int)count);
        local_sink += found == NULL ? 0U : (unsigned long long)found->id;
    }
    finish = clock();
    benchmark_sink += local_sink;
    printf("%lu,%lu,lookup,%.3f\n",
           (unsigned long)count,
           (unsigned long)repetitions,
           seconds_between(start, finish) * 1000000.0 / (double)repetitions);

    start = clock();
    for (i = 0; i < repetitions; ++i) {
        const Complaint *next = complaint_next_pending(&store);
        local_sink += next == NULL ? 0U : (unsigned long long)next->id;
    }
    finish = clock();
    benchmark_sink += local_sink;
    printf("%lu,%lu,next_pending,%.3f\n",
           (unsigned long)count,
           (unsigned long)repetitions,
           seconds_between(start, finish) * 1000000.0 / (double)repetitions);
    free(store.items);
}

int main(void)
{
    puts("records,repetitions,operation,microseconds_per_operation");
    run_benchmark(100, 100000);
    run_benchmark(1000, 10000);
    run_benchmark(10000, 1000);
    return benchmark_sink == 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
