#include "i8259.h"
#include "tests.h"
#include "lib.h"
#include "rtc.h"
#include "keyboard.h"

#define Status_Register_A  0x8A
#define Status_Register_B  0x8B
#define Status_Register_C  0x0C
#define RW_P               0x71
#define RTC_PORT           0x70
#define SLAVE_IRQ          2
#define RTC_RAM            0x2F
#define SIXTH_ON           0x40


volatile uint8_t rtc_flag[NUM_TERM]; //rtc interrupt occurred
uint32_t rate = 0;	  //init rate

//We used OS dev and the data sheet provided to implement enable_rtc
/*  enable_rtc
*	  INPUTS: NONE
*     Description: Enables RTC line and also sets frequency of scheduled interrupts
*     OUTPUTS: NONE
*     SIDE EFFECTS: Spews garbage values onto screen, periodically but useless information
*/
void enable_rtc()
{
	uint8_t term;
	/* enable rtc irq lines on master for slave*/
	enable_irq(SLAVE_IRQ);
	
	/* select Status Register A, and disable NMI (by setting the 0x70 bit) */
	outb(Status_Register_A,  RTC_PORT);
	uint8_t prev=inb(RW_P);
	/* write to CMOS/RTC RAM */
	outb(RTC_RAM, RW_P );

	/* select register B, and disable NMI */
	outb(Status_Register_B, RTC_PORT);		

	/* read the current value of register B */
	prev=inb(RW_P);

	/* write the previous value ORed with 0x40. This turns on bit 6 of register B */
	outb( prev | SIXTH_ON ,RW_P);

	/* set the index again (a read will reset the index to register D) */
	outb(Status_Register_B, RTC_PORT);

	/* This will turn on the IRQ with the default 1024 Hz rate. */
	enable_irq(RTC_IRQ);

	//set starting frequency, null fd,2 for frequency, and 4bytes
	open_rtc(NULL);
	
	//multiple terminals so one flag each-hardcoded to 3
	for(term = 0; term < NUM_TERM; term ++)
	{
		rtc_flag[term] = 0;
	}

	
}


/*rtc_interrupt
*	  INPUTS: NONE
*     Description: Scheduled interrupt for rtc. 
*     OUTPUTS: NONE
*     SIDE EFFECTS: Spews garbage values onto screen
*/
void rtc_interrupt()
{
	cli();
	uint8_t term;
	outb(Status_Register_C, RTC_PORT);	// select register C
	inb(RW_P);			// just throw away contents

	//mark that an rtc interrupt occured
	for(term = 0; term < NUM_TERM; term ++)
	{
		rtc_flag[term] = 1;
	}

	send_eoi(RTC_IRQ);
	sti();
}
	

/*read_rtc
* 	INPUTS: fd-- file descriptor 
			buf -- buffer to read
			nbytes -- number of bytes to read   
* 	Description: blocks until the next rtc interupt*     
* 	OUTPUTS: NONE*     
* 	SIDE EFFECTS: blocks until rtc intrupt so if it never comes it will freeze everything
*/
int32_t read_rtc(int32_t fd, void* buf, int32_t nbytes)
{		
	while(rtc_flag[current_term] == 0){/*Do nothing*/}	//block until an interupt	
	rtc_flag[current_term] = 0;
	return  0;	
}


/*write_rtc
* 	INPUTS: fd-- file descriptor 
			buf -- frequency to set
			nbytes -- byte to write(must be 4)
* 	Description: change rtc frquency     
* 	OUTPUTS: NONE     
* 	SIDE EFFECTS: 
*/
int32_t write_rtc(int32_t fd, const void* buf, int32_t nbytes)
{	

	if(nbytes != 4 || buf == NULL){return -1;} //must have 4 bytes
	cli();
	int32_t frequency = ((int32_t*)buf)[0];

	if((frequency < 2) || (frequency > 1024))// make sure its in the correct range		
		return -1;	

	int32_t temp = frequency;	
	while(temp != 1)			//make sure its a power of 2
	{	
		if(temp %2 != 0)			
			return -1;	//-1 is error	
		temp = temp/2;	
	}

	rate = 1;	

	while (frequency != 32768) //32768 is 2^15 and used to calculate rate
	{
		frequency = frequency << 1;		
		rate++;	
	}

	
	outb(Status_Register_A,RTC_PORT);		//we must access reg A to change rate	
	uint8_t A_DATA = inb(RW_P);			//copy data from the port
	A_DATA = A_DATA & 0xF0;					//access top 4 bits
	outb(Status_Register_A,RTC_PORT);		//reaccess reg A so we can change the rate now
	outb((rate | A_DATA), RW_P );
	sti();	
	return 0;
}


/*open_rtc
* 	INPUTS: filename -- name of rtc filename
* 	Description: change rtc frquency     
* 	OUTPUTS: NONE     
* 	SIDE EFFECTS: NONE
*/
int32_t open_rtc(const uint8_t* filename)
{	
	uint32_t rate[1] = {2};
	write_rtc(0,rate,4);	//open_rtc should initialize the rtc to interupt at a rate of 2Hrz	
	return 0;
}


/*close_rtc
* 	INPUTS: fd-- file descriptor
* 	Description: change rtc frquency     
* 	OUTPUTS: NONE
* 	SIDE EFFECTS: NONE 
*/
int32_t close_rtc(int32_t fd)
{	
	return 0;
}




