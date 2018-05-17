#ifndef _IDT_H
#define _IDT_H 

//Values for index in IDT table 
//Indexes taken from lecture slides for these  
#define KEYBOARD_INT	0x21	//IRQ1
#define RTC_INT			0x28	//Real time clock
#define PIT_INT			0x20	//PIT IRQ0
#define SYSCALL_INT		0x80	//System call entry
#define KERNEL_PRIV     0x0
#define USER_PRIV       0x3
//Initialize the IDT
void init_IDT(void);

void squash_exception(void);

void GPE(void);

void PF(void);

#endif
