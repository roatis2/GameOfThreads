#include "IDT.h"
#include "x86_desc.h"
#include "lib.h"
#include "keyboard.h"
#include "Wrappers.h"
#include "syscall.h"
#include "types.h"

/* Macro that will make an exception function */
#define EXCEPTION(name,msg)	\
void name(){				\
 	cli();					\
	printf("%s", #msg);	\
	while(1);				\
};

/* create functions using above defined macro for the exceptions*/
EXCEPTION(DivZero,"Divide Error Exception");
EXCEPTION(Debug,"Debug Exception");
EXCEPTION(NMI,"NMI Interrupt");
EXCEPTION(Brkpt,"Breakpoint Exception");
EXCEPTION(overflow,"Overflow Exception");
EXCEPTION(boundrange,"BOUND Range Exceeded Exception");
EXCEPTION(invalOP,"Invalid Opcode Exception");
EXCEPTION(DNAexception,"Device Not Available Exception");
EXCEPTION(DF,"Double Fault Exception");
EXCEPTION(CoP_seg,"Coprocessor Segment Overrun");
EXCEPTION(InvalTSS,"Invalid TSS Exception");
EXCEPTION(SegNP,"Segment Not Present");
EXCEPTION(StackF,"Stack Fault Exception");
EXCEPTION(FloatError,"Floating Point Exception");
EXCEPTION(Alignment,"Alignment Check Exception");
EXCEPTION(MachineChk,"Machine Check Exception");
EXCEPTION(SimDFP,"SIMD Floating-Point Exception");
EXCEPTION(general, "general interrupt");


/*
 * init_IDT
 *   DESCRIPTION: Initialize IDT table
 *   INPUTS: NONE
 *   OUTPUTS: NONE
 *   RETURN VALUE: NONE
 *   SIDE EFFECTS: initializes the IDT Table and links the handlers with their 
 *				   respective interrupt entries
 */
void init_IDT()
{
	int index;				//use to assign IDT table entries
	
	lidt(idt_desc_ptr);		//initialize IDTR     
	
	for(index = 0; index < 32; index++) //First 32 used for exceptions
	{
		if(index == 15){ continue;}
		
		idt[index].reserved4 = 0x0;
        idt[index].reserved3 = 0x1; //Type -> 32-bit interrupt gate
        idt[index].reserved2 = 0x1;
        idt[index].reserved1 = 0x1;
        idt[index].reserved0 = 0x0;
		
		//Set privilege level to kernel
		idt[index].dpl = KERNEL_PRIV;
		//mark handler as present
		idt[index].present = 0x1;
		//Size stays as 1
		idt[index].size = 0x1;
		//Currently in Kernel Segment
		idt[index].seg_selector = KERNEL_CS;
	}

	for(index = 32; index < NUM_VEC; index++) //remaining entries
	{
		idt[index].reserved4 = 0x0;
        idt[index].reserved3 = 0x0; //Type 16-bit interrupt gate
        idt[index].reserved2 = 0x1;
        idt[index].reserved1 = 0x1;
        idt[index].reserved0 = 0x0;
		
		//Set privilege level to kernel
		idt[index].dpl = KERNEL_PRIV;
		//mark handler as present
		idt[index].present = 0x1;
		//Size stays as 1
		idt[index].size = 0x1;
		//Currently in Kernel Segment
		idt[index].seg_selector = KERNEL_CS;
	
		if(index == SYSCALL_INT)
		{	
			// interrupt vector 
			idt[index].reserved3 = 0x1; //////////??????
			//Set privilege level to user
			idt[index].dpl = USER_PRIV;
		}
	}
	
	//Still need to setup for remaining entries and the PIC, Keyboard and RTC	
		
	// Route each exception to its corresponding handler
	// generic print handlers at the moment for entries 0-19
		
	SET_IDT_ENTRY(idt[0], DivZero);
	SET_IDT_ENTRY(idt[1], Debug);
	SET_IDT_ENTRY(idt[2], NMI);
	SET_IDT_ENTRY(idt[3], Brkpt);
	SET_IDT_ENTRY(idt[4], overflow);
	SET_IDT_ENTRY(idt[5], boundrange);
	SET_IDT_ENTRY(idt[6], invalOP);
	SET_IDT_ENTRY(idt[7], DNAexception);
	SET_IDT_ENTRY(idt[8], DF);
	SET_IDT_ENTRY(idt[9], CoP_seg);
	SET_IDT_ENTRY(idt[10], InvalTSS);
	SET_IDT_ENTRY(idt[11], SegNP);
	SET_IDT_ENTRY(idt[12], StackF);
	SET_IDT_ENTRY(idt[13], GPE);
	SET_IDT_ENTRY(idt[14], PF);
	//SET_IDT_ENTRY(idt[15], reserved); // interrupt reserved by INTEL
	SET_IDT_ENTRY(idt[16], FloatError);
	SET_IDT_ENTRY(idt[17], Alignment);
	SET_IDT_ENTRY(idt[18], MachineChk);
	SET_IDT_ENTRY(idt[19], SimDFP);
	
	// Route to RTC interrupt handler
	SET_IDT_ENTRY(idt[RTC_INT], RTC_ASM_wrapper);
	
	// Route to Keyboard interrupt handler //calls test func for now
	SET_IDT_ENTRY(idt[KEYBOARD_INT], keyboard_ASM_wrapper);

	// Route to System Call interrupt handler
	SET_IDT_ENTRY(idt[SYSCALL_INT], sys_call_handler);

	// Route to PIT interrupt handler
	SET_IDT_ENTRY(idt[PIT_INT], PIT_ASM_wrapper);
	
}


/*
 * PF
 *   DESCRIPTION: page fault exception subroutine
 *   INPUTS: NONE
 *   OUTPUTS: NONE
 *   RETURN VALUE: NONE
 *   SIDE EFFECTS: prints address that caused the page fault
 *				   halts the process which caused the fault
 */
void PF(void)
{
	uint32_t prob_addr;

	asm volatile (
		"movl %%cr2, %0;"
		:"=r" (prob_addr)
	);
	printf("\nPage Fault Exception***\n");
	printf("\nAddr: 0x%x\n",prob_addr);
	squash_exception();
}


/*
 * GPE
 *   DESCRIPTION: general protection error exception subroutine
 *   INPUTS: NONE
 *   OUTPUTS: NONE
 *   RETURN VALUE: NONE
 *   SIDE EFFECTS: prints address that caused the error
 *				   halts the process which caused the eror
 */
void GPE(void)
{
	uint32_t source;

	asm volatile(
		"mov %%cr2, %0;"
		: "=r"(source)
	);

	printf("General protection exception! cr2: 0x%x\n", source);

	squash_exception();
}


/*
 * squash_exception
 *   DESCRIPTION: halts the process which caused the exception if terms initialized else "spins"
 *   INPUTS: NONE
 *   OUTPUTS: NONE
 *   RETURN VALUE: NONE
 *   SIDE EFFECTS: 
 */
void squash_exception(void)
{
	//exception occurred due to a terminal process, so halt it 
	if (terms_on)
	{
		PCB_t* cur = (PCB_t*)get_pcb_addr_C(terminal[current_term].process);
		cur->halt_stat = 1;

		halt(0); //halt on exception if terminal initialized
	}
	//exception occured due to something in kernel so dont squash just spin
	/*modeled after the end of kernel.c*/
	asm volatile(".1: hlt; jmp .1;");
}


