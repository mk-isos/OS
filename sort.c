#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>

enum task_status
{
    UNSORTED,
    SORTED,
    COMPLETE
};

struct sorting_task
{
    enum task_status status; // enum 추가
    int n_a;
    double *a;
};

// You must use at least one conditional variable.
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cv = PTHREAD_COND_INITIALIZER;

/* For your information: How to use Mutex and Conditional Variable

    pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER ;
    pthread_mutex_lock(&m) ;
    pthread_mutex_unlock(&m) ;
    pthread_cond_t cv = PTHREAD_COND_INITIALIZER ;
    pthread_cond_signal(&cv) ;
    pthread_cond_wait(&cv, &m) ;
*/

void *sort_thread(void *ptr)
{
    // 1. Receive a sorting_task object via ptr.
    struct sorting_task *task = (struct sorting_task *)ptr;

    // 2. Sort the given array in ascending order, and then update the status as SORTED.
    /* For your reference: Sorting algorithm

    int i, j ;
    for (i = 0 ; i < N - 1 ; i++) {
        for (j = i + 1 ; j < N ; j++) {
            if (!(a[i] < a[j])) {
                double tmp ;
                tmp = a[i] ;
                a[i] = a[j] ;
                a[j] = tmp ;
            }
        }
    }
    */
    int i, j;
    for (i = 0; i < task->n_a - 1; i++)
    {
        for (j = i + 1; j < task->n_a; j++)
        {
            if (!(task->a[i] < task->a[j]))
            {
                double tmp;
                tmp = task->a[i];
                task->a[i] = task->a[j];
                task->a[j] = tmp;
            }
        }
    }
    pthread_mutex_lock(&m);

    task->status = SORTED;
    pthread_cond_signal(&cv);

    pthread_mutex_unlock(&m);

    // 3. Wait until the main thread updates the status as COMPLETE.
    // while (task->status != COMPLETE) , cv wait

    pthread_mutex_lock(&m);

    while (task->status != COMPLETE)
    {
        pthread_cond_wait(&cv, &m);
    }

    pthread_mutex_unlock(&m);

    // 4. Free the sorting_task object.
    free(task);

    return NULL;
}

int main()
{
    int i;

    struct sorting_task *t = (struct sorting_task *)malloc(sizeof(struct sorting_task)); // refc : orting_task -> struct sorting_task

    t->status = UNSORTED;
    t->n_a = 1000;
    t->a = (double *)calloc(1000, sizeof(double));
    for (i = 0; i < t->n_a; i++)
    {
        int num = rand();
        int den = rand();
        if (den != 0.0)
            t->a[i] = ((double)num) / ((double)den);
        else
            t->a[i] = ((double)num);
    }

    pthread_t thread;
    pthread_create(&thread, NULL, sort_thread, t); // refc: &tread -> &thread / &t -> t

    // 1. Wait until the child thread to finish sorting and update the task status as SORTED.
    // while (t->status != SORTED)

    pthread_mutex_lock(&m);

    while (t->status != SORTED)
    {
        pthread_cond_wait(&cv, &m);
    }

    pthread_mutex_unlock(&m);

    // 2. Print out the sorted numbers.
    for (i = 0; i < t->n_a; i++)
    {
        printf("%lf ", t->a[i]);
    }

    // 3. Update the task status as COMPLETE
    // signal
    pthread_mutex_lock(&m);

    t->status = COMPLETE;
    pthread_cond_signal(&cv);

    pthread_mutex_unlock(&m);

    return EXIT_SUCCESS;
}
