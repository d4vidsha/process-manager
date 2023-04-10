# Process Manager

In this project, we implement a _process manager_ capable of allocating memory to processes and scheduling them for execution. The process scheduling and memory allocation are simulated. There is a _challenge task_ that requires controlling real processes and relies on interprocess communication system calls such as `pipe`, `fork`, `dup2` and `exec`. We will assume only one process is running at a time, i.e. a single-core CPU. 

Both the memory manager and the process queues are implemented as linked lists. More specifically, the memory manager is implemented as a linked list of _memory blocks_ (`block_t`) and the process queues are implemented as linked lists of _process control blocks_ (`pcb_t`).

## Modules

- `main`: the main program including the process manager
- `process`: used to simulate real processes
- `linkedlist`: implementation for storing any data type
- `memorymanager`: the memory manager API
- `pcb`: the process control block API
- `process-api`: API that controls `process`

## How to compile

```bash
make            # compile the main program
make process    # compile process executable, used to simulate real processes
```

## Options

All options are required.

- `-f <file>`: the file containing the processes to be managed
- `-s <scheduler>`: the scheduler to use. Can be `SJF` or `RR`
- `-m <memory>`: the memory allocation algorithm to use. Can be `infinite` or `best-fit`
- `-q <quantum>`: the quantum of each cycle

## Run test cases

Copy and paste any or all commands into the terminal to run the test cases. No output indicates that the test case/s passed.

```bash
./allocate -f cases/task1/simple.txt -s SJF -m infinite -q 1 | diff - cases/task1/simple-sjf.out
./allocate -f cases/task1/more-processes.txt -s SJF -m infinite -q 3 | diff - cases/task1/more-processes.out

./allocate -f cases/task2/simple.txt -s RR -m infinite -q 3 | diff - cases/task2/simple-rr.out
./allocate -f cases/task2/two-processes.txt -s RR -m infinite -q 1 | diff - cases/task2/two-processes-1.out
./allocate -f cases/task2/two-processes.txt -s RR -m infinite -q 3 | diff - cases/task2/two-processes-3.out

./allocate -f cases/task3/simple.txt -s SJF -m best-fit -q 3 | diff - cases/task3/simple-bestfit.out
./allocate -f cases/task3/non-fit.txt -s SJF -m best-fit -q 3 | diff - cases/task3/non-fit-sjf.out
./allocate -f cases/task3/non-fit.txt -s RR -m best-fit -q 3 | diff - cases/task3/non-fit-rr.out

./allocate -f cases/task4/spec.txt -s SJF -m infinite -q 3 | diff - cases/task4/spec.out
./allocate -f cases/task1/more-processes.txt -s SJF -m infinite -q 3 | diff - cases/task1/more-processes.out
./allocate -f cases/task2/simple.txt -s RR -m infinite -q 3 | diff - cases/task2/simple-rr.out

./allocate -f cases/task1/simple.txt -s SJF -m infinite -q 1 | diff - cases/task1/simple-sjf.out
./allocate -f cases/task2/two-processes.txt -s RR -m infinite -q 3 | diff - cases/task2/two-processes-3.out
```

## About

This project is part of the course [Computer Systems](https://handbook.unimelb.edu.au/2023/subjects/comp30023) at the University of Melbourne.