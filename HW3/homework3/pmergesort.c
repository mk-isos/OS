#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h> // getopt 함수를 사용하기 위한 헤더 파일

#define max_tasks 200

// 데이터 배열과 작업 수 배열 정의
double *data;
int *task_counts;

enum task_status
{
    UNDONE,
    PROCESS,
    DONE
};

struct sorting_task
{
    double *a;
    int n_a;
    int status;
};

struct sorting_task tasks[max_tasks];
int n_tasks = 0;
int n_undone = 0;
int n_done = 0;

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cv = PTHREAD_COND_INITIALIZER;

void merge_lists(double *a1, int n_a1, double *a2, int n_a2);
void merge_sort(double *a, int n_a);
void update_task_count(int thread_index); // 스레드의 작업 수 업데이트 함수
void print_task_counts(int num_threads);  // 각 스레드의 작업 수 출력 함수

void *worker(void *ptr)
{
    int thread_index = *(int *)ptr;
    while (1)
    {
        pthread_mutex_lock(&m);
        while (n_undone == 0)
        {
            pthread_cond_wait(&cv, &m);
        }

        int i;
        for (i = 0; i < n_tasks; i++)
        {
            if (tasks[i].status == UNDONE)
                break;
        }

        if (i == n_tasks)
        {
            pthread_mutex_unlock(&m);
            return NULL;
        }

        tasks[i].status = PROCESS;
        n_undone--;
        pthread_mutex_unlock(&m);

        printf("[Thread %d] starts Task %d\n", thread_index, i);

        merge_sort(tasks[i].a, tasks[i].n_a);

        printf("[Thread %d] completed Task %d\n", thread_index, i);

        pthread_mutex_lock(&m);
        tasks[i].status = DONE;
        n_done++;
        update_task_count(thread_index); // 작업 수 업데이트
        pthread_mutex_unlock(&m);
    }
}

// 데이터를 초기화하는 스레드 함수
void *initialize_data(void *arg)
{
    // 인덱스 시작점을 가리키는 포인터를 정수로 형변환하여 start에 할당
    int start = *(int *)arg;

    // 시작 인덱스에서 각 스레드가 처리할 데이터 개수를 더해 종료 인덱스를 계산
    int end = start + (*(int *)(arg + sizeof(int)));

    // start부터 end까지 반복하며 데이터를 초기화
    for (int i = start; i < end; i++)
    {
        // 랜덤한 정수 생성
        int num = rand();
        int den = rand();

        // 분모가 0이 아닐 경우 num을 den으로 나눈 값을 data 배열의 i번째 요소에 할당
        if (den != 0.0)
            data[i] = ((double)num) / ((double)den);
        else
            // 분모가 0일 경우 num을 그대로 data 배열의 i번째 요소에 할당
            data[i] = ((double)num);
    }
    // 함수가 void* 타입의 포인터를 반환해야 하기 때문에 NULL 반환
    return NULL;
}

int main(int argc, char *argv[])
{
    int num_data = 0, num_threads = 0;
    int opt;
    // 명령줄 인자를 처리하여 데이터 수와 스레드 수를 설정
    while ((opt = getopt(argc, argv, "d:t:")) != -1)
    {
        switch (opt)
        {
        case 'd':
            num_data = atoi(optarg);
            break;
        case 't':
            num_threads = atoi(optarg);
            break;
        default:
            fprintf(stderr, "Usage: %s -d <data elements> -t <threads>\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (num_data <= 0 || num_threads <= 0)
    {
        fprintf(stderr, "Invalid arguments.\n");
        exit(EXIT_FAILURE);
    }

    data = malloc(num_data * sizeof(double));
    task_counts = calloc(num_threads, sizeof(int)); // 각 스레드의 작업 수 초기화
    if (data == NULL || task_counts == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    // gettimeofday 실행 시간 계산/출력
    struct timeval start, end;
    gettimeofday(&start, NULL);

    // 데이터 초기화 스레드 배열과 각 스레드의 시작 및 종료 인덱스를 저장할 배열.
    pthread_t init_threads[num_threads]; // 데이터 초기화 스레드 배열
    int indices[num_threads][2];         // 각 스레드의 시작과 종료 인덱스를 저장할 배열

    for (int i = 0; i < num_threads; i++)
    {
        indices[i][0] = i * (num_data / num_threads);                                                   // 시작 인덱스
        indices[i][1] = (i == num_threads - 1) ? (num_data - indices[i][0]) : (num_data / num_threads); // 종료 인덱스
        pthread_create(&init_threads[i], NULL, initialize_data, indices[i]);                            // 데이터 초기화 스레드 생성
    }

    // 모든 초기화 스레드가 작업을 완료할 때까지 대기
    for (int i = 0; i < num_threads; i++)
    {
        pthread_join(init_threads[i], NULL);
    }

    pthread_t threads[num_threads];
    int thread_indices[num_threads];
    for (int i = 0; i < num_threads; i++)
    {
        thread_indices[i] = i + 1;
        pthread_create(&threads[i], NULL, worker, &thread_indices[i]);
    }

    for (int i = 0; i < max_tasks; i++)
    {
        pthread_mutex_lock(&m);

        tasks[n_tasks].a = data + (num_data / max_tasks) * n_tasks;
        tasks[n_tasks].n_a = num_data / max_tasks;
        if (n_tasks == max_tasks - 1)
            tasks[n_tasks].n_a += num_data % max_tasks;
        tasks[n_tasks].status = UNDONE;

        n_undone++;
        n_tasks++;

        pthread_cond_signal(&cv);
        pthread_mutex_unlock(&m);
    }

    pthread_mutex_lock(&m);
    while (n_done < max_tasks)
    {
        pthread_mutex_unlock(&m);
        pthread_mutex_lock(&m);
    }
    pthread_mutex_unlock(&m);

    int n_sorted = num_data / n_tasks;
    for (int i = 1; i < n_tasks; i++)
    {
        merge_lists(data, n_sorted, tasks[i].a, tasks[i].n_a);
        n_sorted += tasks[i].n_a;
    }

    // gettimeofday 실행 시간 계산/출력
    gettimeofday(&end, NULL);
    double elapsed = (end.tv_sec - start.tv_sec) + ((end.tv_usec - start.tv_usec) / 1000000.0);
    printf("\nExecution time: %.2f seconds\n", elapsed);

    print_task_counts(num_threads);

#ifdef DEBUG
    for (int i = 0; i < num_data; i++)
    {
        printf("%lf ", data[i]);
    }
    printf("\n");
#endif

    free(data);
    free(task_counts); // free(task_counts): 할당된 작업 수 배열 메모리 해제
    return EXIT_SUCCESS;
}

void update_task_count(int thread_index)
{
    task_counts[thread_index - 1]++;
}

void print_task_counts(int num_threads)
{
    for (int i = 0; i < num_threads; i++)
    {
        printf("Thread %d : %d tasks\n", i + 1, task_counts[i]);
    }
}

void merge_lists(double *a1, int n_a1, double *a2, int n_a2)
{
    double *a_m = (double *)calloc(n_a1 + n_a2, sizeof(double));
    int i = 0;

    int top_a1 = 0;
    int top_a2 = 0;

    for (i = 0; i < n_a1 + n_a2; i++)
    {
        if (top_a2 >= n_a2)
        {
            a_m[i] = a1[top_a1];
            top_a1++;
        }
        else if (top_a1 >= n_a1)
        {
            a_m[i] = a2[top_a2];
            top_a2++;
        }
        else if (a1[top_a1] < a2[top_a2])
        {
            a_m[i] = a1[top_a1];
            top_a1++;
        }
        else
        {
            a_m[i] = a2[top_a2];
            top_a2++;
        }
    }
    memcpy(a1, a_m, (n_a1 + n_a2) * sizeof(double));
    free(a_m);
}

void merge_sort(double *a, int n_a)
{
    if (n_a < 2)
        return;

    double *a1;
    int n_a1;
    double *a2;
    int n_a2;

    a1 = a;
    n_a1 = n_a / 2;

    a2 = a + n_a1;
    n_a2 = n_a - n_a1;

    merge_sort(a1, n_a1);
    merge_sort(a2, n_a2);

    merge_lists(a1, n_a1, a2, n_a2);
}
