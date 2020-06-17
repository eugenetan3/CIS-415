CFLAGS= -W -Wall

PROGRAM = pseudo-shell

OBJECTS = main.o command.o

target: $(PROGRAM)

pseudo-shell: command.o main.o
	gcc -o pseudo-shell command.o main.o

command.o: command.c command.h

main.o: main.c

clean:
	rm -f $(OBJECTS) $(PROGRAM)
