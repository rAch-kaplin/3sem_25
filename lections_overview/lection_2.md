1. Куда сохраняется (регистровый) контекст процесса? -- на стек ядра!
```text
     +---------------------------+   <---- Bottom of Kernel Stack (High Address)
     |                           |
     |  Kernel function frames   | \
     |  (e.g., sys_call handler) |  |-- Grows downward as more kernel functions are called
     |                           | /
     +---------------------------+
     |        struct pt_regs     | \
     |---------------------------|  |
     |           SS              |  |
     |           SP              |  |
     |         RFLAGS            |  |
     |           CS              |  |-- The saved user process context.
     |           IP              |  |   Pushed by CPU and kernel entry code.
     |        ORIG_RAX           |  |
     |           R15             |  |
     |           ...             |  |
     |           RAX             |  |
     +---------------------------+   <---- Top of Kernel Stack after save (Low Address)
```
             Рис.1. Стек ядра

2. Пример struct pt_regs для x64:
```c
struct pt_regs {
    unsigned long r15;
    unsigned long r14;
    unsigned long r13;
    unsigned long r12;
    unsigned long bp;
    unsigned long bx;
    unsigned long r11;
    unsigned long r10;
    unsigned long r9;
    unsigned long r8;
    unsigned long ax;
    unsigned long cx;
    unsigned long dx;
    unsigned long si;
    unsigned long di;
    unsigned long orig_ax; // The original syscall number
    unsigned long ip;      // Instruction Pointer
    unsigned long cs;      // Code Segment
    unsigned long flags;   // CPU Flags (e.g., interrupt enable)
    unsigned long sp;      // Stack Pointer (user-space)
    unsigned long ss;      // Stack Segment
};
```
Рис. 2. Пример struct pt_regs

2. Сохранение на стек, пошагово:

2.1 Trap:

Пусть пользовательский код позвал системный вызов (например, write()). Обертка в glibc загружает номер вызова в rax и исполняет инструкцию вызова.

CPU:  user (ring 3) -->  kernel (ring 0).
Происходит переключение с пользовательского стека (%rsp) в стек ядра текущего процесса. Адрес стека хранится в PCB (struct task_struct).
CPU автоматически сохраняет часть контекста ("critical minimal state") до того, как начнет работать код ядра:

- user-space %rip (Instruction Pointer) so the kernel knows where to return.
- user-space %rsp (Stack Pointer) so it can switch back.
- %rflags register.
- %cs and %ss segment registers.


2.2. После чего передается управление на точку входа в обработчик вызова в ядре.
Далее ядро сохраняет остальные регистры в прологе обработчика вызова, и начинает обрабатывать вызов.



3. Восстановление контекста:
   
- Загрузка регистров из стека ядра (по %rsp)
- После -- инструкция sysretq (iretq) . CPU использует загруженные значения регистров для возврата из kernel mode в user mode.


4. PCB в Linux (struct task_struct):
```c
struct task_struct {
    // Process identification
    pid_t pid;                    // Process ID
    pid_t tgid;                   // Thread Group ID
    pid_t ppid;                   // Parent PID
    
    // Process state
    volatile long state;          // Process state (running, sleeping, etc.)
    int exit_state;               // Exit state
    int exit_code;                // Exit code
    
    // Scheduling information
    int prio;                     // Static priority
    int static_prio;              // Dynamic priority
    struct sched_entity se;       // Scheduling entity
    
    // Memory management
    struct mm_struct *mm;         // Memory descriptor
    struct vm_area_struct *mmap;  // Memory areas
    
    // Files and I/O
    struct files_struct *files;   // Open files
    
    // Signal handling
    struct signal_struct *signal; // Signal information
    sigset_t blocked;             // Blocked signals
    
    // Credentials and capabilities
    const struct cred *cred;      // Process credentials
    
    // Relationships
    struct task_struct *real_parent; // Real parent process
    struct task_struct *parent;      // Parent process
    struct list_head children;       // List of children
    struct list_head sibling;        // Sibling processes
    
    // Thread information
    struct thread_struct thread;     // Architecture-specific thread info
    
    // Timers and time accounting
    cputime_t utime, stime;          // User and system time
    
    // ... many more fields (over 100 in modern kernels)
};
```


