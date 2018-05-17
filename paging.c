/* paging.c - Functions for paging
 * 
 */
 
 /* CALL STUFF IN KERNAL.C MY DUDES */

#include "paging.h"
#include "lib.h"
#include "syscall.h"
#include "keyboard.h"

#define BOOTIMAGE_ADDR			0x08048000
#define KERNVIRT_ADDR			0x400000
#define PAGEDIR_MAP_ADDR		0x08000000 
#define BOOTIMG_OFFSET			0x00048000

#define VIDEO_MEM				0xB8000
#define VIDEO_IND				184
#define USER_
#define FOURMB					0x400000
#define EIGHTMB					0x800000

#define HEAD					0
#define FIRST_TABLE				1



/* 
 * init_paging
 *   DESCRIPTION: initializes the page directory
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: initializes the page directory
 *                 
 */
void init_paging(void){
	int i;
	
	//initialize page directory and page table
	for( i = 0; i < PDIR_ENT; i++)
	{
        page_dir_array[i] = F_W_E;	 //mark all entries as not present
        page_tab_array[i] = (i << 12) | F_W_E;//mark all entries as not present, one-to-one mapping, 4kB pages hence 12
		page_tab_array_video[i] = 0;//mark all entries as not present
	}

	// create PDE for 4KB pages between 0-4MB in physical memory
	page_dir_array[HEAD] = (((uint32_t)page_tab_array) & 0xFFFFF000)| F_PRESENT;
	// create PDE for kernel page between 4-8MB in physical memory
	page_dir_array[FIRST_TABLE] = FOURMB | F_PRESENT | F_MB4_PG | F_W_E; 
	//map virtual address to physical video memory
	page_tab_array[VIDEO_IND] = VIDEO_MEM | F_PRESENT | F_W_E; 


	// create 4kB entry for vidmapping 
    page_dir_array[VID_PAGE] = (uint32_t)(page_tab_array_video) | F_W_E |F_PRESENT | F_USR ;
    page_tab_array_video[HEAD] = VIDEO_MEM | F_W_E |F_PRESENT | F_USR;  

	asm volatile(
		//1-load directory flush tlb
		"movl %0, %%cr3;"
		//2-enable page extend
		"movl %%cr4, %%eax;"
        "orl  $0x00000010, %%eax;"
        "movl  %%eax, %%cr4;"
        //3- enable paging
        "movl %%cr0, %%eax;"
        "orl  $0x80000000, %%eax;"
        "movl %%eax, %%cr0;"
		:
		:"r"(page_dir_array)
		:"eax"
	);
	
}


/* 
 * flush_tlb
 *   DESCRIPTION: Flushes the tlb by loading first directory array   
 *   INPUTS: NONE
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: map CR3 to pag_dir_array  
 */
void flush_tlb(void)
{
	asm volatile(
		"movl %0, %%cr3;"
		:
		:"r"(page_dir_array)
	);
	
}



/* 
 * set_page_execute
 *   DESCRIPTION: sets paging from execute, remaps our user process and puts it in mem, changes processes      
 *   INPUTS: PID -- process number being mapped 
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: sets paging from execute. Maps virtual to physical memory              
 */

void set_page_execute(uint32_t PID)
{    
	  // create PDE for 4MB pages for the program after 8MB, and set entry flags
    page_dir_array[USER_ENT] = ((EIGHTMB+FOURMB*PID)| F_MB4_PG | F_W_E |F_PRESENT | F_USR);  

    flush_tlb();
}


/* 
 * map_term_to_buf
 *   DESCRIPTION: maps terminal video memory stuff to its virtual video buffer 
 *   INPUTS: term -- terminal being mapped to its buffer page
 *   OUTPUTS: NONE
 *   RETURN VALUE: NONE
 *   SIDE EFFECTS: flush tlb       
 */
void map_term_to_buf(uint8_t term)
{
	page_tab_array_video[term] = ((MEM_32 + (term*PAGES_SIZE)) | F_W_E |F_PRESENT | F_USR);
	flush_tlb();
}


/* 
 * map_term_to_vid
 *   DESCRIPTION: map terminal video memory stuff to physical video memory
 *   INPUTS: term -- terminal requesting to move cursor on viewing terminal
 *   OUTPUTS: NONE
 *   RETURN VALUE: NONE
 *   SIDE EFFECTS: NONE         
 */
void map_term_to_vid(uint8_t term)
{
	page_tab_array_video[term] = (VIDEO | F_W_E |F_PRESENT | F_USR );
	flush_tlb();
}




