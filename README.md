# multithreaded-brainfuck-c

Are you worried that your brainfuck code might not perform excellent when its serving thousands of users in production? Or are you trying to train highly complicated AI inside your brainfuck program?

Then look no further than multithreaded-brainfuck-c - a Brainfuck interpreter that allows writing Brainfuck programs with multithreading. It is written in C so you can expect highest performance for your brainfuck code!

This interpreter adds a new symbol "/" which will act like the "[fork(2)](https://man7.org/linux/man-pages/man2/fork.2.html)" system command and will create a new thread at the current position.

All brainfuck threads share the same data stack so you can work on the same values and share results.

# Symbols

multithreaded-brainfuck-c supports all standard brainfuck symbol plus some more to support multithreading:

- `/`: Fork the current thread. Read the next chapter to learn more about this
- `%`: End child processes. Read the next chapter to learn more about this
- `#`: No-Op. This character does nothing
- `!`: Wait 1 second. This allows you to pause the thread for testing

# Installation and usage

1. Clone this repository
2. Compile the c program using `gcc -o bf-multi bf-multi.c`
3. Create a file that contains your brainfuck code or use one of the supplied example files
4. Run `./bf-multi [filename]` (e.g. `./bf-multi examples/multi-hello.bf`) to execute the file

Additionally you can add "d" as a second argument to use debug mode:
```
./bf-multi examples/multi-hello.bf d
```
This will output what each thread is doing.

# Forking

You can use the `/` symbol to fork the program at the current position.

This will continue executing the program normally on the main thread but will **skip 20 symbols** on the forked thread. In these 20 characters you can add your code that may allow you to distinguish between whether you are on the main thread or on the child thread.

If you don't have code to fill all 20 characters, you can add the Noop character (`#`) to fill remaining characters.

You can also add the "%" symbol if you are done with computing on your children. When a child process (i.e. all processes except the main thread) sees this symbol in its code, it will end itself. 

Example:
```bf
++>+++++.     This code executes before the fork command and thus only runs on the single main thread

/             We will fork the program at this exact position

##########>>>>+++###    These 20 characters will only execute on the main thread as the child thread will skip 20 characters

.++++++..+++  These symbols will be executed on both the main and child process

%             All threads except the main thread will exit when executing this symbol
>>.....       due to this this last line will only execute on the main thread as all other threads have exited
```

You can look into the examples folder for example code using multiple threads.

# Credits

This project is based on [brainfuck-c](https://github.com/kgabis/brainfuck-c) and adds the additional multithreading support to that.

# License

This project is licensed under the MIT License