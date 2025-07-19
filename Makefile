CFLAGS=-std=c11 -g -static
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

9cc: $(OBJS)
	$(CC) -o 9cc $(OBJS) $(LDFLAGS)

$(OBJS): 9cc.h

# 外部で関数を定義したもののオブジェクトファイルを生成
test_func.o: test_func.c
	$(CC) $(CFLAGS) -c test_func.c

test: 9cc test_func.o
	./test.sh
	
clean:
	rm -f 9cc *.o *~ tmp*

.PHONY: test clean