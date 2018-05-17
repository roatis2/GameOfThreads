#include "syscall.h"
#include "types.h"
#include "x86_desc.h"
#include "lib.h"
#include "rtc.h"
#include "keyboard.h"
#include "filesystem.h"
#include "paging.h"
#include "IDT.h"
#include "i8259.h"
#include "keyboard.h"

#define HEADER_SIZE			40
#define PROG_IMAGE_LOCATION	0x08048000
#define COM_BUF_SIZE        128			//max size of command buffer and most string buffers


file_ops_t rtc_ops = { open_rtc, close_rtc, read_rtc, write_rtc };
file_ops_t file_ops = { file_open, file_close, file_read, file_write };
file_ops_t directory_ops = { directory_open, directory_close, directory_read, directory_write };
file_ops_t std_ops = { terminal_open, terminal_close, terminal_read, terminal_write};



/* 
 * halt
 *   DESCRIPTION: halts current process in terminal
 *   INPUTS: status -- input value for halt (0-255)
 *   OUTPUTS: 0
 *   RETURN VALUE: 0 / manipulates eax to return status value or 256 on exception
 *   SIDE EFFECTS: change program mapping or re-execute shell in terminal               
 */
int32_t halt (uint8_t status)
{
	// status becomes return value of execute. find a way to give status into execute. You can use a register or global holding status  
	cli();
	uint32_t retval = (uint32_t)status;
	//get pcb of current program
	PCB_t* current = (PCB_t*)get_pcb_addr_C(terminal[current_term].process);
	
	//close all files in pcb -- looping 8 times using syscall. check open flag
	int32_t fd;

	// In this case, you don't need to care about the return from close
	for(fd = FILE_START; fd < MAX_FILE ; fd++ )
	{
		close(fd);
	}
	//decrement number of programs running, free PID
	total_progs[current_term]--;
	process_cnt--;
	PCB_in_use[current->PID_num] = 0;
	
	//if closing last program re-execute shell
	if(total_progs[current_term] == 0)
	{
		execute((uint8_t*)"shell");
	}
	//set head process in terminal to parent of current process being halted
	terminal[current_term].process = current->parent_PID_num;
	//Update Task State Segment SS0 and ESP0 -- values inside of tss. user stack and kernal stack. change esp0 points to kernel stack on correct process.
	tss.ss0= KERNEL_DS;   					 
	// go back to top of parent stack
	tss.esp0 = get_esp0(current->parent_PID_num);

	set_page_execute(current->parent_PID_num);

	if(current->halt_stat == 1)
	{
		current->halt_stat = 0;
		retval = 256;
	}
	// parent_halt has proper esp and ebp, and clear tlb
	asm volatile(
		"movl %0, %%esp;"
        "movl %1, %%ebp;"
        "movl %2, %%eax;"
		"jmp JUMPFINISH;"
    	:
		:"r"(current->par_esp), "r"(current->par_ebp), "r"(retval)
		);

	return -1;
}

/* 
 * execute
 *   DESCRIPTION: system call for execute
 *   INPUTS: command -- holds command for a function
 *   OUTPUTS: 0 or -1
 *   RETURN VALUE: 0 -- success
 				  -1 -- abnormal failure
 				  256 -- failed on exception
 *   SIDE EFFECTS: executes a program               
 */
int32_t execute (const uint8_t* command)
{
	
	//buffers to hold filename and remaining args of command
	uint8_t filename[COM_BUF_SIZE], args[COM_BUF_SIZE];
	uint8_t i;
	//check if there is a pid we can give it
	uint32_t pid = pid_available();
	
	if(pid == -1)
	{
		printf("\n Error: Max processes reached\n");
		return -1;
	}

	if(command ==NULL){return -1;}

	//check if command is valid and get filename and args
	if(parse_args(command,filename,args) == -1)
	{
		printf("Bad command parse\n");
		PCB_in_use[pid] = 0;	//free PID on failure
		return -1;
	}

	//check if we can find file by name
	dentry_t fdentry;
	if(read_dentry_by_name(filename, &fdentry) == -1)
	{
		printf("\nError: File not found \n");
		PCB_in_use[pid] = 0;	//free PID on failure
		return -1;
	}

	if(fdentry.filetype != TXT_FILE)
	{
		printf("\nError: Filetype\n");
		PCB_in_use[pid] = 0;	//free PID on failure
		return -1;
	}
	//use header to confirm file is an executable
	uint8_t header [HEADER_SIZE];
	// populate header with first 40 bytes of shell file
	if(read_data(fdentry.inode_num, 0, header, HEADER_SIZE) == -1)
	{
		printf("Read data error\n");
		PCB_in_use[pid] = 0;	//free PID on failure
		return -1;
	}

	// Read the first four bytes of the ELF file
	if((header[0] != EID0) || (header[1] != EID1) || (header[2] != EID2) || (header[3] != EID3))
	{
		printf("\nError: File is not an executable \n");
		PCB_in_use[pid] = 0;	//free PID on failure
		return -1;
	}

	//virtual address of first instruction
	uint32_t EIP = 0;

	// Shift by one bytes, then two  bytes, then 3 bytes to set the EIP (entry point of program)
	EIP |= (header[27] << 24) | (header[26] << 16) | (header[25] << 8) | header[24];
	//printf("\n EIP = %x \n", EIP);
	/////////////////////   File is ready to execute, make it happen ////////////////////////////////////////////
	cli();

	// SET UP PAGING //
	set_page_execute(pid);

	//load program
	if(read_data(fdentry.inode_num, 0, (uint8_t*)PROG_IMAGE_LOCATION, MB4) == -1)
	{
		PCB_in_use[pid] = 0;	//free PID on failure
		sti();
		return -1;
	}

	PCB_t * pcb = (PCB_t*)get_pcb_addr_C(pid);//pcb for new process

	//set pid num 0 or 1
	pcb->PID_num = pid;
	
	//if only process on terminal, it is its own parent
	if(total_progs[current_term] == 0)
	{
		pcb->parent_PID_num = pid;	
	}
	else
	{	//set parent to be process which called it
		pcb->parent_PID_num = terminal[current_term].process;
	}
	//update current running PID in terminal
	terminal[current_term].process = pid;

	//make pcb at an open pid
	//set up first two files which are reserved as stdin and stdout
	// Mark file descriptor as present
	pcb->file_des_array[0].op_table = std_ops;
	pcb->file_des_array[0].flags = 1;
	pcb->file_des_array[0].file_position = 0;
	pcb->file_des_array[0].inode = -1;
	pcb->file_des_array[1].op_table = std_ops;
	pcb->file_des_array[1].flags = 1;
	pcb->file_des_array[1].file_position = 0;
	pcb->file_des_array[1].inode = -1;

	for (i = FILE_START; i < MAX_FILE; ++i)
	{
		pcb->file_des_array[i].flags = 0;
	}

	//move args to pcb 
	if(strcpy((int8_t*)pcb->args, (int8_t*)args) == NULL)
	{
		printf("Problem with args\n");
		PCB_in_use[pid] = 0;
		sti();
		return -1;
	}
	
	//store current esp, par_pdbr, and ebp
	asm volatile ( 
		"movl	%%esp, %0;"
		"movl	%%ebp, %1;"						
		: "=r"(pcb->par_esp), "=r"(pcb->par_ebp)
		: 
		: "memory"
	);
	

	tss.ss0  = KERNEL_DS;
	tss.esp0 = get_esp0(pid);////////////////////////////modify to include terminal stuff

	total_progs[current_term]++;
	process_cnt++;

	//cited sources
	//http://www.jamesmolloy.co.uk/tutorial_html/10.-User%20Mode.html
	//http://www.ibiblio.org/gferg/ldp/GCC-Inline-Assembly-HOWTO.html

	uint32_t USER_ESP = MEM_132 -4;	//this is user's memory location it ia 128 - 4 + 4MB because  its bytes 24-27 so 31 - 4 =27 and theres 4 bytes

	uint32_t retval;
	asm volatile ( 
		"movw	%0, %%ax;"         
		"movw	%%ax, %%ds;" 
		//"movw	%%ax, %%es;" 		
		//"movw	%%ax, %%fs;" 		
		//"movw	%%ax, %%gs;"			
		"movl	%%esp, %%eax;"		
		"pushl  %0;"                   
		"pushl  %1;"           			
		"pushfl ;" 					
		"popl   %%eax;"                
		"orl    $0x200,%%eax;" //enable interrupts
		"pushl  %%eax;"                
		"pushl  %2;"                
		"pushl  %3;"                    
		"iret;   "
		"JUMPFINISH:;"										
		:
		: "i"(USER_DS),"r"(USER_ESP),"i"(USER_CS),"r"(EIP)
		: "eax"
	);

	asm volatile (
		"movl %%eax, %0;"
		:"=r"(retval)
	);

	sti();	
    
	return retval;
}

/* 
 * read
 *   DESCRIPTION: system call for read
 *   INPUTS: fd -- file directory
 *	 		 buf -- buffer for variables to pass in the parameters
 *  		 nbytes -- size
 *   OUTPUTS: -1 or rtc_read,file_read,dir_read
 *   RETURN VALUE: -1 or rtc_read,file_read,dir_read
 *   SIDE EFFECTS: read function
 *                 
 */
int32_t read (int32_t fd, void* buf, int32_t nbytes)
{
	//check if fd valid 0-7 not stdout(1)
	if (fd < 0 || fd >= MAX_FILE || fd == STDOUT_IDX || buf == NULL){ return -1;}
	//get pcb address to check info
	PCB_t* current = (PCB_t*)get_pcb_addr_C(terminal[current_term].process);
    //Check if file descriptor present
	if(current->file_des_array[fd].flags == NOT_PRES){return -1;}
	//call corresponding read function using the table
	return current->file_des_array[fd].op_table.read(fd,buf,nbytes);
}

/* 
 * write
 *   DESCRIPTION: system call for write
 *   INPUTS: fd -- file directory
 *	 		 buf -- buffer for variables to pass in the parameters
 *  		 nbytes -- size
 *   OUTPUTS: -1 or rtc_write,file_write,dir_write
 *   RETURN VALUE: -1 or rtc_write,file_write,dir_write
 *   SIDE EFFECTS: write function
 *                 
 */
int32_t write (int32_t fd, const void* buf, int32_t nbytes)
{
	//check if fd valid 0-7 not stdout(1)
	if (fd < 0 || fd >= MAX_FILE || fd == STDIN_IDX || buf == NULL){ return -1;}
	//get pcb address to check info
	PCB_t* current = (PCB_t*)get_pcb_addr_C(terminal[current_term].process);
	//check if filedescriptor is present
	if(current->file_des_array[fd].flags == NOT_PRES){return -1;}
	//call corresponding read function using the table
	return current->file_des_array[fd].op_table.write(fd,buf,nbytes);
}

/* 
 * open
 *   DESCRIPTION: system call for read
 *   INPUTS: filename -- name of the file
 *   OUTPUTS: -1 or open_read, dir_open, file_open
 *   RETURN VALUE: -1 or open_read, dir_open, file_open
 *   SIDE EFFECTS: open function
 *                 
 */
int32_t open (const uint8_t* filename) 
{
	//check for valid name size
	if(filename == NULL){return -1;}

	uint32_t fd_index;

	//find file in directory
	dentry_t fdentry;
	PCB_t* current = (PCB_t*)get_pcb_addr_C(terminal[current_term].process);
	
	if (read_dentry_by_name(filename, &fdentry) != 0){ return -1;}

	//assign the file to an fd in the pcb by finding next open
	for(fd_index = FILE_START; fd_index < MAX_FILE; fd_index++)
	{
		if(current->file_des_array[fd_index].flags == NOT_PRES)
		{
			//set pcb entries based on file type
				// 0 is rtc so NULL inode, point to rts ops and use rtc oen 
				// 1 is a file so set file position to start, point to file ops, inode ptr to null????
				// 2 is executable so set inode ptr, fops table, and file position(0)
			//same as previous cases
			switch(fdentry.filetype)
			{
				//RTC file assign rtc table
				case RTC_FILE:	
					current->file_des_array[fd_index].op_table = rtc_ops;
					//current->file_des_array[fd_index].inode = -1; //not in inodes
					break;
				//DIR file assign directory ops
				case DIRECTORY_FILE:
					current->file_des_array[fd_index].op_table = directory_ops;
					//current->file_des_array[fd_index].inode = -1;//not in inodes
					break;
				//TXT file assign file ops
				case TXT_FILE:
					current->file_des_array[fd_index].op_table = file_ops;
					current->file_des_array[fd_index].inode = fdentry.inode_num;
					break;
				//unsupported file type
				default:
					return -1;
			}
			current->file_des_array[fd_index].file_position = 0; //set to file start
			current->file_des_array[fd_index].flags = PRESENT;
			break;
		}
	}
	//error if none available
	if(fd_index == MAX_FILE){return -1;}
	
	//call the open function, return -1 if error
	if(current->file_des_array[fd_index].op_table.open(filename) != 0){return -1;}
	//return fd index opened
	return  fd_index;

}

/* 
 * close
 *   DESCRIPTION: system call for read
 *   INPUTS: fd -- file descriptor value
 *   OUTPUTS: -1 or value returned by close
 *   RETURN VALUE: -1 or rtc_close,file_close,dir_close
 *   SIDE EFFECTS: close operation
 *                 
 */
int32_t close (int32_t fd)
{
	//check if valid fd num 2-7 cant do anything for stdin or stdout
	if(fd < FILE_START || fd >= MAX_FILE ){ return -1; }
	//get current pcb
	PCB_t* current = (PCB_t*)get_pcb_addr_C(terminal[current_term].process);
    //Check if file descriptor present
	if(current->file_des_array[fd].flags == NOT_PRES){return -1;}//try to close file not present
	
	else
	{
		current->file_des_array[fd].flags = NOT_PRES;  //make fd available for other files BY MARKING NOT PRESENT
		return current->file_des_array[fd].op_table.close(fd); //call corresponding close function and return code
	}

}

/* 
 * getargs
 *   DESCRIPTION: get arguments from pcb struct and copy into buf
 *   INPUTS: buf -- buffer to copy args into
 			 nbytes -- number of bytes to read in
 *   OUTPUTS: none
 *   RETURN VALUE: -1 on failure, 0 on success
 *   SIDE EFFECTS: 
 */
int32_t getargs (uint8_t* buf, int32_t nbytes)
{
	if(buf == NULL || nbytes < 0 ){return -1;}

	//get current pcb
	PCB_t* current = (PCB_t*)get_pcb_addr_C(terminal[current_term].process);
	//check if args present
	uint32_t length = strlen((int8_t*)current->args);
	if(length == 0 || nbytes < length){return -1;}
	//copy args into buf
	if (strcpy((int8_t*)buf,(int8_t*)current->args) == NULL){return -1;}

	return 0;
}

/* 
 * vidmap
 *   DESCRIPTION: map a virtual address to physical video memory
 *   INPUTS: screen_start -- virtual address of video memory
 *   OUTPUTS: 0
 *   RETURN VALUE: virtual base address for vid memory
 *   SIDE EFFECTS: video mapping operation
 *                 
 */
int32_t vidmap (uint8_t** screen_start)//////////////////////////////////////////////////////
{
	//check for valid screen pointer
	//if (screen_start == NULL){return -1;}
	//check that virtual address is in valid virtual space
	if ((uint32_t) screen_start < MEM_128 || (uint32_t) screen_start > MEM_132) {return -1;}
	//set pointer to video memory 
    *screen_start = terminal[current_term].term_video_mem;
 	//return virtual address of video memory
	return (int32_t)(terminal[current_term].term_video_mem);
}

/* 
 * set_handler
 *   DESCRIPTION: set's handler (optional)
 *         
 *   INPUTS: signum -- significant number
 *			 handler_address -- address of the handler
 *   OUTPUTS: -1
 *   RETURN VALUE: -1
 *   SIDE EFFECTS: optional set handler
 *                 
 */
int32_t set_handler (int32_t signum, void* handler_address)
{
	return -1;
}

/* 
 * sigreturn
 *   DESCRIPTION: returns sig
 *         
 *   INPUTS: NONE
 *   OUTPUTS: -1
 *   RETURN VALUE: -1 on error
 *   SIDE EFFECTS: optional sigreturn
 */
int32_t sigreturn (void)
{
	return -1;
}

/* 
 * pid_available
 *   DESCRIPTION: check if there is  a pid available
 *   INPUTS: NONE
 *   OUTPUTS: -1 or i
 *   RETURN VALUE: -1 or i
 *   SIDE EFFECTS: check if pid is availabl
 */
int32_t pid_available(void)
{
	int32_t i;
	for(i=0; i < MAX_NUM_PROC; i++)
	{                                                                                               
		if(PCB_in_use[i] == 0)
		{
			PCB_in_use[i] = 1;
			return i;
		}
	}
	return -1;
}

/* 
 * get_pcb_addr_C
 *   DESCRIPTION: get pcb address
 *   INPUTS: pid - process id
 *   OUTPUTS: addr
 *   RETURN VALUE: addr
 *   SIDE EFFECTS: get pcb address
 */
uint32_t get_pcb_addr_C(uint8_t pid)
{
	uint32_t addr = TOP_OF_KERNEL - KB8*(pid+1);
	return addr;
}

/* 
 * exec_test_shell 
 *   DESCRIPTION: execute a test shell
 *   INPUTS: pid - process id
 *   OUTPUTS: NONE
 *   RETURN VALUE: NONE
 *   SIDE EFFECTS: executes a test shell
 *                 
 */
void exec_test_shell(void)
{
	const uint8_t * shell = (uint8_t *) " shell yes";
	execute(shell);
	printf("\n made it here 3\n");
	return;
}


/* parse_args
*   DESCRIPTION: parse buffer into two space separated args
				 first arg is placed in an array while the rest
				 of the orig. buffer is placed in a third buffer  
*	INPUT: command -- buffer to parse in 2
*		   filename -- buffer for first argument
*		   args -- buffer for args
*	OUTPUT:NONE
*	RETURN: 0 on success, -1 on error
*	SIDE EFFECTS:
*/
int8_t parse_args(const uint8_t* command, uint8_t* filename, uint8_t* args)
{
	// the command parameter in system execute will have the string that describes the command.
	// Possibly in this checkpoint, they would enter one word at a time instead of multiple commands
	// seperated by spaces. for example: entering "ls"(one word) to open current directory along versus 
	// entering "ls mp1"(two words) to open mp1's directory. Trying to find a way to parse command by its SPACES
	
	int32_t start = 0;
	int32_t end = 0;
	
	//check valid inputs
	if(command == NULL || filename == NULL || args == NULL){return -1;}
	
	//check for leading spaces
	while(command[start] == SPACE)
	{
		start++;
		//check if buffer is full of spaces or premature newline or Null terminator
		if((command[end] == NEWLINE) || (command[end] == NOKE)){return -1;}
	}
	//find end of first word
	end = start; 
	//move to end of name argument/check if reaching end of command buffer , use a limit on command size????
	while((command[end] != SPACE) && (command[end] != NEWLINE) && (command[end] != NOKE) )
	{
		filename[end-start] = command[end];
		end++;
	}
	//add null terminator to filename
	filename[end-start] = NOKE;
	//check if reached the end of buffer or reached next arg
	start = end;
	while(command[start] == SPACE){start++;}
	//find end of args and use to index into arg buffer
	end = start;
	//copy remainder of buffer into args buffer with all spaces
	while(command[end] != NOKE && command[end] != NEWLINE )
	{
		args[end - start] = command[end];
		end++;
	}
	args[end-start] = NOKE;
	return 0;
}


/* 
 * get_esp0
 *   DESCRIPTION: calculate kernel stack pointer for the process passed in
 *   INPUTS: pid -- process ID we want a stack pointer for
 *   OUTPUTS: NONE
 *   RETURN VALUE: stack pointer address
 *   SIDE EFFECTS: NONE               
 */
uint32_t get_esp0(uint8_t pid)
{
	return TOP_OF_KERNEL - (KB8*pid) - 4;
}



