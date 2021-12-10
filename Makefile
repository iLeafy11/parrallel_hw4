CC:=gcc -Wall -pthread
obj:=pi eval

bmp: tpool.o main.c
	$(CC) tpool.o main.c -o pi -lm

test:
	valgrind --leak-check=full --show-leak-kinds=all -s ./pi 

sanitizer: tpool.o main.c
	$(CC) -g tpool.o main.c -o pi -lm -fsanitize=thread 

clean:
	rm -f *.o $(obj)
