#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>   //  fork, exec, pipe
#include <sys/wait.h> // 자식 프로세스 상태 관리
#include <dirent.h>   // 디렉토리 엔트리를 다루는 함수들을 제공하는 헤더 파일
#include <fcntl.h>    // 파일 제어 관련 함수들을 정의하는 헤더 파일
#include <sys/time.h> // gettimeofday

#define BUFFER_SIZE 2048 // 효율적인 크기? 4096?

// 프로토타입 선언
int run_compiler(const char *source_code);
void setup_child_process(const char *input_path, int pipefd[], int error_pipefd[], int time_limit);
void process_parent_result(const char *expected_output_file_path, int pipefd[], int error_pipefd[], pid_t pid, struct timeval *start, struct timeval *end, int *Success_cnt, int *TimeLimit_cnt, int *RuntimeErr_cnt, int *WrongAnswer_cnt, int *total_tests);
void print_test_result(const char *result_name, int count, enum TestResult result_type);

// 성공, 시간 초과, 런타임 에러, 잘못된 결과를 표시하는 열거형 정의
enum TestResult
{
    TOTAL,
    SUCCESS,
    TIMEOUT,
    RUNTIME_ERROR,
    WRONG_ANSWER
};

// 소스 컴파일
int compile(const char *source_code)
{
    pid_t pid = fork(); // 자식 프로세스를 생성

    if (pid == -1)
    {
        // fork() 실패시 오류 메시지 출력
        perror("Fork failed");
        return -1;
    }

    if (pid == 0) // 자식 프로세스
    {
        dup2(fileno(stdout), fileno(stderr));

        // 소스 코드 컴파일: gcc 실행
        execlp("gcc", "gcc", "-fsanitize=address", source_code, "-o", "student_code", NULL);
        perror("Execlp failed"); // execlp() 실패시 오류 메시지 출력
        return -1;
    }
    else
    {
        // 부모 프로세스
        int status;
        waitpid(pid, &status, 0); // 자식 프로세스의 종료 대기

        // 자식 프로세스 정상적 종료 확인
        if (WIFEXITED(status))
        {
            if (WEXITSTATUS(status) == 0) // 종료 상태 0인지 확인
            {
                return 0; // 컴파일 성공
            }
            else
            {
                printf("Compile failed\n");
                return -1; // 컴파일 실패
            }
        }
        else
        {
            printf("Child process did not exit normally\n"); // 자식 프로세스가 정상적으로 종료되지 않았을 때
            return -1;
        }
    }
}

// 출력 값이랑 답 비교
int evaluate_results(const char *expected_output_file_path, const char *output)
{
    FILE *fp = fopen(expected_output_file_path, "r"); // 읽기모드 파일 열기
    if (!fp)
    {
        perror("failed to open answer_file"); // 파일열기 실패 시 오류 메시지 출력
        return -1;                            // 파일열기 실패
    }

    char expected_char;
    size_t index = 0;

    // 파일에서 한문자씩 읽어서 실제 출력된 데이터와 비교
    while ((expected_char = fgetc(fp)) != EOF)
    {
        if (expected_char != output[index])
        {
            fclose(fp);
            return -1; // 불일치 -1
        }
        index++;
    }

    // 파일에서 읽은 데이터의 길이와 출력된 데이터의 길이가 같은지 확인
    if (index != strlen(output))
    {
        fclose(fp);
        return -1; // 데이터 길이 불일치 -1
    }

    fclose(fp);
    return 0; // 문자, 길이 일치
}

// 학생 코드를 실행하여 결과를 평가하는 함수
void evaluate_submission(const char *input_path, const char *expected_output_file_path, int time_limit, int *Success_cnt, int *TimeLimit_cnt, int *RuntimeErr_cnt, int *WrongAnswer_cnt, int *total_tests)
{
    struct timeval start, end;      // 실행 시간 측정을 위한 구조체
    int pipefd[2], error_pipefd[2]; // 두 개의 파이프를 생성
    pipe(pipefd);
    pipe(error_pipefd); // 에러를 위한 파이프 생성
    pid_t pid = fork(); // fork() 사용 새로운 프로세스를 생성

    if (pid == -1)
    {
        perror("Fork failed"); // fork 실패
        exit(EXIT_FAILURE);    // 프로그램을 종료
    }

    else if (pid == 0)
    {
        // 자식 프로세스
        setup_child_process(input_path, pipefd, error_pipefd, time_limit);
    }
    else
    {
        // 부모 프로세스
        gettimeofday(&start, NULL); // 자식프로세스 시작 시간 기록
        process_parent_result(expected_output_file_path, pipefd, error_pipefd, pid, &start, &end, Success_cnt, TimeLimit_cnt, RuntimeErr_cnt, WrongAnswer_cnt, total_tests);
    }
}

// 자식 프로세스 설정 및 실행
void setup_child_process(const char *input_path, int pipefd[], int error_pipefd[], int time_limit)
{
    close(pipefd[0]); // 부모 프로세스의 입력 측 파이프 닫기

    // 표준 출력을 파이프로 리다이렉션
    dup2(pipefd[1], STDOUT_FILENO); // 파이프로 연결
    close(pipefd[1]);               // 사용 완료 후 닫기

    close(error_pipefd[0]); // 부모 프로세스의 입력 측 파이프 닫기

    // 표준 에러를 에러 파이프로 리다이렉션
    dup2(error_pipefd[1], STDERR_FILENO); // 에러 파이프로 연결
    close(error_pipefd[1]);               // 사용 완료 후 닫기

    // 입력 파일을 표준 입력으로 리다이렉트
    int input_fd = open(input_path, O_RDONLY); // 입력 파일 열기
    if (input_fd < 0)
    {
        perror("failed to open input_file"); // 파일 열기 실패
        exit(EXIT_FAILURE);
    }
    dup2(input_fd, STDIN_FILENO); // 입력 파일을 표준 입력에 연결
    close(input_fd);              // 파일 디스크립터 닫기

    alarm(time_limit); // 실행 시간 제한을 설정

    // 학생 코드를 실행
    execl("./student_code", "./student_code", (char *)NULL);
    perror("execl failed");
    exit(EXIT_FAILURE);
}

// AddressSanitizer에 의해 오류가 발생한 경우를 확인하는 함수
int is_ASan_triggered(const char *error_output)
{
    return strstr(error_output, "AddressSanitizer") != NULL;
}

// 부모 프로세스에서 결과 처리
void process_parent_result(const char *expected_output_file_path, int pipefd[], int error_pipefd[], pid_t pid, struct timeval *start, struct timeval *end, int *Success_cnt, int *TimeLimit_cnt, int *RuntimeErr_cnt, int *WrongAnswer_cnt, int *total_tests)
{
    gettimeofday(end, NULL); // 자식프로세스 시작 시간 기록
    close(pipefd[1]);        // 자식 프로세스의 출력 파이프 닫기
    close(error_pipefd[1]);  // 자식 프로세스의 에러 출력 파이프 닫기

    char output[BUFFER_SIZE] = {0};                                  // 프로세스의 출력을 저장할 버퍼
    ssize_t count_output = read(pipefd[0], output, BUFFER_SIZE - 1); // 파이프로부터 출력을 읽기
    output[count_output] = '\0';                                     // 문자열 종료 처리
    close(pipefd[0]);                                                // 사용 완료 후 닫기

    char buffer[BUFFER_SIZE]; // 에러 출력을 저장할 버퍼
    ssize_t count;            // 읽은 바이트 수

    // 파이프로부터 에러 출력 읽기 및 콘솔 출력
    while ((count = read(error_pipefd[0], buffer, BUFFER_SIZE - 1)) > 0)
    {
        buffer[count] = '\0'; // 문자열 종료 처리
        printf("%s", buffer); // 콘솔에 에러 메시지 출력
    }
    close(error_pipefd[0]); // 사용 완료 후 파이프 닫기

    int status;    // 자식 프로세스의 상태 저장 변수
    wait(&status); // 자식 프로세스가 종료될 때까지 대기

    // 실행시간계산
    long seconds = end->tv_sec - start->tv_sec;
    long micros = end->tv_usec - start->tv_usec;
    double excute_time = seconds + micros * 1e-6;

    // 자식 프로세스가 시그널에 의해 종료, 시그널이 SIGALRM이면 Timeout으로 간주
    if (WIFSIGNALED(status) && WTERMSIG(status) == SIGALRM)
    {
        printf("Result : Time out\n");
        (*TimeLimit_cnt)++; // main함수에 정의한 카운터 값 조정
    }

    // 자식 프로세스가 AddressSanitizer로 인한 오류 발생한 경우
    else if (is_ASan_triggered(buffer))
    {
        printf("Result : Runtime Error\n");
        (*RuntimeErr_cnt)++;
    }

    // 예상 출력과 학생 코드의 출력이 일치 하는지 확인
    else if (evaluate_results(expected_output_file_path, output) == 0)
    {
        printf("Result : Correct answer!\n");
        printf("excute Time : %f 초 \n", excute_time);
        (*Success_cnt)++;
    }
    // 예상 출력과 학생 코드의 출력이 일치하지 않는 경우
    else
    {
        printf("Result : Wrong Answer\n");
        (*WrongAnswer_cnt)++;
    }
}

// 디렉토리에서 파일을 읽고 테스트를 수행하는 함수
void test_files_in_directory(DIR *dir, const char *input_dir, const char *answer_dir, int time_limit, int *Success_cnt, int *TimeLimit_cnt, int *RuntimeErr_cnt, int *WrongAnswer_cnt, int *total_tests)
{
    struct dirent *entry;                 // 디렉토리 엔트리 구조체 선언
    char input_path[1024];                // 입력 파일 경로 저장 배열
    char expected_output_file_path[1024]; // 예상 출력 파일 경로 저장 배열

    // 디렉토리 안의 파일을 하나씩 테스트
    for (entry = readdir(dir); entry != NULL; entry = readdir(dir))
    {
        if (entry->d_type == DT_REG)
        {
            snprintf(input_path, sizeof(input_path), "%s/%s", input_dir, entry->d_name);                                // 입력 파일 경로 생성
            snprintf(expected_output_file_path, sizeof(expected_output_file_path), "%s/%s", answer_dir, entry->d_name); // 예상 출력 파일 경로 생성
            printf("=======================\n");
            printf("Test file : %s\n", entry->d_name);                                                                                                         // 파일이름출력                                                                                                    // 테스트한 파일 이름과 // 카운터 포인터들 run_and_compare로 전달
            evaluate_submission(input_path, expected_output_file_path, time_limit, Success_cnt, TimeLimit_cnt, RuntimeErr_cnt, WrongAnswer_cnt, total_tests); // 테스트 결과 출력
            (*total_tests)++;
        }
    }
}

// 입력 디렉토리에서 파일을 읽고 테스트를 수행하는 함수
void test_input_files(const char *source_code_file, const char *input_dir, const char *answer_dir, int time_limit, int *Success_cnt, int *TimeLimit_cnt, int *RuntimeErr_cnt, int *WrongAnswer_cnt, int *total_tests)
{
    DIR *dir; // 디렉토리 포인터 선언

    // 입력 디렉토리 열기
    if (!(dir = opendir(input_dir)))
    {
        perror("Failed to open input directory");
        exit(EXIT_FAILURE);
    }

    // 디렉토리 안의 파일 테스트 수행
    test_files_in_directory(dir, input_dir, answer_dir, time_limit, Success_cnt, TimeLimit_cnt, RuntimeErr_cnt, WrongAnswer_cnt, total_tests);

    closedir(dir);
}

// 결과 출력 함수
void print_test_results(int success_cnt, int timeout_cnt, int runtime_err_cnt, int wrong_answer_cnt, int total_tests)
{
    printf("\n==========================\n");
    printf("       Test Results\n");
    printf("==========================\n");
    color("Total tests", total_tests, TOTAL);
    color("Success", success_cnt, SUCCESS);
    color("Timeout", timeout_cnt, TIMEOUT);
    color("Runtime Error", runtime_err_cnt, RUNTIME_ERROR);
    color("Wrong Answer", wrong_answer_cnt, WRONG_ANSWER);
    printf("==========================\n\n");
}

// 결과 상세 내용 출력 함수
void color(const char *result_name, int count, enum TestResult result_type)
{
    const char *color_code = "\033[0m"; // 기본 색상

    // 결과 유형에 따라 색상 지정
    switch (result_type)
    {
    case TOTAL:
        color_code = "\033[0;30m"; // 검정색
        break;
    case SUCCESS:
        color_code = "\033[0;32m"; // 녹색
        break;
    case TIMEOUT:
        color_code = "\033[0;33m"; // 노란색
        break;
    case RUNTIME_ERROR:
        color_code = "\033[0;31m"; // 빨간색
        break;
    case WRONG_ANSWER:
        color_code = "\033[0;35m"; // 자홍색
        break;
    default:
        break;
    }
    printf("%s%s: %d\033[0m\n", color_code, result_name, count); // 색상을 적용하여 출력
}

int main(int argc, char *argv[])
{
    int Success_cnt = 0;     // 성공한 테스트 수를 저장하는 변수
    int TimeLimit_cnt = 0;   // 시간 초과로 실패한 테스트 수를 저장하는 변수
    int RuntimeErr_cnt = 0;  // 런타임 에러로 실패한 테스트 수를 저장하는 변수
    int WrongAnswer_cnt = 0; // 잘못된 결과로 실패한 테스트 수를 저장하는 변수
    int total_tests = 0;

    char *input_dir = NULL;        // 입력 파일이 있는 디렉토리 경로를 저장하는 포인터
    char *answer_dir = NULL;       // 정답 파일이 있는 디렉토리 경로를 저장하는 포인터
    int time_limit = 0;            // 각 테스트의 시간 제한을 나타내는 변수
    char *source_code_file = NULL; // 컴파일할 소스 코드 파일의 경로를 저장하는 포인터
    int opt_arg;                   // getopt 함수로부터 반환된 옵션 값 저장 변수

    // getopt 함수를 사용하여 명령행 인자 처리
    while ((opt_arg = getopt(argc, argv, "i:a:t:")) != -1)
    {
        if (opt_arg == 'i')
        {
            input_dir = optarg;
        }
        else if (opt_arg == 'a')
        {
            answer_dir = optarg;
        }
        else if (opt_arg == 't')
        {
            time_limit = atoi(optarg);
        }
        else
        {
            fprintf(stderr, "usage: %s -i <inputdir> -a <outputdir> -t <timelimit> <target src>\n", argv[0]);
            return -1;
        }
    }

    // 소스 코드 파일의 경로 확인
    if (optind < argc)
    {
        source_code_file = argv[optind];
    }
    else
    {
        fprintf(stderr, "not correct argument\n");
        return -1;
    }

    // 소스 코드 컴파일 및 테스트 실행
    if (compile(source_code_file) == 0)
    {
        test_input_files(source_code_file, input_dir, answer_dir, time_limit, &Success_cnt, &TimeLimit_cnt, &RuntimeErr_cnt, &WrongAnswer_cnt, &total_tests);
    }
    else
    {
        exit(EXIT_FAILURE); // 컴파일 실패
    }

    // 테스트 결과 출력
    print_test_results(Success_cnt, TimeLimit_cnt, RuntimeErr_cnt, WrongAnswer_cnt, total_tests);

    return 0; // 프로그램 종료
}