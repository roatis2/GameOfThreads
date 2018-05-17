#include "PIT.h"
#include "IDT.h"
#include "types.h"
#include "i8259.h"
#include "lib.h"
#include "keyboard.h"
#include "x86_desc.h"
#include "paging.h"
#include "syscall.h"

#define DIV_VAL			1193180//oscillator
#define MODE_2		    0x34  //rate generator
#define MODE_3  		0x36  //Square wave generator mode
#define MASK_FRQ		0xFF  //get lower 8 bits
#define BIT_8SHIFT		8	  //shift by 8 bits
#define CHAN0			0x40  //Channel 0 data port (read/write)
#define CHAN1			0x41  //Channel 1 data port (read/write)
#define CHAN2			0x42  //Channel 2 data port (read/write)
#define CMD_REG		    0x43  //Mode/Command register (write only, a read is ignored)



/* 
 * init_PIT
 *   DESCRIPTION: initalize PIT
 *   INPUTS: term -- frequency for PIT interrupt
 *   OUTPUTS: NONE
 *   RETURN VALUE: NONE
 *   SIDE EFFECTS: enable PIT IRQ   
 */
void init_PIT(uint32_t frequency)
{
	//Set the interrupt rate using divisor and frequency input
	uint32_t divisor = DIV_VAL / frequency;
	// Send the command byte to set to mode 3
	outb(MODE_3, CMD_REG);
	// Split upper and lower bytes of divisor to send to regs
	uint8_t lower = (divisor & MASK_FRQ);
	uint8_t higher = (divisor >> BIT_8SHIFT) & MASK_FRQ ;
	// Send the frequency divisor in 2 bytes
	outb(lower, CHAN0);
	outb(higher,CHAN0);
	//enable IRQ for PIT
	enable_irq(PIT_IRQ);
}


/* 
 * PIT_handler
 *   DESCRIPTION: Handles process switching on PIT interrupt
 *   INPUTS: NONE
 *   OUTPUTS: NONE
 *   RETURN VALUE: NONE
 *   SIDE EFFECTS: remaps process page   
 */
void PIT_handler(void)
{

	
	cli();	//prevent interrupts since changing so much memory stuff
	
	
	uint8_t next_term = get_scheduled_term();

	//check if what switching to is different and requires context switch
	if(next_term == current_term)
	{	
		send_eoi(PIT_IRQ);
		sti();
		return;
	}

	//get current pcb for this terminal

	uint8_t cur_pid = terminal[current_term].process;
	uint8_t next_PID = terminal[next_term].process;
	PCB_t* cur_pcb = (PCB_t*)get_pcb_addr_C(cur_pid);
	PCB_t* next_pcb = (PCB_t*)get_pcb_addr_C(next_PID);
	

	//switch paging to next terminal
	set_page_execute(next_PID);

	//update cursor
	//move_cursor(next_term);

	tss.ss0 = KERNEL_DS;
	tss.esp0 = get_esp0(next_PID);
	///////////Save Current ////////////////////
	asm volatile ( 
		"movl	%%esp, %0;" 
		"movl	%%ebp, %1;"        							
		: "=r"(cur_pcb->this_esp), "=r"(cur_pcb->this_ebp)
		: 
		: "memory"
	);

	//update the current term we are running
	current_term = next_term;
	
	//////////// Switch to next scheduled //////////////////// more stuff?
	asm volatile(
		"movl %0, %%esp;"
		"movl %1, %%ebp;" 
		:
		: "r"(next_pcb->this_esp), "r"(next_pcb->this_ebp)
		: "esp","ebp"
	);
	send_eoi(PIT_IRQ);
	sti();
	return;
}


/* 
 * get_scheduled_term
 *   DESCRIPTION: searches through the open terminals to find the next process
 *   INPUTS: NONE
 *   OUTPUTS: NONE
 *   RETURN VALUE: term -- terminal which has a process we are switching to next
 *   SIDE EFFECTS: NONE
 */
uint8_t get_scheduled_term(void)
{
	uint8_t term = (current_term+1)%NUM_TERM;

	while(term != current_term)
	{
		if(total_progs[term]>0)
		{
			return term;
		}
		term = (term + 1)%NUM_TERM;
	}
	return term;
}

