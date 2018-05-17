/* paging.h - Defines what is used in paging
 * 
 * 
 */

#ifndef _PAGING_H
#define _PAGING_H

#include "lib.h"

#define PDIR_ENT		1024
#define PTABLE_ENT		1024
#define PAGES_SIZE		4096
#define MEM_128			0x08000000
#define MEM_132			0x08400000
#define MEM_136         0x8800000
#define MEM_32          0x2000000
#define VIDEO  			0xB8000
#define F_PRESENT		0x01
#define F_W_E		    0x02
#define F_MB4_PG		0x80
#define F_GLOBL			0x100
#define F_USR			0x04
#define USER_ENT        32			//map to virtual adress 128-132MB
#define VID_PAGE        33			//map to virtual address 132MB


uint32_t page_dir_array[PDIR_ENT] __attribute__((aligned (PAGES_SIZE)));

uint32_t page_tab_array[PTABLE_ENT] __attribute__((aligned (PAGES_SIZE)));

uint32_t page_tab_array_video[PTABLE_ENT] __attribute__((aligned (PAGES_SIZE)));


void flush_tlb(void);
void init_paging(void);
void set_page_execute(uint32_t PID);
void map_term_to_buf(uint8_t term);
void map_term_to_vid(uint8_t term);

#endif /* _PAGING_H */

