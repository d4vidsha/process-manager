EXE=allocate

$(EXE): main.c
	gcc -Wall -o $(EXE) $<

format:
	clang-format -style=file -i *.c

clean:
	rm -f $(EXE)