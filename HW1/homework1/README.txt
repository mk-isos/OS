README.

#프로그램 설명 및 사용법

프로그램 개요:
이 프로그램은 학생이 작성한 C 소스 코드를 컴파일하고, 주어진 입력 파일에 대해 실행하여 예상 출력과 비교하여 테스트하는 프로그램입니다. 테스트는 compile error, time out, runtime error, wrong answer 등의 상황에 대해 결과를 출력합니다.

사용법:
프로그램을 실행할 때는 명령줄에서 다음과 같은 옵션을 사용합니다.

$ ./autojudge -i <input_directory> -a <answer_directory> -t <time_limit> <source_code_file>
-i <input_directory>: 입력 파일이 있는 디렉토리의 경로를 지정합니다.
-a <answer_directory>: 예상 출력 파일이 있는 디렉토리의 경로를 지정합니다.
-t <time_limit>: 각 테스트의 시간 제한을 초 단위로 지정합니다.
<source_code_file>: 컴파일할 C 소스 코드 파일의 경로를 지정합니다.

주의사항:
입력 파일과 예상 출력 파일은 각각의 테스트 케이스에 대해 동일한 이름을 가져야 합니다.
컴파일된 실행 파일은 student_code라는 이름으로 생성됩니다.
컴파일 중 또는 실행 중에 발생한 오류는 콘솔에 출력됩니다.
프로그램 실행 후 결과는 total tests, correct, timeout, runtime error, wrong answer 형태로 출력됩니다.
total tests, correct, timeout, runtime error, wrong answer 등이 요약되어 출력됩니다.
Makefile을 작성하여서 "make"명령어를 실행하면 컴파일이 됩니다.
