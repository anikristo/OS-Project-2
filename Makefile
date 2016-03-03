
all:    t_indexgen pi

t_indexgen: 
	gcc -g -Wall -o t_indexgen t_indexgen.c -lpthread

pi: 
	gcc -g -Wall -o pi pi.c -lpthread

clean:
	rm -fr *~   t_indexgen pi
