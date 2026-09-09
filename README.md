# ShellForge - Unix/Linux Command Interpreter

**ShellForge** is a modular, educational Unix/Linux command interpreter written in C adhering strictly to POSIX standards (`C99`, `-pedantic`). It demonstrates core Operating Systems concepts including process creation and lifecycle management, parent-child synchronization, inter-process communication (IPC) via anonymous pipes, I/O stream redirection, background asynchronous job execution, POSIX signal handling, and in-memory command history tracking.

---

# PART I: REVIEW 2 (Phase 1 Baseline)

## 1. Review 2 Scope & Features

This section preserves the complete **Review 2 (Phase 1, ~50%)** milestone:

- [x] **Shell Startup & Interactive Prompt**: Clean welcome banner and `ShellForge> ` REPL prompt.
- [x] **Input Handling**: Safe string reading using `fgets()` with graceful EOF (`Ctrl+D`) handling.
- [x] **Basic Tokenization/Parser**: Whitespace-based splitting into command and argument tokens.
- [x] **Built-in `cd`**: Working directory changes executed directly in the parent process via `chdir()`.
- [x] **Built-in `pwd`**: Current working directory printing via `getcwd()`.
- [x] **Built-in `exit`**: Graceful shell shutdown via `exit()`.
- [x] **External Command Execution**: Standard Unix process lifecycle using `fork()`, `execvp()`, and `waitpid()`.
- [x] **Error Handling**: Friendly error messages for unknown commands, invalid directories, and system call failures.
- [x] **Review 3 Guards**: Detection of advanced operators (`|`, `<`, `>`, `>>`, `&`, `;`) with deferred notices.
- [x] **Modular C Codebase**: Separation into `main.c`, `parser.c`, `builtins.c`, `executor.c`.
- [x] **Makefile**: Standard `make` and `make clean` targets.

---

## 2. Review 2 Project Directory Structure

```
shellforge/
├── src/
│   ├── main.c          # Interactive REPL loop, startup banner, prompt
│   ├── parser.h        # Tokenizer constants & declarations
│   ├── parser.c        # Whitespace tokenization & unsupported feature guard
│   ├── builtins.h      # Declarations for built-in commands (cd, pwd, exit)
│   ├── builtins.c      # Implementations of cd, pwd, exit
│   ├── executor.h      # Declarations for process execution
│   └── executor.c      # fork(), execvp(), waitpid() process management
├── Makefile            # Build configuration
└── README.md           # Documentation & Viva presentation guide
```

---

## 3. Review 2 How ShellForge Works Internally

### Command Execution Lifecycle
```text
               User Enters Command Line
                          ↓
                [main.c] fgets()
                          ↓
             [parser.c] parse_input()
        (Tokenizes via whitespace delimiters)
                          ↓
           Is it an unsupported feature?
            ├── YES → Print "Feature not implemented yet."
            └── NO  ↓
           Is it a Built-in Command?
            ├── YES (cd, pwd, exit) → [builtins.c]
            │      • cd: chdir() in PARENT process
            │      • pwd: getcwd()
            │      • exit: exit(0)
            └── NO  (ls, echo, mkdir, cat, grep, ...)
                   ↓ [executor.c]
                  fork()
                   ├── Child (pid == 0)   → execvp(args[0], args)
                   └── Parent (pid > 0)  → waitpid(pid, &status, 0)
                          ↓
                 Return to prompt: ShellForge>
```

---

## 4. Key Operating System Concepts for Review 2 Viva

### A. Why must `cd` be a built-in command?
In Unix/Linux, each process maintains its own Current Working Directory (CWD) in its Process Control Block (PCB).
- If `cd` were executed as an external command, `fork()` would create a child process.
- The child process would change its own working directory using `chdir()` and then terminate.
- The parent process (ShellForge) would remain in the old directory.
- Therefore, `cd` **must** be executed directly by the parent shell process.

### B. Process Creation (`fork`)
- `fork()` duplicates the calling process, creating an exact child process with its own address space, file descriptor table, and PID.
- `fork()` returns `0` in the child and the child's `pid` in the parent.

### C. Program Execution (`execvp`)
- `execvp()` replaces the memory space, code, stack, and heap of the child process with the new binary image (e.g., `/bin/ls`).
- If `execvp()` succeeds, it never returns. If it returns `-1`, the execution failed (e.g., command not found).

### D. Parent-Child Synchronization (`waitpid`)
- The parent calls `waitpid(pid, &status, 0)` to block and wait until the child finishes.
- This prevents the child from becoming a **Zombie Process** (an exited process whose termination status has not been read by its parent) and ensures sequential execution.

---

## 5. Review 2 Verification & Test Cases

| Test Case | Input | Expected Output |
|---|---|---|
| **Print Working Directory** | `pwd` | Displays current directory path |
| **List Files** | `ls -l` | Detailed directory listing |
| **Echo Text** | `echo hello world` | `hello world` |
| **Directory Navigation** | `mkdir test`<br>`cd test`<br>`pwd`<br>`cd ..` | Navigates in and out of folder; working directory updates |
| **Invalid Command** | `abcxyz` | `ShellForge: command not found: abcxyz` |
| **Invalid Directory** | `cd nonexistent` | `ShellForge: cd: No such file or directory` |
| **Deferred Feature Guard** | `ls \| grep src` | `ShellForge: Feature not implemented yet.` |
| **Exit Shell** | `exit` | `Goodbye!` |

---

# PART II: REVIEW 3 (Phase 2 - Advanced Shell Features)

## 6. Review 3 Scope & Advanced Deliverables

All advanced features previously deferred in Review 2 are now **100% fully implemented and verified**:

- [x] **Pipeline Inter-Process Communication (`|`)**:
  - Single-stage (`cmd1 | cmd2`) and multi-stage arbitrary pipelines (`cmd1 | cmd2 | cmd3 | ...`).
  - Implemented via `pipe()`, `fork()`, `dup2()`, and rigorous descriptor closing across parent and child processes.
- [x] **Standard I/O Redirection (`<`, `>`, `>>`)**:
  - Input redirection (`<`) using `open(..., O_RDONLY)` and `dup2(fd, STDIN_FILENO)`.
  - Truncating output redirection (`>`) using `open(..., O_WRONLY | O_CREAT | O_TRUNC, 0644)` and `dup2(fd, STDOUT_FILENO)`.
  - Appending output redirection (`>>`) using `open(..., O_WRONLY | O_CREAT | O_APPEND, 0644)` and `dup2(fd, STDOUT_FILENO)`.
  - Works on external commands as well as parent-executed built-ins.
- [x] **Arbitrary Pipeline & Redirection Combinations**:
  - E.g., `cat < input.txt | grep keyword > output.txt`.
- [x] **Background Job Execution (`&`)**:
  - Executes jobs asynchronously without blocking the parent shell.
  - Immediately reports `[Background PID: <PID>]` and re-prompts the user.
- [x] **Asynchronous Zombie Process Reaping**:
  - Automatic non-blocking reaping of completed background children using `waitpid(-1, &status, WNOHANG)` before every prompt.
  - Prevents zombie accumulation in the OS process table and reports `[Process <PID> finished]`.
- [x] **POSIX Signal Handling (`SIGINT` / Ctrl+C)**:
  - Custom `sigaction()` handler prevents the parent shell from terminating on `Ctrl+C`.
  - Automatically forwards `SIGINT` via `kill(fg_pid, SIGINT)` to interrupt only the active foreground child.
- [x] **In-Memory Command History (`history`)**:
  - Dynamic in-memory list retaining previous user commands.
  - Numbered sequential 1-based display via `history` built-in command.
- [x] **Advanced Lexical Tokenizer & Syntax Validator**:
  - Supports spaced (`ls | grep .c`) and unspaced (`ls|grep .c`, `echo hello>file.txt`) operators.
  - Full single and double quote preservation (`"hello world"`, `'foo bar'`).
  - Comprehensive syntax checks reporting errors for trailing pipes (`ls |`) or missing redirection targets (`cat <`).

---

## 7. Review 3 Project Directory Structure

```
shellForge/
│
├── src/
│   ├── main.c          # Interactive REPL, SIGINT handler, background zombie reaper
│   ├── parser.h        # Command and Pipeline structs, parser function prototypes
│   ├── parser.c        # Lexical tokenizer, operator separation, syntax validator
│   ├── builtins.h      # Declarations for built-in commands (cd, pwd, exit, history)
│   ├── builtins.c      # Implementations of cd, pwd, exit, history (with stdout redirect support)
│   ├── executor.h      # Pipeline executor, stream redirection, and background reaper
│   ├── executor.c      # fork(), execvp(), waitpid(), pipe(), dup2(), open(), close()
│   ├── history.h       # In-memory history tracking declarations
│   └── history.c       # History list management, dynamic allocation, cleanup
│
├── Makefile            # C99 strict build (-Wall -Wextra -pedantic -D_POSIX_C_SOURCE=200809L)
└── README.md           # Unified Review 2 & Review 3 technical documentation & demo script
```

---

## 8. Review 3 Architecture & Process Lifecycles

### A. High-Level Architecture
```text
 +-------------------------------------------------------------+
 |                         ShellForge                          |
 |                 REPL (Read-Eval-Print Loop)                 |
 +-------------------------------------------------------------+
         |                                             |
         v                                             v
 [ Signals: SIGINT ]                         [ Background Reaper ]
 Prevents shell exit,                        waitpid(WNOHANG)
 forwards to fg child                        reaps finished jobs
         |                                             |
         +----------------------+----------------------+
                                |
                                v
                           fgets(stdin)
                                |
                                v
                      parse_pipeline(line)
           (Tokenizes words, |, <, >, >>, &, quotes)
                                |
                                v
                      history_add(line)
                                |
                                v
                    execute_pipeline(pipeline)
                                |
        +-----------------------+-----------------------+
        |                                               |
  [ Single Built-in ]                           [ External / Pipeline ]
  (cd, pwd, exit, history)                               |
  Executed directly in parent              +-------------+-------------+
  (Redirects stdout if requested)          |                           |
                                      Single External             N-Pipe Pipeline
                                       fork() + execvp()       pipe() * (N-1)
                                       apply_redirection()     fork() * N
                                       waitpid() (if fg)       dup2() + execvp()
                                                               waitpid() * N
```

### B. Pipeline IPC Mechanism (`command1 | command2`)
```text
                 ShellForge (Parent)
                          |
                    pipe(pipefds)
                          |
            +-------------+-------------+
            |                           |
         fork()                      fork()
            |                           |
        [Child 1]                   [Child 2]
  dup2(pipefds[1], STDOUT)    dup2(pipefds[0], STDIN)
  close(all pipefds)          close(all pipefds)
  execvp(command1)            execvp(command2)
            |                           |
            +=======> [PIPE BUFFER] ====+
                          |
                   Parent Process
               close(all pipefds in parent)
               waitpid(Child 1)
               waitpid(Child 2)
```

---

## 9. POSIX System Calls Guide for Review 3 Viva

| System Call | Prototype | Role in ShellForge | Where Used |
|---|---|---|---|
| `fork()` | `pid_t fork(void)` | Creates a new child process with an isolated address space and cloned file descriptor table. | `src/executor.c` |
| `execvp()` | `int execvp(const char *file, char *const argv[])` | Replaces current child image with target executable searched in `$PATH`. | `src/executor.c` |
| `waitpid()` | `pid_t waitpid(pid_t pid, int *status, int options)` | Blocks for foreground child completion; called with `WNOHANG` to reap background zombies without blocking. | `src/executor.c` |
| `pipe()` | `int pipe(int pipefd[2])` | Creates a unidirectional kernel data buffer (`pipefd[0]` for read, `pipefd[1]` for write). | `src/executor.c` |
| `dup2()` | `int dup2(int oldfd, int newfd)` | Atomically duplicates file descriptor `oldfd` onto `newfd` (`STDIN_FILENO` or `STDOUT_FILENO`). | `src/executor.c`, `src/builtins.c` |
| `open()` | `int open(const char *path, int flags, mode_t mode)` | Opens file descriptors with `O_RDONLY`, `O_WRONLY \| O_CREAT \| O_TRUNC`, or `O_APPEND`. | `src/executor.c` |
| `close()` | `int close(int fd)` | Closes file descriptors to flush data, release kernel references, and trigger `EOF` on pipes. | `src/executor.c` |
| `chdir()` | `int chdir(const char *path)` | Updates current working directory of the shell parent process directly. | `src/builtins.c` |
| `sigaction()` | `int sigaction(int signum, const struct sigaction *act, ...)` | Configures signal handling for `SIGINT` (Ctrl+C) with `SA_RESTART` flag. | `src/main.c` |
| `kill()` | `int kill(pid_t pid, int sig)` | Dispatches POSIX interrupt signals to the active foreground child process. | `src/main.c` |

---

## 10. Review 3 Viva Q&A Quick Reference

1. **Why must `pipe()` be called before `fork()`?**
   - *Answer*: `pipe()` creates the kernel buffer and allocates two descriptors in the parent's table. When `fork()` is called subsequently, the child processes inherit clones of those descriptors pointing to the same pipe buffer, establishing the IPC channel.
2. **Why must unused pipe ends be closed in all processes?**
   - *Answer*: A pipe reader receives `EOF` only when **all** write descriptors across all processes are closed. If parent or children leave write ends open, the reader blocks indefinitely waiting for more input.
3. **How does `dup2()` enable stream redirection?**
   - *Answer*: `dup2(fd, STDOUT_FILENO)` closes standard output and binds slot 1 to `fd`. Subsequent calls to `printf` or `write(1, ...)` go directly to the target file or pipe.
4. **How does ShellForge avoid background zombies?**
   - *Answer*: When a background job (`&`) finishes, it enters zombie state until reaped. ShellForge invokes `waitpid(-1, &status, WNOHANG)` non-blockingly before every interactive prompt, claiming the exit status and freeing the process table slot.
5. **How does Ctrl+C work without killing the shell?**
   - *Answer*: ShellForge handles `SIGINT` via `sigaction()`. If a foreground child is active, the handler forwards `SIGINT` to the child's PID via `kill()`. If idle at the prompt, it simply refreshes the prompt.

---

# PART III: DEMO SCRIPT & COMMANDS WALKTHROUGH

This section contains the **exact step-by-step set of commands** to run during the live demonstration. Each command includes a 1–2 line explanation of what it does, the underlying OS mechanism, and what output to expect.

### Step 1: Compilation & Shell Startup
```bash
make clean
make
./shellforge
```
- **Line 1 & 2 (`make clean && make`)**: Removes old object files and cleanly compiles all modules under strict POSIX `-std=c99 -Wall -Wextra -pedantic` flags.
- **Line 3 (`./shellforge`)**: Launches the ShellForge interactive REPL and prints the welcome banner and prompt.

---

### Step 2: Review 2 Baseline (Built-in Commands & Navigation)
```bash
pwd
```
- **Explanation**: Calls the `pwd` built-in command which uses `getcwd()` in the parent process to print the current working directory.

```bash
echo "Testing ShellForge Review 3"
```
- **Explanation**: Executes the external `echo` command via `fork()`, `execvp()`, and `waitpid()`, demonstrating standard child process lifecycle and argument passing.

```bash
mkdir demo_dir
cd demo_dir
pwd
```
- **Explanation**: Creates a new folder, uses the parent-executed `cd` built-in (`chdir()`) to enter it, and verifies that the shell's working directory successfully changed.

```bash
cd ..
rmdir demo_dir
```
- **Explanation**: Navigates back up to the parent directory and removes the temporary demo folder, demonstrating clean directory management.

---

### Step 3: Single & Multi-Stage Pipelines (`|`)
```bash
ls src | grep .c
```
- **Explanation**: Creates an anonymous pipe via `pipe()`, redirects stdout of `ls src` into the pipe using `dup2()`, and redirects stdin of `grep .c` from the pipe to list all C source files.

```bash
ls src | grep .c | wc -l
```
- **Explanation**: Executes a 3-stage pipeline with two kernel pipes, concurrently running `ls`, `grep`, and `wc` to count the total number of C source files.

```bash
ls|grep parser
```
- **Explanation**: Demonstrates parser resilience by handling unspaced pipe operators (`|`) without requiring spaces between arguments.

---

### Step 4: File I/O Redirection (`>`, `>>`, `<`)
```bash
echo "System Calls Demo" > demo.txt
cat demo.txt
```
- **Explanation**: Overwrites standard output to `demo.txt` using `open(..., O_WRONLY | O_CREAT | O_TRUNC, 0644)` and `dup2()`, then displays the file contents.

```bash
echo "Appended Second Line" >> demo.txt
cat demo.txt
```
- **Explanation**: Appends output to `demo.txt` using `open(..., O_APPEND)` without overwriting previous content, showing multi-line file buildup.

```bash
cat < demo.txt
```
- **Explanation**: Redirects standard input from `demo.txt` using `open(..., O_RDONLY)` and `dup2(fd, STDIN_FILENO)` into `cat`.

---

### Step 5: Combined Pipelines and Redirection
```bash
ls src | grep .c > c_sources.txt
cat c_sources.txt
```
- **Explanation**: Routes the filtered pipeline output directly into a file, combining both multi-process IPC and file descriptor redirection simultaneously.

```bash
cat < c_sources.txt | grep main
```
- **Explanation**: Feeds an input-redirected file stream into a pipeline to search for `main`, demonstrating bidirectional redirection and piping in a single command.

---

### Step 6: Background Execution & Non-blocking Zombie Reaping (`&`)
```bash
sleep 3 &
pwd
```
- **Explanation**: Launches `sleep 3` asynchronously in the background, immediately returns control to the shell prompt, and allows the user to run `pwd` without waiting.

```bash
# Wait 3 seconds, then press Enter
```
- **Explanation**: Triggers the automatic `waitpid(-1, &status, WNOHANG)` zombie reaper before the next prompt, displaying `[Process <PID> finished]` and preventing zombie buildup.

---

### Step 7: Signal Handling (`SIGINT` / Ctrl+C)
```bash
sleep 10
# Press Ctrl+C while sleep is running
```
- **Explanation**: The parent shell intercepts `SIGINT` via `sigaction()` and forwards it to the active foreground child using `kill(child_pid, SIGINT)`, terminating the child while keeping the shell alive.

```bash
# Press Ctrl+C at the empty ShellForge prompt
```
- **Explanation**: Demonstrates that pressing `Ctrl+C` at an idle prompt does not crash or exit ShellForge; it prints a clean newline and redisplays the prompt.

---

### Step 8: In-Memory Command History (`history`)
```bash
history
```
- **Explanation**: Calls the `history` built-in command to display a numbered sequential list of all previously executed commands stored in memory.

---

### Step 9: Robust Error Handling & Syntax Diagnostics
```bash
invalidcommand123
```
- **Explanation**: Tests command resolution by attempting an invalid program name, cleanly returning `ShellForge: command not found: invalidcommand123`.

```bash
cd /folder_does_not_exist
```
- **Explanation**: Tests built-in error handling by attempting to navigate to an invalid path, reporting `ShellForge: cd: No such file or directory`.

```bash
cat < missing_file.txt
```
- **Explanation**: Validates input redirection error handling when a file is absent, producing `ShellForge: missing_file.txt: No such file or directory`.

```bash
ls |
```
- **Explanation**: Triggers syntax error diagnostics for an incomplete trailing pipeline operator, returning `ShellForge: syntax error near unexpected token '|'`.

```bash
cat >
```
- **Explanation**: Triggers syntax error diagnostics for a redirection operator with a missing destination filename, returning `ShellForge: syntax error near unexpected token 'newline'`.

---

### Step 10: Graceful Exit
```bash
exit
```
- **Explanation**: Gracefully terminates ShellForge via the `exit` built-in command, freeing allocated history buffers and returning code 0 to the parent environment.
