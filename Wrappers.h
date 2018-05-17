#ifndef _WRAPPERS_H
#define _WRAPPERS_H

/* Declares assembly linker code for each of the 
 * the specified handlers. 
 *
 * Required for convention as stated in lab lecture
*/

//assembly linkage for keyboard
extern void keyboard_ASM_wrapper();
 
//assembly linkage for PIC
//void PIC_ASM_wrapper();

//assembly linkage for RTC
extern void RTC_ASM_wrapper();

//assembly linkage for System calls
extern void sys_call_handler();
 
//assembly linkage for PIT
extern void PIT_ASM_wrapper();


#endif
