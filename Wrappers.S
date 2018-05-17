
#define ASM 1
#include "x86_desc.h"

.globl sys_call_handler   #global pointer for system handler

   #Jump table for system calls
sys_call_table:   #0x0 used to keep index same as call number
	.long 0x0, halt, execute, read, write, open, close, getargs, vidmap #,set_handler,sigreturn


  #macro for asm wrappers
#define HANDLER(name,fn)	 \
.GLOBL name								;\
name:										  ;\
	pushal									;\
	pushfl									;\
	call fn									;\
	popfl									  ;\
	popal									  ;\
	iret									  ;\
 
HANDLER (keyboard_ASM_wrapper, keyboard_handler); #handler/wrapper for keyboard
HANDLER (RTC_ASM_wrapper, rtc_interrupt);		  #handler wrapper for RTC
HANDLER (PIT_ASM_wrapper, PIT_handler);     #handler wrapper for PIT


  #ASM wrapper to handle the parsing of the system call 
sys_call_handler:
  #save flags
  pushfl
  #save all segments ### needed????
  pushl %ds
  pushl %es
  pushl %fs
  pushl %gs
  #push registers and push arguments
  pushl %ebp
  pushl %edi
  pushl %esi
  pushl %edx
  pushl %ecx
  pushl %ebx

  #check if %eax value is valid (0-6 for Checkpoint 3)
  cmpl $1, %eax
  jl sys_call_error
  cmpl $8, %eax	#eventually make 10
  jg sys_call_error

  #index into jump table
  call *sys_call_table(,%eax,4)
  jmp end_int

sys_call_error:
  movl $-1, %eax

end_int:
  #restore registers and flag
  popl %ebx
  popl %ecx
  popl %edx
  popl %esi
  popl %edi
  popl %ebp

  popl %gs
  popl %fs
  popl %es
  popl %ds
  popfl
  #return
  iret


















