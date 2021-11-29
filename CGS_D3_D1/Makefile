all: shell clean-o

shell: shell.o filesys.o
	gcc -o shell shell.o filesys.o

shell.o: shell.c
	gcc -c shell.c

mymemory.o: filesys.c
	gcc -c filesys.c

clean-o:
	rm -f *.o
	
clean:
	rm -f shell
	rm -f *.o