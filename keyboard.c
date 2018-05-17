/* keyboard.c - Functions to output information from keyboard driver
 * vim:ts=4 noexpandtab
 */

#include "keyboard.h"
#include "lib.h"
#include "i8259.h"
#include "types.h"
#include "syscall.h"
#include "x86_desc.h"
#include "paging.h"


 /*  NOTE getScancode(),getChar() are taken from OSDev.org */
 /*
PS/2 keyboard code.
Dependencies: inb function and scancode table.
*/

//scancode array holds symbols for printing indexed by scancode
//128 is size of array as given by OSDev

//Need more arrays for shift and caps
static uint8_t scancode[KEY_MODES][KEY_BUFF_SIZE] = 
{
// Caps Lock OFF / Shift OFF
   {/*There is no 0x00*/NOKE, /*Escape    - 0x01*/NOKE, /*1 pressed - 0x02*/ '1', /*2 pressed - 0x03*/ '2',
	/*3 pressed - 0x04*/ '3', /*4 pressed - 0x05*/ '4', /*5 pressed - 0x06*/ '5', /*6 pressed - 0x07*/ '6', 
	/*7 pressed - 0x08*/ '7', /*8 pressed - 0x09*/ '8', /*9 pressed - 0x0A*/ '9', /*0 pressed - 0x0B*/ '0', 
	/*- pressed - 0x0C*/ '-', /*= pressed - 0x0D*/ '=', /*backspace - 0x0E*/NOKE, /*tab press - 0x0F*/NOKE, 
	/*q pressed - 0x10*/ 'q', /*w pressed - 0x11*/ 'w', /*e pressed - 0x12*/ 'e', /*r pressed - 0x13*/ 'r',
	/*t pressed - 0x14*/ 't', /*y pressed - 0x15*/ 'y', /*u pressed - 0x16*/ 'u', /*i pressed - 0x17*/ 'i', 
	/*o pressed - 0x18*/ 'o', /*p pressed - 0x19*/ 'p', /*[ pressed - 0x1A*/ '[', /*] pressed - 0x1B*/ ']', 
	/*ENTER     - 0x1C*/NOKE, /*lctl press- 0x1D*/NOKE, /*a pressed - 0x1E*/ 'a', /*s pressed - 0x1F*/ 's', 
	/*d pressed - 0x20*/ 'd', /*f pressed - 0x21*/ 'f', /*g pressed - 0x22*/ 'g', /*h pressed - 0x23*/ 'h', 
	/*j pressed - 0x24*/ 'j', /*k pressed - 0x25*/ 'k', /*l pressed - 0x26*/ 'l', /*; pressed - 0x27*/ ';', 
	/*' pressed - 0x28*/'\'', /*` pressed - 0x29*/ '`', /*lshift    - 0x2A*/NOKE, /*\ pressed - 0x2B*/'\\', 
	/*z pressed - 0x2C*/ 'z', /*x pressed - 0x2D*/ 'x', /*c pressed - 0x2E*/ 'c', /*v pressed - 0x2F*/ 'v', 
	/*b pressed - 0x30*/ 'b', /*n pressed - 0x31*/ 'n', /*m pressed - 0x32*/ 'm', /*, pressed - 0x33*/ ',', 
	/*. pressed - 0x34*/ '.', /*/ pressed - 0x35*/ '/', /*Rshift    - 0x36*/NOKE, /** pressed - 0x37*/ '*', 
	/* LAlt     - 0x38*/NOKE, /* space    - 0x39*/ ' ', /* CapsLock - 0x3A*/NOKE, /*F1pressed - 0x3B*/NOKE, 
	/*F2pressed - 0x3C*/NOKE, /*F3pressed - 0x3D*/NOKE, /*F4pressed - 0x3E*/NOKE, /*F5pressed - 0x3F*/NOKE, 
	/*F6pressed - 0x40*/NOKE, /*F7pressed - 0x41*/NOKE, /*F8pressed - 0x42*/NOKE, /*F9pressed - 0x43*/NOKE, 
	/*F10pressed- 0x44*/NOKE, /*Numlock   - 0x45*/NOKE, /*ScrollLock- 0x46*/NOKE, /*7 keypad  - 0x47*/ '7', 
	/* 8 keypad - 0x48*/ '8', /*9 keypad  - 0x49*/ '9', /*- keypad  - 0x4A*/ '-', /* 4 keypad - 0x4B*/ '4', 
	/* 5 keypad - 0x4C*/ '5', /*6 keypad  - 0x4D*/ '6', /*+ keypad  - 0x4E*/ '+', /* 1 keypad - 0x4F*/ '1', 
	/* 2 keypad - 0x50*/ '2', /*3 keypad  - 0x51*/ '3', /*0 keypad  - 0x52*/ '0', /* . keypad - 0x53*/ '.', 
	/* Nothing  - 0x54*/NOKE, /* Nothing  - 0x55*/NOKE, /* Nothing  - 0x56*/NOKE, /* F11press - 0x57*/NOKE, 
	/*F12press  - 0x58*/NOKE, /* Nothing  - 0x59*/NOKE, /* Nothing  - 0x5A*/NOKE, /* Nothing  - 0x5B*/NOKE,
	/* Nothing  - 0x5C*/NOKE, /* Nothing  - 0x5D*/NOKE, /* Nothing  - 0x5E*/NOKE, /* Nothing  - 0x5F*/NOKE, 
	/* Nothing  - 0x60*/NOKE, /* Nothing  - 0x61*/NOKE, /* Nothing  - 0x62*/NOKE, /* Nothing  - 0x63*/NOKE,
	/* Nothing  - 0x64*/NOKE, /* Nothing  - 0x65*/NOKE, /* Nothing  - 0x66*/NOKE, /* Nothing  - 0x67*/NOKE,
	/* Nothing  - 0x68*/NOKE, /* Nothing  - 0x69*/NOKE, /* Nothing  - 0x6A*/NOKE, /* Nothing  - 0x6B*/NOKE, 
	/* Nothing  - 0x6C*/NOKE, /* Nothing  - 0x6D*/NOKE, /* Nothing  - 0x6E*/NOKE, /* Nothing  - 0x6F*/NOKE,
	/* Nothing  - 0x70*/NOKE, /* Nothing  - 0x71*/NOKE, /* Nothing  - 0x72*/NOKE, /* Nothing  - 0x73*/NOKE,
	/* Nothing  - 0x74*/NOKE, /* Nothing  - 0x75*/NOKE, /* Nothing  - 0x76*/NOKE, /* Nothing  - 0x77*/NOKE, 
	/* Nothing  - 0x78*/NOKE, /* Nothing  - 0x79*/NOKE, /* Nothing  - 0x7A*/NOKE, /* Nothing  - 0x7B*/NOKE,
	/* Nothing  - 0x7C*/NOKE, /* Nothing  - 0x7D*/NOKE, /* Nothing  - 0x7E*/NOKE, /* Nothing  - 0x7F*/NOKE },

//Caps Lock ON/ Shift OFF
   {/*There is no 0x00*/NOKE, /*Escape    - 0x01*/NOKE, /*1 pressed - 0x02*/ '1', /*2 pressed - 0x03*/ '2',
	/*3 pressed - 0x04*/ '3', /*4 pressed - 0x05*/ '4', /*5 pressed - 0x06*/ '5', /*6 pressed - 0x07*/ '6', 
	/*7 pressed - 0x08*/ '7', /*8 pressed - 0x09*/ '8', /*9 pressed - 0x0A*/ '9', /*0 pressed - 0x0B*/ '0', 
	/*- pressed - 0x0C*/ '-', /*= pressed - 0x0D*/ '=', /*backspace - 0x0E*/NOKE, /*tab press - 0x0F*/NOKE, 
	/*q pressed - 0x10*/ 'Q', /*w pressed - 0x11*/ 'W', /*e pressed - 0x12*/ 'E', /*r pressed - 0x13*/ 'R',
	/*t pressed - 0x14*/ 'T', /*y pressed - 0x15*/ 'Y', /*u pressed - 0x16*/ 'U', /*i pressed - 0x17*/ 'I', 
	/*o pressed - 0x18*/ 'O', /*p pressed - 0x19*/ 'P', /*[ pressed - 0x1A*/ '[', /*] pressed - 0x1B*/ ']', 
	/*ENTER     - 0x1C*/NOKE, /*lctl press- 0x1D*/NOKE, /*a pressed - 0x1E*/ 'A', /*s pressed - 0x1F*/ 'S', 
	/*d pressed - 0x20*/ 'D', /*f pressed - 0x21*/ 'F', /*g pressed - 0x22*/ 'G', /*h pressed - 0x23*/ 'H', 
	/*j pressed - 0x24*/ 'J', /*k pressed - 0x25*/ 'K', /*l pressed - 0x26*/ 'L', /*; pressed - 0x27*/ ';', 
	/*' pressed - 0x28*/'\'', /*` pressed - 0x29*/ '`', /*lshift    - 0x2A*/NOKE, /*\ pressed - 0x2B*/'\\', 
	/*z pressed - 0x2C*/ 'Z', /*x pressed - 0x2D*/ 'X', /*c pressed - 0x2E*/ 'C', /*v pressed - 0x2F*/ 'V', 
	/*b pressed - 0x30*/ 'B', /*n pressed - 0x31*/ 'N', /*m pressed - 0x32*/ 'M', /*, pressed - 0x33*/ ',', 
	/*. pressed - 0x34*/ '.', /*/ pressed - 0x35*/ '/', /*Rshift    - 0x36*/NOKE, /** pressed - 0x37*/ '*', 
	/* LAlt     - 0x38*/NOKE, /* space    - 0x39*/ ' ', /* CapsLock - 0x3A*/NOKE, /*F1pressed - 0x3B*/NOKE, 
	/*F2pressed - 0x3C*/NOKE, /*F3pressed - 0x3D*/NOKE, /*F4pressed - 0x3E*/NOKE, /*F5pressed - 0x3F*/NOKE, 
	/*F6pressed - 0x40*/NOKE, /*F7pressed - 0x41*/NOKE, /*F8pressed - 0x42*/NOKE, /*F9pressed - 0x43*/NOKE, 
	/*F10pressed- 0x44*/NOKE, /*Numlock   - 0x45*/NOKE, /*ScrollLock- 0x46*/NOKE, /*7 keypad  - 0x47*/ '7', 
	/* 8 keypad - 0x48*/ '8', /*9 keypad  - 0x49*/ '9', /*- keypad  - 0x4A*/ '-', /* 4 keypad - 0x4B*/ '4', 
	/* 5 keypad - 0x4C*/ '5', /*6 keypad  - 0x4D*/ '6', /*+ keypad  - 0x4E*/ '+', /* 1 keypad - 0x4F*/ '1', 
	/* 2 keypad - 0x50*/ '2', /*3 keypad  - 0x51*/ '3', /*0 keypad  - 0x52*/ '0', /* . keypad - 0x53*/ '.', 
	/* Nothing  - 0x54*/NOKE, /* Nothing  - 0x55*/NOKE, /* Nothing  - 0x56*/NOKE, /* F11press - 0x57*/NOKE, 
	/*F12press  - 0x58*/NOKE, /* Nothing  - 0x59*/NOKE, /* Nothing  - 0x5A*/NOKE, /* Nothing  - 0x5B*/NOKE,
	/* Nothing  - 0x5C*/NOKE, /* Nothing  - 0x5D*/NOKE, /* Nothing  - 0x5E*/NOKE, /* Nothing  - 0x5F*/NOKE, 
	/* Nothing  - 0x60*/NOKE, /* Nothing  - 0x61*/NOKE, /* Nothing  - 0x62*/NOKE, /* Nothing  - 0x63*/NOKE,
	/* Nothing  - 0x64*/NOKE, /* Nothing  - 0x65*/NOKE, /* Nothing  - 0x66*/NOKE, /* Nothing  - 0x67*/NOKE,
	/* Nothing  - 0x68*/NOKE, /* Nothing  - 0x69*/NOKE, /* Nothing  - 0x6A*/NOKE, /* Nothing  - 0x6B*/NOKE, 
	/* Nothing  - 0x6C*/NOKE, /* Nothing  - 0x6D*/NOKE, /* Nothing  - 0x6E*/NOKE, /* Nothing  - 0x6F*/NOKE,
	/* Nothing  - 0x70*/NOKE, /* Nothing  - 0x71*/NOKE, /* Nothing  - 0x72*/NOKE, /* Nothing  - 0x73*/NOKE,
	/* Nothing  - 0x74*/NOKE, /* Nothing  - 0x75*/NOKE, /* Nothing  - 0x76*/NOKE, /* Nothing  - 0x77*/NOKE, 
	/* Nothing  - 0x78*/NOKE, /* Nothing  - 0x79*/NOKE, /* Nothing  - 0x7A*/NOKE, /* Nothing  - 0x7B*/NOKE,
	/* Nothing  - 0x7C*/NOKE, /* Nothing  - 0x7D*/NOKE, /* Nothing  - 0x7E*/NOKE, /* Nothing  - 0x7F*/NOKE },

//Caps Lock OFF/ Shift ON
   {/*There is no 0x00*/NOKE, /*Escape    - 0x01*/NOKE, /*1 pressed - 0x02*/ '!', /*2 pressed - 0x03*/ '@',
	/*3 pressed - 0x04*/ '#', /*4 pressed - 0x05*/ '$', /*5 pressed - 0x06*/ '%', /*6 pressed - 0x07*/ '^', 
	/*7 pressed - 0x08*/ '&', /*8 pressed - 0x09*/ '*', /*9 pressed - 0x0A*/ '(', /*0 pressed - 0x0B*/ ')', 
	/*- pressed - 0x0C*/ '_', /*= pressed - 0x0D*/ '+', /*backspace - 0x0E*/NOKE, /*tab press - 0x0F*/NOKE, 
	/*q pressed - 0x10*/ 'Q', /*w pressed - 0x11*/ 'W', /*e pressed - 0x12*/ 'E', /*r pressed - 0x13*/ 'R',
	/*t pressed - 0x14*/ 'T', /*y pressed - 0x15*/ 'Y', /*u pressed - 0x16*/ 'U', /*i pressed - 0x17*/ 'I', 
	/*o pressed - 0x18*/ 'O', /*p pressed - 0x19*/ 'P', /*[ pressed - 0x1A*/ '{', /*] pressed - 0x1B*/ '}', 
	/*ENTER     - 0x1C*/NOKE, /*lctl press- 0x1D*/NOKE, /*a pressed - 0x1E*/ 'A', /*s pressed - 0x1F*/ 'S', 
	/*d pressed - 0x20*/ 'D', /*f pressed - 0x21*/ 'F', /*g pressed - 0x22*/ 'G', /*h pressed - 0x23*/ 'H', 
	/*j pressed - 0x24*/ 'J', /*k pressed - 0x25*/ 'K', /*l pressed - 0x26*/ 'L', /*; pressed - 0x27*/ ':', 
	/*' pressed - 0x28*/'\"', /*` pressed - 0x29*/ '~', /*lshift    - 0x2A*/NOKE, /*\ pressed - 0x2B*/'|', 
	/*z pressed - 0x2C*/ 'Z', /*x pressed - 0x2D*/ 'X', /*c pressed - 0x2E*/ 'C', /*v pressed - 0x2F*/ 'V', 
	/*b pressed - 0x30*/ 'B', /*n pressed - 0x31*/ 'N', /*m pressed - 0x32*/ 'M', /*, pressed - 0x33*/ '<', 
	/*. pressed - 0x34*/ '>', /*/ pressed - 0x35*/ '?', /*Rshift    - 0x36*/NOKE, /** pressed - 0x37*/ '*', 
	/* LAlt     - 0x38*/NOKE, /* space    - 0x39*/ ' ', /* CapsLock - 0x3A*/NOKE, /*F1pressed - 0x3B*/NOKE, 
	/*F2pressed - 0x3C*/NOKE, /*F3pressed - 0x3D*/NOKE, /*F4pressed - 0x3E*/NOKE, /*F5pressed - 0x3F*/NOKE, 
	/*F6pressed - 0x40*/NOKE, /*F7pressed - 0x41*/NOKE, /*F8pressed - 0x42*/NOKE, /*F9pressed - 0x43*/NOKE, 
	/*F10pressed- 0x44*/NOKE, /*Numlock   - 0x45*/NOKE, /*ScrollLock- 0x46*/NOKE, /*7 keypad  - 0x47*/ '7', 
	/* 8 keypad - 0x48*/ '8', /*9 keypad  - 0x49*/ '9', /*- keypad  - 0x4A*/ '-', /* 4 keypad - 0x4B*/ '4', 
	/* 5 keypad - 0x4C*/ '5', /*6 keypad  - 0x4D*/ '6', /*+ keypad  - 0x4E*/ '+', /* 1 keypad - 0x4F*/ '1', 
	/* 2 keypad - 0x50*/ '2', /*3 keypad  - 0x51*/ '3', /*0 keypad  - 0x52*/ '0', /* . keypad - 0x53*/ '.', 
	/* Nothing  - 0x54*/NOKE, /* Nothing  - 0x55*/NOKE, /* Nothing  - 0x56*/NOKE, /* F11press - 0x57*/NOKE, 
	/*F12press  - 0x58*/NOKE, /* Nothing  - 0x59*/NOKE, /* Nothing  - 0x5A*/NOKE, /* Nothing  - 0x5B*/NOKE,
	/* Nothing  - 0x5C*/NOKE, /* Nothing  - 0x5D*/NOKE, /* Nothing  - 0x5E*/NOKE, /* Nothing  - 0x5F*/NOKE, 
	/* Nothing  - 0x60*/NOKE, /* Nothing  - 0x61*/NOKE, /* Nothing  - 0x62*/NOKE, /* Nothing  - 0x63*/NOKE,
	/* Nothing  - 0x64*/NOKE, /* Nothing  - 0x65*/NOKE, /* Nothing  - 0x66*/NOKE, /* Nothing  - 0x67*/NOKE,
	/* Nothing  - 0x68*/NOKE, /* Nothing  - 0x69*/NOKE, /* Nothing  - 0x6A*/NOKE, /* Nothing  - 0x6B*/NOKE, 
	/* Nothing  - 0x6C*/NOKE, /* Nothing  - 0x6D*/NOKE, /* Nothing  - 0x6E*/NOKE, /* Nothing  - 0x6F*/NOKE,
	/* Nothing  - 0x70*/NOKE, /* Nothing  - 0x71*/NOKE, /* Nothing  - 0x72*/NOKE, /* Nothing  - 0x73*/NOKE,
	/* Nothing  - 0x74*/NOKE, /* Nothing  - 0x75*/NOKE, /* Nothing  - 0x76*/NOKE, /* Nothing  - 0x77*/NOKE, 
	/* Nothing  - 0x78*/NOKE, /* Nothing  - 0x79*/NOKE, /* Nothing  - 0x7A*/NOKE, /* Nothing  - 0x7B*/NOKE,
	/* Nothing  - 0x7C*/NOKE, /* Nothing  - 0x7D*/NOKE, /* Nothing  - 0x7E*/NOKE, /* Nothing  - 0x7F*/NOKE },


// Caps Lock ON / Shift ON
   {/*There is no 0x00*/NOKE, /*Escape    - 0x01*/NOKE, /*1 pressed - 0x02*/ '!', /*2 pressed - 0x03*/ '@',
	/*3 pressed - 0x04*/ '#', /*4 pressed - 0x05*/ '$', /*5 pressed - 0x06*/ '%', /*6 pressed - 0x07*/ '^', 
	/*7 pressed - 0x08*/ '&', /*8 pressed - 0x09*/ '*', /*9 pressed - 0x0A*/ '(', /*0 pressed - 0x0B*/ ')', 
	/*- pressed - 0x0C*/ '_', /*= pressed - 0x0D*/ '+', /*backspace - 0x0E*/NOKE, /*tab press - 0x0F*/NOKE, 
	/*q pressed - 0x10*/ 'q', /*w pressed - 0x11*/ 'w', /*e pressed - 0x12*/ 'e', /*r pressed - 0x13*/ 'r',
	/*t pressed - 0x14*/ 't', /*y pressed - 0x15*/ 'y', /*u pressed - 0x16*/ 'u', /*i pressed - 0x17*/ 'i', 
	/*o pressed - 0x18*/ 'o', /*p pressed - 0x19*/ 'p', /*[ pressed - 0x1A*/ '{', /*] pressed - 0x1B*/ '}', 
	/*ENTER     - 0x1C*/NOKE, /*lctl press- 0x1D*/NOKE, /*a pressed - 0x1E*/ 'a', /*s pressed - 0x1F*/ 's', 
	/*d pressed - 0x20*/ 'd', /*f pressed - 0x21*/ 'f', /*g pressed - 0x22*/ 'g', /*h pressed - 0x23*/ 'h', 
	/*j pressed - 0x24*/ 'j', /*k pressed - 0x25*/ 'k', /*l pressed - 0x26*/ 'l', /*; pressed - 0x27*/ ':', 
	/*' pressed - 0x28*/'\"', /*` pressed - 0x29*/ '~', /*lshift    - 0x2A*/NOKE, /*\ pressed - 0x2B*/ '|', 
	/*z pressed - 0x2C*/ 'z', /*x pressed - 0x2D*/ 'x', /*c pressed - 0x2E*/ 'c', /*v pressed - 0x2F*/ 'v', 
	/*b pressed - 0x30*/ 'b', /*n pressed - 0x31*/ 'n', /*m pressed - 0x32*/ 'm', /*, pressed - 0x33*/ '<', 
	/*. pressed - 0x34*/ '>', /*/ pressed - 0x35*/ '?', /*Rshift    - 0x36*/NOKE, /* * keypad - 0x37*/ '*', 
	/* LAlt     - 0x38*/NOKE, /* Space    - 0x39*/ ' ', /* CapsLock - 0x3A*/NOKE, /*F1pressed - 0x3B*/NOKE, 
	/*F2pressed - 0x3C*/NOKE, /*F3pressed - 0x3D*/NOKE, /*F4pressed - 0x3E*/NOKE, /*F5pressed - 0x3F*/NOKE, 
	/*F6pressed - 0x40*/NOKE, /*F7pressed - 0x41*/NOKE, /*F8pressed - 0x42*/NOKE, /*F9pressed - 0x43*/NOKE, 
	/*F10pressed- 0x44*/NOKE, /*Numlock   - 0x45*/NOKE, /*ScrollLock- 0x46*/NOKE, /*7 keypad  - 0x47*/ '7', 
	/* 8 keypad - 0x48*/ '8', /*9 keypad  - 0x49*/ '9', /*- keypad  - 0x4A*/ '-', /* 4 keypad - 0x4B*/ '4', 
	/* 5 keypad - 0x4C*/ '5', /*6 keypad  - 0x4D*/ '6', /*+ keypad  - 0x4E*/ '+', /* 1 keypad - 0x4F*/ '1', 
	/* 2 keypad - 0x50*/ '2', /*3 keypad  - 0x51*/ '3', /*0 keypad  - 0x52*/ '0', /* . keypad - 0x53*/ '.', 
	/* Nothing  - 0x54*/NOKE, /* Nothing  - 0x55*/NOKE, /* Nothing  - 0x56*/NOKE, /* F11press - 0x57*/NOKE, 
	/*F12press  - 0x58*/NOKE, /* Nothing  - 0x59*/NOKE, /* Nothing  - 0x5A*/NOKE, /* Nothing  - 0x5B*/NOKE,
	/* Nothing  - 0x5C*/NOKE, /* Nothing  - 0x5D*/NOKE, /* Nothing  - 0x5E*/NOKE, /* Nothing  - 0x5F*/NOKE, 
	/* Nothing  - 0x60*/NOKE, /* Nothing  - 0x61*/NOKE, /* Nothing  - 0x62*/NOKE, /* Nothing  - 0x63*/NOKE,
	/* Nothing  - 0x64*/NOKE, /* Nothing  - 0x65*/NOKE, /* Nothing  - 0x66*/NOKE, /* Nothing  - 0x67*/NOKE,
	/* Nothing  - 0x68*/NOKE, /* Nothing  - 0x69*/NOKE, /* Nothing  - 0x6A*/NOKE, /* Nothing  - 0x6B*/NOKE, 
	/* Nothing  - 0x6C*/NOKE, /* Nothing  - 0x6D*/NOKE, /* Nothing  - 0x6E*/NOKE, /* Nothing  - 0x6F*/NOKE,
	/* Nothing  - 0x70*/NOKE, /* Nothing  - 0x71*/NOKE, /* Nothing  - 0x72*/NOKE, /* Nothing  - 0x73*/NOKE,
	/* Nothing  - 0x74*/NOKE, /* Nothing  - 0x75*/NOKE, /* Nothing  - 0x76*/NOKE, /* Nothing  - 0x77*/NOKE, 
	/* Nothing  - 0x78*/NOKE, /* Nothing  - 0x79*/NOKE, /* Nothing  - 0x7A*/NOKE, /* Nothing  - 0x7B*/NOKE,
	/* Nothing  - 0x7C*/NOKE, /* Nothing  - 0x7D*/NOKE, /* Nothing  - 0x7E*/NOKE, /* Nothing  - 0x7F*/NOKE }
 };
 //special key flags
	uint8_t Cap_flag;				//CAP key flag
	uint8_t Shift_flag;			    //Shift key flag
	uint8_t Alt_flag;				//Alt flag
	uint8_t Ctl_flag; 				//Control
    uint8_t Num_flag;
	

/*init_board
*	DESCRIPTION: initialize keyboard for use and set flags
*	INPUT: none
*	OUTPUT: none
*	RETURN: none
*	SIDE EFFECTS: links keyboard to PIC through IRQ 1
*/
void init_board(void)
{
	// set PIC IRQ
	enable_irq(KEYBOARD_IRQ_NUM);	
	
	//Clear/init flags
	Cap_flag = UNPRESS;
	Shift_flag = UNPRESS;
	Alt_flag = UNPRESS;
	Ctl_flag = UNPRESS; 
	Num_flag = UNPRESS;
}


/*getScancode
*	DESCRIPTION: get scan code from key board 
*	INPUT: none
*	OUTPUT: none
*	RETURN:scan code from keyboard
*	SIDE EFFECTS: none
*/
uint8_t getScancode(void) 
{   
    return inb(KEYBOARD_PORT); //set c to value   
}
 

/*getchar
*	DESCRIPTION: uses scan code to determine symbol to print
*	INPUT: none
*	OUTPUT: none
*	RETURN: none
*	SIDE EFFECTS: none
*/
uint8_t getchar(uint8_t code) 
{
	//need to determine character to return based on flags 
	if(!Shift_flag && !Cap_flag)	//normal keys
		return scancode[0][code];  

    else if(!Shift_flag && Cap_flag)//CAPS ON
		return scancode[1][code];  

	else if(Shift_flag && !Cap_flag)//SHIFT ON
		return scancode[2][code];  

	else 
		return scancode[3][code]; 	//SHIFT ON AND CAPS ON 						  
}


/*keyboard_handler
*	DESCRIPTION: manages all functions of keyboard
*	INPUT: none
*	OUTPUT: none
*	RETURN: none
*/
void keyboard_handler(void)
{
	
	//get status
	uint8_t pressed = getScancode();
	//disable interrupts temporarily as the keyboard is being processed 
	cli();
	//decide what to do with key press 
	terminal_t* term = &(terminal[viewing_term]);
	switch(pressed)
	{
		case BACKSPACE_PRESS:
				Backspace(viewing_term);
			break;
		
		case TAB_PRESS:    
			if(term->keyboard_buf_idx+TAB_SIZE < KEY_BUFF_SIZE)
			{
				uint8_t	index;
				for(index = 0; index < TAB_SIZE; index++)
				{
					term->keyboard_buffer[term->keyboard_buf_idx] = SPACE; //put spaces for tab
					term->keyboard_buf_idx++;
					term_putc(SPACE, viewing_term);
				}
			}
			break;
		
		case ENTER_PRESS:
			term->Enter_flag = PRESS;
			term->keyboard_buffer[term->keyboard_buf_idx] = NEWLINE; //put spaces for tab
			term->keyboard_buf_idx++;
			//copy keyboard buffer into history
			clear_buf(term->history_buf[term->hist_fill], KEY_BUFF_SIZE);
		    memcpy( (term->history_buf[term->hist_fill]), (term->keyboard_buffer), term->keyboard_buf_idx-1 );
		    term->hist_fill = (term->hist_fill + 1) % MAX_HIST;
		    term->hist_ptr = term->hist_fill;
		    term->hist_chk = 0; //reset use
		    term->hist_cnt++;
		    Enter(viewing_term);
		    if(term->hist_cnt > MAX_HIST){term->hist_cnt = MAX_HIST;}//limit to 3 searches max
			break;
		
		case CAPS_PRESS:
			Cap_flag = ~Cap_flag;
			break;
		case NUMLOCK_PRESS:
			if(Num_flag == PRESS)
			{
				Num_flag = UNPRESS;
			}
			else
			{
				Num_flag = PRESS;
			}
			break;
		
		case LSHIFT_PRESS:  
			if(Shift_flag == PRESS){/*DO NOTHING*/}
			else
			{
				Shift_flag = PRESS;
			}
			break;
		
		case LSHIFT_RELEASE:
			if(Shift_flag == UNPRESS){/*DO NOTHING*/}
			else
			{
				Shift_flag = UNPRESS;
			}
			break;
		
		case RSHIFT_PRESS:
			if(Shift_flag == PRESS){/*DO NOTHING*/}
			else
				Shift_flag = PRESS;
			break;
		
		case RSHIFT_RELEASE: 
			if(Shift_flag == UNPRESS){/*DO NOTHING*/}
			else
			{
				Shift_flag = UNPRESS;
			}
			break;   
		
		case LALT_PRESS:
			if(Alt_flag == UNPRESS)
			{
				Alt_flag = PRESS;
			}
			break;
		
		case LALT_RELEASE:  
			if(Alt_flag == PRESS)
			{
				Alt_flag = UNPRESS;
			}
			break;   
		
		case LCTRL_PRESS:
			if(Ctl_flag == UNPRESS)
			{
				Ctl_flag = PRESS;
			}
			break;
		
		case LCTRL_RELEASE:
			if(Ctl_flag == PRESS)
			{
				Ctl_flag = UNPRESS;
			}
			break;
		
		case L_PRESS:
			if(Ctl_flag == UNPRESS)
			{
				key_press(pressed);
			}
			else
			{	//clear screen function called
				term_clear(viewing_term);
			}
			break;

		case F1_PRESS:
			if(Alt_flag)
			{
				terminal_switch(viewing_term, TERM0);
			}
			break;

		case F2_PRESS:
			if(Alt_flag)
			{
				terminal_switch(viewing_term, TERM1);
			}
			break;

		case F3_PRESS:
			if(Alt_flag)
			{
				terminal_switch(viewing_term, TERM2);
			}
			break;
		case UP_KEY:
			if(Num_flag== UNPRESS)
			{
				terminal_history(UP_ARROW);
			}
			else
			{
				key_press(pressed);
			}
			break;

		case DOWN_KEY:
			if(Num_flag== UNPRESS)
			{
				terminal_history(DOWN_ARROW);
			}
			else
			{
				key_press(pressed);
			}
			break;

		default:
			key_press(pressed);
			break;
	}
	//finished handling the keyboard	
	send_eoi(KEYBOARD_IRQ_NUM); 

	//re-enable interrupts on exit
	sti();
}


/*key_press
*	DESCRIPTION: determines what to do with scancode 
*	INPUT: scancode -- code from keyboard for what was pressed
*	OUTPUT: none
*	RETURN: none
*   SIDE EFFECT: prints to terminal
*/
void key_press(uint8_t scancode)
{
	//Not a release code which are all greater than 0x80
	if(scancode < KEY_BUFF_SIZE)
	{
		//will need to check if within buffer size and add to buffer
		uint8_t key = getchar(scancode);

		if(terminal[viewing_term].keyboard_buf_idx < KEY_BUFF_SIZE-1) //only allow 127 charcaters plus enter hence -1
		{
			if(key == NOKE){return;}

			else
			{
				terminal[viewing_term].keyboard_buffer[terminal[viewing_term].keyboard_buf_idx] = key;
				terminal[viewing_term].keyboard_buf_idx++;
				term_putc(key, viewing_term);	//print to terminal or use putc????
			}
		}
	}
	return;
}


/*clear_key_buf
*	DESCRIPTION: Fills the buffer with NOKE to clear it
*	INPUT: term -- terminal that is having its buffer cleared
*	OUTPUT: NONE
*	RETURN: NONE
*   SIDE EFFECT: sets the keyboard_buf_idx to 0
*/
void clear_key_buf(uint8_t term)
{
	uint8_t index;
	for(index = 0; index < KEY_BUFF_SIZE; index++)
	{
		terminal[term].keyboard_buffer[index] = NOKE;
	}
	terminal[term].keyboard_buf_idx = 0;
}


/*terinal_open
*	DESCRIPTION: initializes terminal(s) variables
*	INPUT: NONE
*	OUTPUT: NONE
*	RETURN: zero as always succesful in opening as always open
*   SIDE EFFECT: NONE
*/
int32_t terminal_open(const uint8_t* filename)
{
	return 0;
}


/*terminal_close
*	DESCRIPTION: close terminal //do nothing
*	INPUT: NONE
*	OUTPUT: NONE
*	RETURN: zero as alway open
*   SIDE EFFECT: NONE
*/
int32_t terminal_close(int32_t fd)
{
	return 0;
}


/*terminal_read
*	DESCRIPTION: copies the keyboard buffer into the read buffer
*	INPUT: fd -- file descriptor
		   buf -- buffer being read into
		   nbytes -- number of bytes to read
*	OUTPUT: NONE
*	RETURN: number of bytes read
*   SIDE EFFECT: clears keyboard buffer
*/
int32_t terminal_read(int32_t fd, void* buf, int32_t nbytes)
{
	//read from keyboard buffer into terminal buff
	uint8_t index = 0;			//index for buffers copied and stored
	//uint8_t reading = 0;	//number of bytes read in
	uint8_t limit = KEY_BUFF_SIZE;
	terminal[current_term].read_wanted = 1;   //set flag if read wanted

	if(buf ==NULL || nbytes < 0){return -1;}

	if(nbytes < limit){limit = nbytes;} //can only read up to max buffer size

	while(terminal[current_term].Enter_flag == UNPRESS) {/* allow blocking until enter pressed*/}
	
	//adjust limit to size of buff index if necessary
	if(limit > terminal[current_term].keyboard_buf_idx){limit = terminal[current_term].keyboard_buf_idx;}
	
	while(index < limit)
	{
		((uint8_t*)buf)[index] = terminal[current_term].keyboard_buffer[index];
		index++;
	} 

	terminal[current_term].Enter_flag = UNPRESS;
	clear_key_buf(current_term);//clear the keyboard buffer as stated in the documentation
	
	terminal[current_term].read_wanted = 0;  //reset read flag
	
	return index; //will return number of bytes read
}


/*terminal_write
*	DESCRIPTION: prints out nbytes from null terminated string buffer
*	INPUT: fd -- file descriptor
		   buff -- buffer containing characters being written to terminal
		   nbytes -- number of bytes to write to terminal
*	OUTPUT: NONE
*	RETURN: number of bytes printed, -1 if error occurs
*   SIDE EFFECT: none
*/
int32_t terminal_write(int32_t fd, const void* buf, int32_t nbytes)
{
	int32_t index =0;			//place in buffer

	if(buf ==NULL || nbytes < 0 || fd < 0 || fd >= MAX_FILE || fd == STDIN_IDX){ return -1;}  //nbytes greater than string so error -1
	
	while(nbytes > 0)	//print out nbytes from buffer
	{
		term_putc(((uint8_t*)buf)[index], current_term);  //write buff value to terminal
		index++;
		nbytes--;
	}

	return index;//return number of bytes written
}


/*terminal_init
*	DESCRIPTION: initializes terminal variables and video memory paging for 3 terminals
*	INPUT: NONE
*	OUTPUT: NONE
*	RETURN: NONE
*   SIDE EFFECT: NONE
*/
void terminal_init(void)
{
	uint8_t term;
	uint8_t hist;
	uint32_t index;

	//set current terminal to first terminal
	current_term = TERM0;
	viewing_term = TERM0;
	//set terminal flags starting positions
	for(term = 0; term < NUM_TERM; term++ )
	{
		terminal[term].screen_x = 0;		 //cursor x-position to start of x 
		terminal[term].screen_y = 0;		 //cursor y-position to start of y
		terminal[term].keyboard_buf_idx = 0; //set current position to start in keyboard buffer	
		terminal[term].Enter_flag = UNPRESS; //Clear enter flag
		terminal[term].read_wanted = UNPRESS;//clear read flag
		terminal[term].process = 10; 	 	 //running 0 arbtrarily
		total_progs[term] = 0;				 //no programs running 
		terminal[term].hist_ptr = 0;		 //current spot in history buffer being checked
		terminal[term].hist_chk = 0;		 //how many times have we checked history( 0 -> HIST_SIZE) 
		terminal[term].hist_fill = 0;		 //pointer to which history buffer is filled next
		
		//clear keyboard buffer and history buffer in each terminal
		for (index = 0; index < KEY_BUFF_SIZE; index ++)
		{
			terminal[term].keyboard_buffer[index] = NOKE;
		}
		//unique map to video memory needed for each term or bufferinng vidmem???????????????????
		if(term == TERM0)
		{
			terminal[term].term_video_mem = (uint8_t*)(MEM_132 + (term*PAGES_SIZE));//point to virtual address 
			map_term_to_vid(term);//map term0 to physial video memory
		}

		else
		{
			terminal[term].term_video_mem = (uint8_t*)(MEM_132 + (term*PAGES_SIZE));//point to virtual address 
			map_term_to_buf(term);	//map term1 and term2 to buffer pages
		}

	}
	//clear terminal history buffers
	for(term = 0; term < NUM_TERM; term ++)
	{
		for (hist=0;hist < MAX_HIST; hist ++)
		{
			for(index =0; index < KEY_BUFF_SIZE; index ++)
			{
				terminal[term].history_buf[hist][index] = NOKE;
			}
		} 
	}
	//set terminal text color
	terminal[TERM0].color = ATTRIB0;
	terminal[TERM1].color = ATTRIB1;
	terminal[TERM2].color = ATTRIB2;
	term_clear(TERM0);
	//clear tlb??
	
	process_cnt = 0;		//total number of processes across all terminals is 0
	terms_on = 1;
	//set screen position to start of screen
	move_cursor(TERM0);
	
}


/*terminal_switch
*	DESCRIPTION: change to another terminal window
*	INPUT: NONE
*	OUTPUT: NONE
*	RETURN: NONE
*   SIDE EFFECT:changes video paging/mapping
*/
void terminal_switch(uint8_t cur, uint8_t dest)
{

	//check for valid terminal number/diff from current?
	if(dest > NUM_TERM || dest == cur)
	{
		printf("Error: Invalid Terminal ID\n");
		return;
	}

	if( (total_progs[dest] == 0) && ((process_cnt+1) > MAX_NUM_PROC) )
	{
		printf("Error: At process limit\n");
		return;
	}

	//get current pcb for this terminal
	PCB_t* current = (PCB_t*)get_pcb_addr_C(terminal[cur].process);

	//save video memory in buffer page
	//1) point curent page to physical mem and copy video mem into buffer for current term
	map_term_to_buf(cur);
	//2) save videomem in buffer page of current terminal(vid buffer at virtual addr 132MB with page offset)
	memcpy(terminal[cur].term_video_mem, (void*)VIDEO, PAGES_SIZE );
	//3) copy buffer page of new terminal into video memory
	memcpy( (void*)VIDEO, terminal[dest].term_video_mem, PAGES_SIZE );
	//4) map new term page to video memory
	map_term_to_vid(dest);
	
	//change current terminal number to the one being switched to
	viewing_term = dest;
	move_cursor(dest);

	//check if running program
	if (total_progs[dest] == 0)
	{ 
		term_clear(dest);

		//store esp, par_pdbr, and ebp in curent pcb
		asm volatile (   
			"movl	%%esp, %0;" 
			"movl	%%ebp, %1;"        							
			: "=r"(current->this_esp), "=r"(current->this_ebp)
			: 
			: "memory"
		);
	
		//update terminal
		current_term = dest;
		send_eoi(KEYBOARD_IRQ_NUM);
		//execute shell since not running a program on new terminal
		execute((uint8_t*)"shell");
		printf("weird error\n");//should not reach
	}
}


/* term_clear
*	DESCRIPTION: clears the video buffer for the terminal
*	INPUT: NONE
*	OUTPUT: NONE
*	RETURN: NONE
*   SIDE EFFECT: 
*/
void term_clear(uint8_t term)
{
	uint8_t* buffer = terminal[term].term_video_mem;
	uint32_t index;
	for (index = 0; index < NUM_ROWS*NUM_COLS; index++)
	{
		buffer[index << 1] = SPACE;
        buffer[(index << 1) + 1] = terminal[term].color;
	}
	terminal[term].screen_x = ROW_START;
	terminal[term].screen_y = COL_START;
	//mark first line after clear with the bash prompt, may need to be changed
	term_puts((int8_t*)"391OS> ",term);
}


/*terminl history
*	DESCRIPTION: allows user to see previosly entered commands
*				 for reuse as well. Starts with most recent
*	INPUT: direction-- history up or down
*	OUTPUT: previous command on screen 
*	RETURN: NONE
*   SIDE EFFECT: clear and repopulate section of video memory and keyboard buffer
*				 moves cursor to new position
*/
void terminal_history(int8_t direction)
{
	//check valid direction, should never fail
	if(direction != DOWN_ARROW && direction != UP_ARROW){return;}

	terminal_t* term = &(terminal[viewing_term]);
	uint8_t index;
	//see if we reached history limit for up and down, act if history 
	if( ((term->hist_chk == term->hist_cnt) && (direction == UP_ARROW)) || term->hist_cnt == 0){return;}	
	else if( (term->hist_chk == 0) && (direction == DOWN_ARROW) ){return;}

	//update pointer to see next history buffer
	if( (term->hist_ptr + direction) < 0){ term->hist_ptr = term->hist_cnt-1;}
	else if ((term->hist_ptr + direction) >= term->hist_cnt){ term->hist_ptr = 0;}
	else{term->hist_ptr = term->hist_ptr + direction;}
	//clear keyboard buffer while updating position
	while(term->keyboard_buf_idx != 0)
	{
		Backspace(viewing_term);//clear mem and set position
	}

	//fill keyboard buffer with history buffer entry while updating position & index
	for(index = 0; index < KEY_BUFF_SIZE; index ++)
	{
		term->keyboard_buffer[index] = term->history_buf[term->hist_ptr][index] ;//copy history buffer into keyboard buffer
		if(term->history_buf[term->hist_ptr][index] == NOKE){break;}
		term_putc( term->history_buf[term->hist_ptr][index], viewing_term); //print character to screen(repopulate vidmem and move cursor)
		term->keyboard_buf_idx++;//increment index pointer
	}
	term->hist_chk -= direction; //update number of accesses (max is MAX_hist)
}


/*term_putc
*	DESCRIPTION: print charcater on corresponding terminal
*	INPUT: c -- character to print
		   term -- terminal being printed on
*	OUTPUT: character on screen
*	RETURN: NONE
*   SIDE EFFECT: modify video memory
*/
void term_putc(uint8_t c, uint8_t term)
{
	terminal_t* current = &(terminal[term]);
	uint8_t color = current->color;

	if(c == '\n' || c == '\r')
	{ 
		current->screen_x = ROW_START; 
	    current->screen_y++;
	    if(current->screen_y == NUM_ROWS){scroll_screen(term);}

	    else{move_cursor(term);} 
	} 

	else 
	{
		uint32_t index = (NUM_COLS * current->screen_y + current->screen_x);
		current->term_video_mem[index << 1 ] = c;
        current->term_video_mem[(index << 1) + 1] = color;

        current->screen_x++;
		if ( current->screen_x  == NUM_COLS) 
		{
			current->screen_x = 0;
			current->screen_y++;
			if ( (current->screen_y) == NUM_ROWS)
			{
				scroll_screen(term);
			}
		}
		move_cursor(term);
	}
	
}


/*term_puts
*	DESCRIPTION: print charcater on corresponding terminal
*	INPUT: s -- string to print
		   term -- terminal being printed on
*	OUTPUT: string on screen
*	RETURN: NONE
*   SIDE EFFECT: modify video memory
*/
int32_t term_puts(int8_t* s,uint8_t term)
{
	int32_t index = 0;
    while (s[index] != '\0') {
        term_putc(s[index], term);
        index++;
    }
    return index;
}



/*clear_buf
*	DESCRIPTION: clear thebuffer being passed in
*	INPUT: buff-- buffer to clear
		   size -- size of buffer
*	OUTPUT: NONE
*	RETURN: NONE
*   SIDE EFFECT: NONE
*/
void clear_buf(uint8_t* buff, uint8_t size)
{
	uint8_t index; 
	for(index = 0; index < size; index++)
	{
		buff[index] = NOKE;
	}

}

