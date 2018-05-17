/* i8259.c - Functions to interact with the 8259 interrupt controller
 */
#include "i8259.h"
#include "lib.h"

#define FULL_MASK    0xFF
#define MASTER_IRQS  8
#define SLAVE_IRQS   16

/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask; /* IRQs 0-7  */
uint8_t slave_mask;  /* IRQs 8-15 */


/*i8259_init
*     INPUTS: NONE
*     DESCRIPTION: Initiates the PIC, mask interrupts while we set up master and slave
*     OUTPUTS: NONE
*     SIDE EFFECTS: Allows us to map interrupts to CPU in order of priority
*/
void i8259_init(void) 
{
	master_mask = FULL_MASK;
	slave_mask = FULL_MASK;
	/* mask all interrupts while we set up pics */
	outb(FULL_MASK, MASTER_8259_PORT_D);
	outb(FULL_MASK, SLAVE_8259_PORT_D);

	/* set up Master pic */
	outb(ICW1, MASTER_8259_PORT); //starts the initialization sequence (in cascade mode)
	outb(ICW2_MASTER, MASTER_8259_PORT_D); //ICW2: Master PIC vector offset (IDT location) 
	outb(ICW3_MASTER, MASTER_8259_PORT_D); //ICW3: Serial offset
	outb(ICW4, MASTER_8259_PORT_D); //ISA=x86, normal/auto EOI
	
	/* set up slave pic */
	outb(ICW1, SLAVE_8259_PORT); //starts the initialization sequence (in cascade mode)
	outb(ICW2_SLAVE, SLAVE_8259_PORT_D); //ICW2: Slave PIC vector offset (IDT location)
	outb(ICW3_SLAVE, SLAVE_8259_PORT_D); //IRQ for slave on master
	outb(ICW4, SLAVE_8259_PORT_D); //ISA=x86, normal/auto EOI
	
}


/*enable_irq
*	  INPUTS: irq_num -- IRQ to unmask
*     DESCRIPTION: unmasks the specific IRQ line so interrupt can be processed
*     OUTPUTS: NONE
*     SIDE EFFECTS: NONE
*/
void enable_irq(uint32_t irq_num) 
{
	 if(irq_num < MASTER_IRQS)
	{
		master_mask = master_mask & ~(0x1 << irq_num);
		outb(master_mask, MASTER_8259_PORT_D); // write to data 
	}
	/* if irq 8-15 we want slave */
	 else if(irq_num <SLAVE_IRQS)
	{
		uint8_t slave_irq_num = irq_num - MASTER_IRQS;
		slave_mask = slave_mask & ~(0x01 << slave_irq_num);
		outb(slave_mask, SLAVE_8259_PORT_D); // write to data 
	}  
}


/*disable_irq
*	  INPUTS: irq_num -- IRQ number to disable
*     DESCRIPTION: mask the speicific IRQ line so that interrupt can stop
*     OUTPUTS: NONE
*     SIDE EFFECTS: stops the interrupt after its termination
*/
void disable_irq(uint32_t irq_num) {
	/* if irq 0-7 we want master */
	 if(irq_num < MASTER_IRQS)
	{
		master_mask = master_mask | (0x01 << irq_num);
		outb(master_mask, MASTER_8259_PORT_D); // write to data 
	} 
	/* if irq 8-15 we want slave */
	else if(irq_num <SLAVE_IRQS)
	{
		uint8_t slave_irq_num = irq_num - MASTER_IRQS;
		slave_mask = slave_mask | (0x01 << slave_irq_num);
		outb(slave_mask, SLAVE_8259_PORT_D);  // write to data 
	} 
}


/*send_eoi
*	  INPUTS: irq_num -- IRQ sending EOI
*     DESCRIPTION: Sends a signal at end of interrupt to know that the interrupt has been processed
*     OUTPUTS: NONE
*     SIDE EFFECTS: lets processor know that they can process next interrupt since this one is completed
*/
void send_eoi(uint32_t irq_num) 
{
	/* 
		if IRQ came from master pic, we can issue this command to master pic
		if IRQ came from slave pic, we issue this command to both slave and master pic
	*/
	
	/* if irq 0-7 we want master */	
	if((irq_num > 7) && (irq_num <SLAVE_IRQS))
	{
		outb(EOI | (irq_num -8), SLAVE_8259_PORT);   // write to command port
		outb(SLAVE_EOI , MASTER_8259_PORT);	// write EOI for SLAVE IRQ on MASTER to command port
	}	
	else 
	{
		outb( (EOI | irq_num ), MASTER_8259_PORT);
	}
}
