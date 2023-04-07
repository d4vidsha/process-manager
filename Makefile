# define C compiler & flags
CC = gcc
CFLAGS = -Wall -g
# define libraries to be linked (for example -lm)
LDLIBS = 

# define sets of source files and object files
SRC = main.c pcb.c linkedlist.c memorymanager.c process-api.c
# OBJ is the same as SRC, just replace .c with .o
OBJ = $(SRC:.c=.o)

# define the executable names
EXE = allocate

# the first target
$(EXE): $(OBJ) 
	$(CC) $(CFLAGS) -o $(EXE) $(OBJ) $(LDLIBS)

process:
	gcc -Wall -g -o process process.c

format:
	clang-format -style=file -i *.c *.h

clean:
	rm -f $(OBJ) $(EXE)