
int do_syscall ();
void handle_exception ();

#define PRINT_INT_SYSCALL	1
#define PRINT_FLOAT_SYSCALL	2
#define PRINT_DOUBLE_SYSCALL	3
#define PRINT_STRING_SYSCALL	4

#define READ_INT_SYSCALL	5
#define READ_FLOAT_SYSCALL	6
#define READ_DOUBLE_SYSCALL	7
#define READ_STRING_SYSCALL	8

#define SBRK_SYSCALL		9

#define EXIT_SYSCALL		10

#define PRINT_CHARACTER_SYSCALL	11
#define READ_CHARACTER_SYSCALL	12

#define OPEN_SYSCALL		13
#define READ_SYSCALL		14
#define WRITE_SYSCALL		15
#define CLOSE_SYSCALL		16

#define EXIT2_SYSCALL		17


// IMPLEMANTATION

#include <string.h>

#define DEFAULT_RUN_STEPS 2147483647

#define CREATE_PROCESS 20




void CreateProcess();
extern bool read_assembly_file (char *name);
extern bool run_program (mem_addr pc, int steps, bool display, bool cont_bkpt, bool* continuable);
extern mem_addr current_data_pc();
extern void set_text_pc (mem_addr addr);
