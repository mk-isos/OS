CC=gcc
# 소스파일 이름
SRC=pmergesort.c
# 실행파일 이름
TARGET=pmergesort

all: $(TARGET)
	@echo "다음과 같은 형식으로 입력하시오."
	@echo "$$ ./$(TARGET) -d <데이터 수 입력> -t <스레드 수 입력>"

# 컴파일링(기본)
$(TARGET): $(SRC)
	$(CC) -o $(TARGET) $(SRC)

# 컴파일링(디버그)
debug:
	$(CC) -o $(TARGET) $(SRC) -DDEBUG
	@echo "<디버그모드>다음과 같은 형식으로 입력하시오."
	@echo "$$ ./$(TARGET) -d <데이터 수 입력> -t <스레드 수 입력>"

clean:
	rm -f $(TARGET)

.PHONY: all debug clean
