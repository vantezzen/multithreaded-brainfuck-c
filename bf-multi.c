/**
 * Multithreaded Brainfuck Interpreter in C
 * For your most performance-sensitive Brainfuck application
 * 
 * This file contains code from https://github.com/kgabis/brainfuck-c/blob/master/brainfuck.c
 * 
 * @author vantezzen
 * @license MIT
 */
#include <stdio.h> 
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/mman.h>

// Brainfuck Operations
#define OP_END          0
#define OP_INC_DP       1
#define OP_DEC_DP       2
#define OP_INC_VAL      3
#define OP_DEC_VAL      4
#define OP_OUT          5
#define OP_IN           6 
#define OP_JMP_FWD      7
#define OP_JMP_BCK      8
// Non-standard Operations
#define OP_THREAD       9
#define OP_NOOP         10
#define OP_WAIT         11
#define OP_CHILD_DIE    12

#define SUCCESS         0
#define FAILURE         1


// Sizes for the parts of our interpreter
#define PROGRAM_SIZE    4096
#define STACK_SIZE      512
#define DATA_SIZE       65535
#define CHILDREN_SIZE   20

// Operations for the execution stack
#define STACK_PUSH(A)   (STACK[SP++] = A)
#define STACK_POP()     (STACK[--SP])
#define STACK_EMPTY()   (SP == 0)
#define STACK_FULL()    (SP == STACK_SIZE)

struct instruction_t {
    unsigned short operator;
    unsigned short operand;
};

static struct instruction_t PROGRAM[PROGRAM_SIZE];
static unsigned short STACK[STACK_SIZE];
static unsigned int SP = 0;
static unsigned int CHILD = 0;
static pid_t children[CHILDREN_SIZE];

/**
 * Compile the Brainfuck characters to the array of operators
 */
int compile_bf(FILE* fp) {
    unsigned short pc = 0, jmp_pc;
    int c;

    while ((c = getc(fp)) != EOF && pc < PROGRAM_SIZE) {
        switch (c) {
            case '>': PROGRAM[pc].operator = OP_INC_DP; break;
            case '<': PROGRAM[pc].operator = OP_DEC_DP; break;
            case '+': PROGRAM[pc].operator = OP_INC_VAL; break;
            case '-': PROGRAM[pc].operator = OP_DEC_VAL; break;
            case '.': PROGRAM[pc].operator = OP_OUT; break;
            case ',': PROGRAM[pc].operator = OP_IN; break;
            case '/': PROGRAM[pc].operator = OP_THREAD; break;
            case '%': PROGRAM[pc].operator = OP_CHILD_DIE; break;
            case '#': PROGRAM[pc].operator = OP_NOOP; break;
            case '!': PROGRAM[pc].operator = OP_WAIT; break;
            case '[':
                PROGRAM[pc].operator = OP_JMP_FWD;
                if (STACK_FULL()) {
                    return FAILURE;
                }
                STACK_PUSH(pc);
                break;
            case ']':
                if (STACK_EMPTY()) {
                    return FAILURE;
                }
                jmp_pc = STACK_POP();
                PROGRAM[pc].operator =  OP_JMP_BCK;
                PROGRAM[pc].operand = jmp_pc;
                PROGRAM[jmp_pc].operand = pc;
                break;
            default: pc--; break;
        }
        pc++;
    }
    if (!STACK_EMPTY() || pc == PROGRAM_SIZE) {
        return FAILURE;
    }
    PROGRAM[pc].operator = OP_END;
    return SUCCESS;
}

/**
 * Execute the (previously compiles) program array 
 */
int execute_bf(bool isDebug) {
    unsigned short pc = 0;
    unsigned int ptr = DATA_SIZE;
    bool isChild = false;

    // Map data so we can share it across processes
    short *data = mmap(NULL, DATA_SIZE*sizeof(short), PROT_READ | PROT_WRITE, 
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    // Set all cells to 0
    while (--ptr) { data[ptr] = 0; }

    // Run commands
    while (PROGRAM[pc].operator != OP_END && ptr < DATA_SIZE) {
        switch (PROGRAM[pc].operator) {
            case OP_INC_DP: ptr++; break;
            case OP_DEC_DP: ptr--; break;
            case OP_INC_VAL: data[ptr]++; break;
            case OP_DEC_VAL: data[ptr]--; break;
            case OP_OUT: putchar(data[ptr]); break;
            case OP_IN: data[ptr] = (unsigned int)getchar(); break;
            case OP_JMP_FWD: if(!data[ptr]) { pc = PROGRAM[pc].operand; } break;
            case OP_JMP_BCK: if(data[ptr]) { pc = PROGRAM[pc].operand; } break;
            case OP_THREAD: 
            {
                // To make sure our compiler doesn't create too many forks,
                // we limit the number to 20 forks per child
                if (CHILD >= CHILDREN_SIZE) {
                    printf("Limit to 20 Children");
                    return FAILURE;
                }

                pid_t pid_fork = fork();
                if (pid_fork == 0) {
                    // We are now in the forked process
                    isChild = true;
                    // Skip 20 chars
                    pc += 20;

                    if (isDebug) {
                        printf("New child thread on PID %d\n", getpid());
                        fflush(stdout);
                    }
                } else {
                    CHILD++;

                    if (isDebug) {
                        printf("Main thread forked itself to %d", pid_fork);
                    }
                }
            }
            break;
            case OP_CHILD_DIE:
              {
                if (isChild) {
                    if (isDebug) {
                        printf("Dying");
                    }
                    // Flush output to make sure nothing gets lost
                    fflush(stdout);
                    _exit(0);
                }
              }
            break;
            case OP_WAIT:
              sleep(1);
            break;
            case OP_NOOP: break;
            default: return FAILURE;
        }

        if (isDebug) {
            printf("\nProcess ID %d at Counter %d with Operation %d and Pointer at %d", getpid(), pc, PROGRAM[pc].operator, ptr);
            fflush(stdout);
        }
        pc++;
    }

    if (isDebug) {
        printf("%d: %d %d %d %d %d %d %d %d\n", getpid(), data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
        printf("Final counter %d on ID %d\n", ptr, getpid());
    }

    // Flush output to make sure nothing gets lost
    fflush(stdout);

    return ptr != DATA_SIZE ? SUCCESS : FAILURE;
}

int main(int argc, const char * argv[])
{
    // Make sure the arguments are valid
    int status;
    FILE *fp;
    if (argc < 2 || (fp = fopen(argv[1], "r")) == NULL) {
        fprintf(stderr, "Usage: %s filename [debug]\n", argv[0]);
        return FAILURE;
    }

    // Compile our Brainfuck code
    status = compile_bf(fp);
    fclose(fp);

    if (status == SUCCESS) {
        // Run our brainfuck code and determine if we should use debug mode
        status = execute_bf(argc > 2 && argv[2][0] == 'd');
    }
    if (status == FAILURE) {
        fprintf(stderr, "Error!\n");
    }
    return status;
}