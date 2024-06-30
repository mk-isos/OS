#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include "stack.h"

#ifndef BOARD_SIZE
#define BOARD_SIZE 15
#endif

pthread_mutex_t print_lock = PTHREAD_MUTEX_INITIALIZER;
int solutions_found = 0;									// 현재까지 찾은 배치 수를 저장하는 전역 변수
pthread_mutex_t solutions_lock = PTHREAD_MUTEX_INITIALIZER; // 배치 수를 보호하기 위한 잠금 객체

typedef struct
{
	int N;
	struct stack_t *prep;
} thread_args;

int row(int cell)
{
	return cell / BOARD_SIZE;
}

int col(int cell)
{
	return cell % BOARD_SIZE;
}

int is_feasible(struct stack_t *queens)
{
	int board[BOARD_SIZE][BOARD_SIZE];
	int c, r;

	for (r = 0; r < BOARD_SIZE; r++)
	{
		for (c = 0; c < BOARD_SIZE; c++)
		{
			board[r][c] = 0;
		}
	}

	for (int i = 0; i < get_size(queens); i++)
	{
		int cell;
		get_elem(queens, i, &cell);

		int r = row(cell);
		int c = col(cell);

		if (board[r][c] != 0)
		{
			return 0;
		}

		for (int y = 0; y < BOARD_SIZE; y++)
		{
			board[y][c] = 1;
		}
		for (int x = 0; x < BOARD_SIZE; x++)
		{
			board[r][x] = 1;
		}

		for (int y = r + 1, x = c + 1; y < BOARD_SIZE && x < BOARD_SIZE; y++, x++)
		{
			board[y][x] = 1;
		}

		for (int y = r + 1, x = c - 1; y < BOARD_SIZE && x >= 0; y++, x--)
		{
			board[y][x] = 1;
		}

		for (int y = r - 1, x = c + 1; y >= 0 && x < BOARD_SIZE; y--, x++)
		{
			board[y][x] = 1;
		}

		for (int y = r - 1, x = c - 1; y >= 0 && x >= 0; y--, x--)
		{
			board[y][x] = 1;
		}
	}

	return 1;
}

void print_placement(struct stack_t *queens)
{
	pthread_mutex_lock(&print_lock);
	for (int i = 0; i < queens->size; i++)
	{
		int queen;
		get_elem(queens, i, &queen);
		printf("[%d,%d] ", row(queen), col(queen));
	}
	printf("\n");
	pthread_mutex_lock(&solutions_lock);
	solutions_found++; // 배치 수 증가
	pthread_mutex_unlock(&solutions_lock);
	pthread_mutex_unlock(&print_lock);
}

void *find_n_queens_with_prepositions_thread(void *arg)
{
	thread_args *args = (thread_args *)arg;
	int N = args->N;
	struct stack_t *prep = args->prep;

	struct stack_t *queens = create_stack(BOARD_SIZE);
	queens->capacity = prep->capacity;
	queens->size = prep->size;
	memcpy(queens->buffer, prep->buffer, prep->size * sizeof(int));

	while (prep->size <= queens->size)
	{
		int latest_queen;
		top(queens, &latest_queen);

		if (latest_queen == BOARD_SIZE * BOARD_SIZE)
		{
			pop(queens, &latest_queen);
			if (!is_empty(queens))
			{
				pop(queens, &latest_queen);
				push(queens, latest_queen + 1);
			}
			else
			{
				break;
			}
			continue;
		}

		if (is_feasible(queens))
		{
			if (get_size(queens) == N)
			{
				print_placement(queens);

				pop(queens, &latest_queen);
				push(queens, latest_queen + 1);
			}
			else
			{
				top(queens, &latest_queen);
				push(queens, latest_queen + 1);
			}
		}
		else
		{
			pop(queens, &latest_queen);
			push(queens, latest_queen + 1);
		}
	}

	delete_stack(queens);
	return NULL;
}

int find_n_queens_with_prepositions(int N, struct stack_t *prep, int num_threads)
{
	pthread_t threads[num_threads];
	thread_args args[num_threads];

	for (int i = 0; i < num_threads; i++)
	{
		args[i].N = N;
		args[i].prep = prep;
		pthread_create(&threads[i], NULL, find_n_queens_with_prepositions_thread, &args[i]);
	}

	for (int i = 0; i < num_threads; i++)
	{
		pthread_join(threads[i], NULL);
	}

	return 0;
}

void *find_n_queens_thread(void *arg)
{
	thread_args *args = (thread_args *)arg;
	int N = args->N;

	struct stack_t *queens = create_stack(BOARD_SIZE);
	push(queens, 0);

	while (!is_empty(queens))
	{
		int latest_queen;
		top(queens, &latest_queen);

		if (latest_queen == BOARD_SIZE * BOARD_SIZE)
		{
			pop(queens, &latest_queen);
			if (!is_empty(queens))
			{
				pop(queens, &latest_queen);
				push(queens, latest_queen + 1);
			}
			else
			{
				break;
			}
			continue;
		}

		if (is_feasible(queens))
		{
			if (get_size(queens) == N)
			{
				print_placement(queens);

				pop(queens, &latest_queen);
				push(queens, latest_queen + 1);
			}
			else
			{
				top(queens, &latest_queen);
				push(queens, latest_queen + 1);
			}
		}
		else
		{
			pop(queens, &latest_queen);
			push(queens, latest_queen + 1);
		}
	}

	delete_stack(queens);
	return NULL;
}

int find_n_queens(int N, int num_threads)
{
	pthread_t threads[num_threads];
	thread_args args[num_threads];

	for (int i = 0; i < num_threads; i++)
	{
		args[i].N = N;
		args[i].prep = create_stack(BOARD_SIZE);
		push(args[i].prep, 0);
		pthread_create(&threads[i], NULL, find_n_queens_thread, &args[i]);
	}

	for (int i = 0; i < num_threads; i++)
	{
		pthread_join(threads[i], NULL);
		delete_stack(args[i].prep);
	}

	return 0;
}

// 시그널 핸들러 함수
void signal_handler(int signum)
{
	pthread_mutex_lock(&solutions_lock);
	// 현재까지 찾은 배치 수를 출력
	printf("Solutions found so far: %d\n", solutions_found);
	pthread_mutex_unlock(&solutions_lock);
	exit(signum); // 프로그램 종료
}
int main(int argc, char *argv[])
{
	int N = 4;			 // 기본값 설정
	int num_threads = 4; // 기본 스레드 수 설정

	// 명령줄 인수 처리
	int opt;
	while ((opt = getopt(argc, argv, "n:t:")) != -1)
	{
		switch (opt)
		{
		case 'n':
			N = atoi(optarg);
			break;
		case 't':
			num_threads = atoi(optarg);
			break;
		default:
			fprintf(stderr, "Usage: %s [-n number_of_queens] [-t number_of_threads]\n", argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	// SIGINT 시그널에 대한 핸들러 설정 (Ctrl+C)
	signal(SIGINT, signal_handler);

	find_n_queens(N, num_threads);
	return EXIT_SUCCESS;
}
