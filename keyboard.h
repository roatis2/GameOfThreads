/* keyboard.h - Defines keyboard driver interaction with interrupt
 * 
 */
#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include "types.h"

//Hardware constants
#define KEYBOARD_PORT	  0x60

//Special single press keys
#define BACKSPACE_PRESS   0x0E
#define TAB_PRESS         0x0F
#define ENTER_PRESS       0x1C
#define CAPS_PRESS		  0x3A
#define NUMLOCK_PRESS     0x45
#define F1_PRESS          0x3B  //F1 pressed, use to go to term 0
#define F2_PRESS          0x3C  //F2 pressed, use to go to term 1
#define F3_PRESS          0x3D  //F3 pressed, use to go to term 2
#define L_PRESS           0x26	//need to know for clear
#define UP_KEY			  0x48  //up key pressed
#define DOWN_KEY          0x50  //down key pressed
#define D_PRESS			  0x20  //need to know for EOI

//Special keys requiring holding
#define LSHIFT_PRESS      0x2A
#define LSHIFT_RELEASE    0xAA
#define RSHIFT_PRESS      0x36
#define RSHIFT_RELEASE    0xB6 
#define LALT_PRESS        0x38
#define LALT_RELEASE      0xB8
#define LCTRL_PRESS       0x1D
#define LCTRL_RELEASE     0x9D

//special keys and constants
#define NOKE    		  '\0'	//Undefined key
#define NEWLINE           '\n'  //New line character
#define SPACE              ' '  //Space character
#define PRESS                1  //certain key pressed (use to set flags)
#define UNPRESS              0  //certain key released (use to set flags)
#define KEY_BUFF_SIZE      128  //Buffer size as spcified by MP doc
#define KEY_MODES            4	//# of Key modes based on CAP and SHIFT
#define TAB_SIZE             3	//Number of spaces for tab key

//Terminal constants
#define NUM_TERM			 3  //specify the number of terminals possible
#define TERM0			     0  //terminal zero (avoid magic#)
#define TERM1			     1  //terminal one (avoid magic#)
#define TERM2			     2  //terminal two (avoid magic#)
#define ATTRIB0            0x7  //Set text color for terminal 
#define ATTRIB1            0x9  //Set text color for terminal
#define ATTRIB2            0xA  //Set text color for terminal
#define UP_ARROW            -1  //UP arrow key pressed on num pad or arrow keys(use for history)
#define DOWN_ARROW           1  //Down arrow key pressed on num pad or arrow keys(use for history)
#define MAX_HIST			 3  //Determine size of history bank

typedef struct { 
	//uint8_t term_id;					  //terminal number
	uint8_t screen_x;					  //cursor x-position , size same as screen_x 
	uint8_t screen_y;					  //cursor y-position , size same as screen_y
	uint8_t keyboard_buffer[KEY_BUFF_SIZE];//key buffer for each terminal 
	uint8_t keyboard_buf_idx;			  //current position in keyboard buffer	 
	uint8_t* term_video_mem; 			  //ptr to video memory for terminal 
	uint8_t process;      			      //process that terminal is running 
	volatile uint8_t Enter_flag;		  //Enter was pressed
	uint8_t read_wanted;				  //terminal process requesting read
	uint8_t color;						  //holds attribute color for terminal
	//history variables
	uint8_t history_buf[MAX_HIST][KEY_BUFF_SIZE];//holds history of N most recent inputs(N = MAX_HIST)
	uint8_t hist_ptr;					  //pointer address pointing to
	uint8_t hist_chk;					  //checks how many times the hist_up-hist_down command used(max=3)
	uint8_t hist_fill;					  //points to buffer in history to fill
	uint8_t hist_cnt;					  //number of commands in history, liited to MAX

} terminal_t; 

uint8_t terms_on;	//flag to show terminals initialized
uint8_t current_term;	//terminal running process
uint8_t viewing_term;	//terminal currently in view
terminal_t terminal[NUM_TERM];


//get code from keyboard on interrupt
uint8_t getScancode(void);

//get char from scan code
uint8_t getchar(uint8_t code);

// Initialize Keyboard  
void init_board(void);

// Keyboard Interrupt Handler 
void keyboard_handler(void);

// character key is pressed
void key_press(uint8_t scancode);

// Clear key board buffer 
void clear_key_buf(uint8_t term);

//initializes terminal stuff (or nothing), return 0
int32_t terminal_open(const uint8_t* filename);

//clears any terminal specific variables (or do nothing), return 0 
int32_t terminal_close(int32_t fd);

//reads FROM the keyboard buffer into buf, return number of bytes read
int32_t terminal_read(int32_t fd, void* buf, int32_t nbytes);/////////////////////////need to fix for proper call- read into buffer passed//////////////

// writes TO the screen from buf, return number of bytes written or -1 on error
int32_t terminal_write(int32_t fd, const void* buf, int32_t nbytes);

//need function to initialize all three terminals
void terminal_init(void);

//need function to switch terminals
void terminal_switch(uint8_t prev, uint8_t terminal_id);

//clear page for video memory buffer per terminal
void term_clear(uint8_t term);

//terminal command history
void terminal_history(int8_t direction);

//putc for terminals
void term_putc(uint8_t c, uint8_t term);

//puts for terminals
int32_t term_puts(int8_t* s,uint8_t term);

//clear the buffer passed in
void clear_buf(uint8_t* buff, uint8_t size);

#endif /* _KEYBOARD_H */
