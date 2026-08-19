# ShellForge - Unix/Linux Command Interpreter (Review 2)

**ShellForge** is a lightweight, educational Unix/Linux command interpreter written in C. It demonstrates core Operating Systems concepts including process creation, program execution, parent-child synchronization, POSIX built-in command handling, and tokenization.

---

## 1. Review 2 Scope & Features

This version implements the **Review 2 (Phase 1, ~50%)** milestone:

- [x] **Shell Startup & Interactive Prompt**: Clean welcome banner and `ShellForge> ` REPL prompt.
- [x] **Input Handling**: Safe string reading using `fgets()` with graceful EOF (Ctrl+D) handling.
- [x] **Basic Tokenization/Parser**: Whitespace-based splitting into command and argument tokens.
- [x] **Built-in `cd`**: Working directory changes executed directly in the parent process via `chdir()`.
- [x] **Built-in `pwd`**: Current working directory printing via `getcwd()`.
- [x] **Built-in `exit`**: Graceful shell shutdown via `exit()`.
- [x] **External Command Execution**: Standard Unix process lifecycle using `fork()`, `execvp()`, and `waitpid()`.
- [x] **Error Handling**: Friendly error messages for unknown commands, invalid directories, and system call failures.
- [x] **Review 3 Guards**: Detection of advanced operators (`|`, `<`, `>`, `>>`, `&`, `;`) reporting `"ShellForge: Feature not implemented yet."`
- [x] **Modular C Codebase**: Separation into `main.c`, `parser.c`, `builtins.c`, `executor.c`.
- [x] **Makefile**: Standard `make` and `make clean` targets.

---

## 2. Project Directory Structure

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

## 3. Building & Running ShellForge

### Compilation
To compile the shell using `gcc`:
```bash
make
```

### Running
```bash
./shellforge
```

### Cleaning Build Artifacts
```bash
make clean
```

---

## 4. How ShellForge Works Internally

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

## 5. Key Operating System Concepts for Review 2 Viva

### A. Why must `cd` be a built-in command?
In Unix/Linux, each process maintains its own Current Working Directory (CWD) in its PCB (Process Control Block).
- If `cd` were executed as an external command, `fork()` would create a child process.
- The child process would change its own working directory using `chdir()` and then terminate.
- The parent process (ShellForge) would remain in the old directory.
- Therefore, `cd` **must** be executed directly by the parent shell process.

### B. Process Creation (`fork`)
- `fork()` duplicates the calling process, creating an exact child process with its own address space, file descriptor table, and PID.
- `fork()` returns `0` in the child and the child's `pid` in the parent.

### C. Program Execution (`execvp`)
- `execvp()` replaces the memory space, code, stack, and heap of the child process with the new binary image (e.g. `/bin/ls`).
- If `execvp()` succeeds, it never returns. If it returns `-1`, the execution failed (e.g. command not found).

### D. Parent-Child Synchronization (`waitpid`)
- The parent calls `waitpid(pid, &status, 0)` to block and wait until the child finishes.
- This prevents the child from becoming a **Zombie Process** (an exited process whose termination status has not been read by its parent) and ensures sequential execution.

---

## 6. Verification & Test Cases

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

## 7. Roadmap for Review 3 (Phase 2)
The following advanced features are intentionally deferred to Review 3:
- I/O Redirection (`<`, `>`, `>>`) via `dup2()`
- Inter-Process Communication via Pipes (`|`) using `pipe()`
- Background Execution (`&`) and asynchronous job control
- Signal Handling (`SIGINT`, `SIGTSTP`)
- Command History & Arrow Key Navigation
