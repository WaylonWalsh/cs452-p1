# Summary of Project

The  goal of this project was to create a basic shell implementation that supports various features such as command execution, built-in commands, job control, and background processes. The shell is designed to provide an interactive interface for users to execute commands and manage processes. I believe I have done a decent job of executing what was required here. 

The basic skeleton of the project we received is split into multiple files (lab.c, lab.h, main.c) The lab.c file contains the core shell functions, while main.c handles the main loop. In this basic shell there are several features we had to implement. The job control system uses an array of job structures to keep track of background processes. This allows the shell to manage multiple background jobs simultaneously and report their status to the user. The cmd_parse function uses dynamic memory allocation to handle commands of varying lengths. It splits the input string into an array of strings, making it easy to pass to execvp or process as built-in commands. The built-in commands such as 'exit', 'cd', 'history', 'pwd', and 'jobs' have been successfully implemented. The shell uses fork() to create new processes and execvp() to run external commands. The shell also ignores certain signals that were listed in task 8. This was done because ignoring certain signals in the parent shell and resetting them in child processes, the shell can maintain control over its environment while allowing normal signal behavior in executed programs. 

In this project we also used the GNU Readline library for input features like command history. In this project we also implemented it where the shell detects background processes by checking for the '&' symbol at the end of commands. It then manages these processes separately, allowing the user to continue interacting with the shell while background jobs run. Being written in C there is no garbage collector so the project pays careful attention to memory allocation and deallocation, using functions like cmd_free to prevent memory leaks. In the project we were provided a test file (test-lab.c) that verified a good portion of the functionality of various shell components but it was not comprehensive. 

The greatest difficulty I had with this project was testing since the test provided a good baseline of what our shell needed to do but it did not cover everything. I did a lot of manual testing to verify certain components were working. This took a lot of time and if i were to do this project again I would likely code some of these tests I was doing manually to save time. Another thing that gave me trouble was the memory management side of this project. I am not the best at it and it caused a lot of problems for me in this project that took a while to resolve. 

Overall I enjoyed This shell project. I feel I have a better understanding of process management, system calls, and user interface design in a Unix-like environment. It provides a solid foundation for understanding how shells work.


## To compile project in command line enter

```bash
make clean
```

```bash
make 
```

## To run shell 

```bash
./myprogram
```

## To run shell with custom prompt

```bash
MY_PROMPT="foo>" ./myprogram
```

## To check shell version

```bash
./myprogram -v
```

## To run test file

```bash
./test-lab
```

## DOCS

https://tiswww.cwru.edu/php/chet/readline/readline.html

https://www.gnu.org/software/libc/manual/html_node/Example-of-Getopt.html

https://tiswww.case.edu/php/chet/readline/rltop.html

https://man7.org/linux/man-pages/man3/getenv.3.html

https://en.wikipedia.org/wiki/End-of-file

https://man7.org/linux/man-pages/man3/exit.3.html

https://man7.org/linux/man-pages/man2/chdir.2.html

https://man7.org/linux/man-pages/man3/getenv.3.html

https://man7.org/linux/man-pages/man2/getuid.2.html

https://man7.org/linux/man-pages/man3/getpwuid.3p.html

https://tiswww.cwru.edu/php/chet/readline/history.html

https://man7.org/linux/man-pages/man3/exec.3.html

https://man7.org/linux/man-pages/man2/fork.2.html

https://man7.org/linux/man-pages/man2/wait.2.html

https://www.gnu.org/software/libc/manual/html_node/Initializing-the-Shell.html

https://www.gnu.org/software/libc/manual/html_node/Launching-Jobs.html

https://man7.org/linux/man-pages/man7/signal.7.html

https://www.gnu.org/software/libc/manual/html_node/Implementing-a-Shell.html