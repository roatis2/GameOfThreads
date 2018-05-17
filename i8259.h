#ifndef _I8259_H
#define _I8259_H

#include "types.h"
/* i8259.h - Defines used in interactions with the 8259 interrupt
 * controller
 */

/* Ports that each PIC sits on */
#define MASTER_8259_PORT    0x20 //command
#define MASTER_8259_PORT_D  0x21 //data
#define SLAVE_8259_PORT     0xA0 //command
#define SLAVE_8259_PORT_D   0xA1 //data

/* Initialization control words to init each PIC.
 * See the Intel manuals for details on the meaning
 * of each word */
#define ICW1                0x11 //starts the initialization sequence (in cascade mode)
#define ICW2_MASTER         0x20 //ICW2: Master PIC vector offset (IDT location) 
#define ICW2_SLAVE          0x28 //ICW2: Slave PIC vector offset (IDT location)
#define ICW3_MASTER         0x04 //ICW3: Serial offset
#define ICW3_SLAVE          0x02 //IRQ for slave on master
#define ICW4                0x01 //ISA=x86, normal/auto EOI

/* End-of-interrupt byte.  This gets OR'd with
 * the interrupt number and sent out to the PIC
 * to declare the interrupt finished */
#define EOI                 0x60
#define SLAVE_EOI           EOI+0x02

#define KEYBOARD_IRQ_NUM     1	//irq for keyboard
#define RTC_IRQ              8	//irq for rtc
#define PIT_IRQ              0	//irq for pit

/* Externally-visible functions */

/* Initialize both PICs */
void i8259_init(void);

/* Enable (unmask) the specified IRQ */
void enable_irq(uint32_t irq_num);

/* Disable (mask) the specified IRQ */
void disable_irq(uint32_t irq_num);

/* Send end-of-interrupt signal for the specified IRQ */
void send_eoi(uint32_t irq_num);

#endif /* _I8259_H */
