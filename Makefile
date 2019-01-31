
all: mandel mandelmovie

mandel: mandel.o bitmap.o
	gcc -Wall -lm mandel.o bitmap.o -o mandel -lpthread

mandelmovie: mandelmovie.o
	gcc -Wall -lm mandelmovie.o -o mandelmovie

mandel.o: mandel.c
	gcc -Wall -lm -g -c mandel.c -o mandel.o

bitmap.o: bitmap.c
	gcc -Wall -lm -g -c bitmap.c -o bitmap.o

mandelmovie.o: mandelmovie.c
	gcc -Wall -lm -g -c mandelmovie.c -o mandelmovie.o

clean:
	rm -f mandel.o bitmap.o mandel mandelmovie.o mandelmovie
