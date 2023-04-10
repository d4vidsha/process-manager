# COMP30023 2023 Project 1

In this project, we implement a _process manager_ capable of allocating memory to proesses and scheduling them for execution. The process scheduling and memory allocation are simulated. There is a _challenge task_ that requires controlling real processes. We will assume only one process is running at a time, i.e. a single-core CPU.

## How to run

```bash
make            # compile the main program
make process    # compile process executable, used to simulate real processes
./allocate -f <file> -s <scheduler> -m <memory> -q <quantum>
```

### Options

- `-f <file>`: the file containing the processes to be managed
- `-s <scheduler>`: the scheduler to use. Can be `SJF` or `RR`
- `-m <memory>`: the memory allocation algorithm to use. Can be `infinite` or `best-fit`
- `-q <quantum>`: the quantum of each cycle

## Test cases

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
