all: batch_job code main.o 

batch_job: batch_job.o
	gcc -o batch_job batch_job.c

code: main.o 
	gcc -lm main.o -o aubatch -lpthread

main.o: main.c
	gcc -c main.c

clean:
	rm *.o aubatch


