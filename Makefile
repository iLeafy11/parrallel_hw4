CC:=gcc -Wall -pthread
obj:=smooth a.out

bmp: tpool.o main.c
	$(CC) tpool.o main.c -o smooth -lm

test:
	valgrind --leak-check=full --show-leak-kinds=all -s ./smooth

sanitizer: tpool.o main.c
	$(CC) -g tpool.o main.c -o smooth -lm -fsanitize=thread 
	./smooth

clean:
	rm -f *.o $(obj)
