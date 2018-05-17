#ifndef _SYSCALL_H 
#define _SYSCALL_H 

#include "types.h"
#include "rtc.h"
#include "filesystem.h"
#include "keyboard.h"


//Flag values for type of files
#define 	MAX_NUM_PROC		   6    
#define		RTC_FILE			   0
#define		DIRECTORY_FILE		   1
#define		TXT_FILE	           2
#define     MAX_FILE               8
#define     MAX_PROGRAM            6	//max number of programs allowed to open???????
#define     MASK_PCB     		   0xFFFFE000	//similar to paging mask the last 13 bits and use rest to index into kernel stack for 8KB PCB 'pages'
#define 	TOP_OF_KERNEL		   0x800000   //top of kernel space in memory
#define     KB8					   0x2000	  //size of PCB 8KB
#define 	KB4					   0x1000	  //4kB
#define     MB8					   0x800000   
#define     MB4					   0x400000
#define     MB2 				   0x200000
#define 	MB128				   0x8000000
#define 	EID0				   0x7F
#define 	EID1				   0x45
#define 	EID2				   0x4C
#define 	EID3				   0x46
#define     NOT_PRES               0 	//FLAG FOR FD TO MARK NOT PRESENT
#define     PRESENT                1    //Mark fd as present/in use
#define     STDOUT_IDX			   1    //Index for stdout in fd array
#define     STDIN_IDX			   0    //Index for stdin in fd array
#define     FILE_START             2    //First index in fd array not stdin or stdout
#define 	USER_PROG			   0x08048000

#define     ARG_BUF_LIM            128


typedef struct file_op{
	int32_t (*open)(const uint8_t* filename);
	int32_t (*close)(int32_t fd);	
	int32_t (*read)(int32_t fd, void* buf, int32_t nbytes);
	int32_t (*write)(int32_t fd, const void* buf, int32_t nbytes);
} file_ops_t;

// Struct for each dynamically assigned file in file descriptor 
typedef struct file_des {
    file_ops_t op_table;
    int32_t  inode;
    int32_t  file_position;
    int32_t  flags;		//use to say if present can use other 31 bits for other flags
}file_des_t;

typedef struct PCB{
	file_des_t file_des_array[MAX_FILE];//Array for open files 8 max but only have 6 non reserved
	uint8_t PID_num;					//process ID number
	uint32_t this_esp;					//esp for current process, use on term switch
	uint32_t this_ebp;					//ebp for current process, use on term switch
	uint32_t halt_stat;					//flag for exception halt call
	uint8_t parent_PID_num;				//parent process ID number
	uint32_t par_esp;					//parent kernal stack ESP???? for restore on halt
	uint32_t par_ebp;					//parent kernal stack EBP???? for restore on halt
	uint8_t args[ARG_BUF_LIM];			//used in get args, hold arguments minus leading spaces and includes all trailing+inner spaces
} PCB_t;
	

//array that tells us what PCB's are available
uint32_t PCB_in_use [MAX_NUM_PROC];
//program count for each terminal
uint8_t total_progs[NUM_TERM];
//total process count, limited to max number of pograms
uint8_t process_cnt;

/* End a system call */
int32_t halt (uint8_t status);
/* Execute a system call*/
int32_t execute (const uint8_t* command);
/* Read system call */
int32_t read (int32_t fd, void* buf, int32_t nbytes);
/* Write system call*/
int32_t write (int32_t fd, const void* buf, int32_t nbytes);
/* Open system call*/
int32_t open (const uint8_t* filename);
/* Close system call*/
int32_t close (int32_t fd);
/* Get arguments system*/
int32_t getargs (uint8_t* buf, int32_t nbytes);
/* Vidmap system call*/
int32_t vidmap(uint8_t** screen_start);
/* Set-handler system Call*/
int32_t set_handler (int32_t signum, void* handler_address);
/* Sigreturn system Call*/
int32_t sigreturn (void);
//get current pcb
PCB_t* get_pcb_addr(void);
//return available pid if it exists
int32_t pid_available(void);
//get addres of pcb given pid
uint32_t get_pcb_addr_C(uint8_t pid);
//test shell execute
void exec_test_shell(void);
//parse args from command buffer
int8_t parse_args(const uint8_t* command, uint8_t* filename, uint8_t* args);
//calculate the esp0 for the pcb with the pid
uint32_t get_esp0(uint8_t pid);



#endif



