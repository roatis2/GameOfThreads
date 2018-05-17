/* file_system.c - Functions used in interactions with the file system
 * 
 */
#include "filesystem.h"
#include "lib.h"
#include "syscall.h"
#include "keyboard.h"
#include "types.h"
 
uint32_t inode_addr; //address of first inode
uint32_t  data_addr; //address of first data block

/*
 * file_init_mod
 *   DESCRIPTION: Initializes boot block and assigns global variables N and D
 *   INPUTS: modStart -- It is mod->mod_start which is taken from kernal.c to point to the boot block
 *   OUTPUTS: None
 *   RETURN VALUE: None
 *   SIDE EFFECTS: Initializes global variables of N,D, and the boot block itself
 */

  void file_init_mod(uint32_t modStart)
  {
  	//removed other variables because info in bootblock
 	bootBlock = (boot_block_t*)modStart;//address of bootblock
 	inode_addr = modStart+ FOURKB;      //address of first inode
	data_addr = modStart + ((bootBlock->inode_count)*FOURKB) + FOURKB; //address of first data block
 }
 

/*
 * file_write
 *   DESCRIPTION: Writes the file, not needed for MP
 *   INPUTS:   fd -- file descriptor
 *			   buf -- buffer to allocate read data to  
 *			   nbytes -- the size of the bytes that will be the length for read_data
 *   OUTPUTS: None
 *   RETURN VALUE: returns -1 (For now as read only filesystem)
 *   SIDE EFFECTS: None
 */
 int32_t file_write(int32_t fd, const void* buf, int32_t nbytes)
 {
	return -1;
 }
 

/*
 * file_open
 *   DESCRIPTION: open a file, does nothing but return as files opened by syscall open
 *   INPUTS: filename -- name of file to open
 *   OUTPUTS: None
 *   RETURN VALUE: returns 0
 *   SIDE EFFECTS: None
 */
 int32_t file_open(const uint8_t* filename)
 {
	return 0;
 }

 /*
 * file_read
 *   DESCRIPTION: Reads the file
 *   INPUTS:   fd -- file descriptor
 *			   buf -- buffer to allocate read data to  
 *			   nbytes -- the size of the bytes that will be the length for read_data
 *   OUTPUTS: None
 *   RETURN VALUE: returns number of bytes read-in, -1 on error
 *   SIDE EFFECTS: None
 */
int32_t file_read(int32_t fd, void* buf, int32_t nbytes)
{
	int32_t retval;
 	//error checking 
 	if(nbytes < 0 || buf == NULL || fd <FILE_START || fd >= MAX_FILE ){return -1;}
 	//Get current pcb to check fd
 	PCB_t* current = (PCB_t*)get_pcb_addr_C(terminal[current_term].process);
 	//Check if fd is present
 	if(current->file_des_array[fd].flags == PRESENT)
 	{
 		//use helper function to read data, return number of bytes read
 		retval = read_data(current->file_des_array[fd].inode,current->file_des_array[fd].file_position, buf, nbytes);
		current->file_des_array[fd].file_position += retval;
 		return retval;
 	}
 	//error occured
	return -1;
 }


/*
 * file_close
 *   DESCRIPTION: Closes the file
 *   INPUTS:   fd -- file descriptor of file to close
 *   OUTPUTS: None
 *   RETURN VALUE: returns 0 when it works
 *   SIDE EFFECTS: None
 */
 int32_t file_close(int32_t fd)
 {
	return 0;	 
 }
 

/*
 * directory_read
 *   DESCRIPTION: Reads the directory
 *   INPUTS:   fd -- file descriptor
 *			   buf -- buffer to allocate read data to  
 *			   nbytes -- the size of the bytes that will be the length for read_data
 *
 *   OUTPUTS: None
 *   RETURN VALUE: returns 0
 *   SIDE EFFECTS: None
 */

 int32_t directory_read(int32_t fd, void* buf, int32_t nbytes)
 {

 	if(fd < FILE_START || fd > MAX_FILE || buf == NULL || nbytes <= 0){return -1;}

 	dentry_t fdentry;
 	PCB_t* current = (PCB_t*)get_pcb_addr_C(terminal[current_term].process);
 
 	uint32_t position = current->file_des_array[fd].file_position;
 	if (position < 0){return -1;}
 	else if(position >bootBlock->dir_count){return 0;}

 	if(read_dentry_by_index( position, &fdentry ) == -1){return -1;}
 	
 	uint32_t limit = NAMELEN;
 	int32_t index = 0;
 	
 	if(nbytes < limit){limit = nbytes;}
 	//copy name over
 	while(index < limit)
 	{
 		((uint8_t*)buf)[index] = fdentry.filename[index];
 		if(((uint8_t*)buf)[index] == NOKE){break;}
 		index++;
 	}

 	current->file_des_array[fd].file_position++ ;	//increment index to point to next dentry

 	return index;
 }

 /*
 * directory_write
 *   DESCRIPTION: Writes the directory
 *   INPUTS:   fd -- file descriptor
 *			   buf -- buffer to allocate read data to  
 *			   nbytes -- the size of the bytes that will be the length for read_data
 *   OUTPUTS: None
 *   RETURN VALUE: returns -1 (For now)
 *   SIDE EFFECTS: None
 */
 int32_t directory_write(int32_t fd, const void* buf, int32_t nbytes)
 {
	return -1;
 }

/*
* directory_open
*   DESCRIPTION: Opens the directory
*   INPUTS:   filename -- name of the directory to open
*   OUTPUTS: None
*   RETURN VALUE: returns 0
*   SIDE EFFECTS: None
*/
int32_t directory_open(const uint8_t* filename)
{
	return 0;
}
 /*
 * file_close
 *   DESCRIPTION: Closes the directory
 *   INPUTS:   fd -- file descriptor
 *   OUTPUTS: None
 *   RETURN VALUE: returns 0
 *   SIDE EFFECTS: None
 */

 int32_t directory_close(int32_t fd)
 {
	return 0;
 }

 

/*
 * read_dentry_by_name
 *   DESCRIPTION: Reads the directory entry by name
 *   INPUTS:   fname -- file name of specific file name to read from
 *			   dentry_name -- directory entry pointer used for read_directory_by_name
 *   OUTPUTS: None
 *   RETURN VALUE: returns 0 or -1
 *   SIDE EFFECTS: Modifies global pointer to dentry struct
 */

 int32_t read_dentry_by_name (const uint8_t* fname, dentry_t* dentry_name)
 {
 	/* 
 	1) take in file name
 	2) search through dentries to find file name
 	3) return the inode #
 	*/

	//make sure args are valid
	if(fname ==NULL || dentry_name ==NULL){return -1;}
	
 	int32_t i;
 	int32_t name_len = strlen((int8_t*)fname);

 	//check for valid name length 
 	if(name_len > NAMELEN || name_len == 0){return -1;}
 	//search directories
 	for(i = 0; i < NUMDIR; i++)
	{
		if (strncmp((int8_t*)bootBlock->directories[i].filename, (int8_t*) fname, NAMELEN) == 0)
		{
			memcpy(dentry_name, &bootBlock->directories[i],sizeof(dentry_t));
			return 0;
		}
	}
	//error did not find in directories
	return -1;
 }


 /*
 * read_dentry_by_index
 *   DESCRIPTION: Reads the directory entry by index
 *   INPUTS:   index -- the directory index taken from read_dentry_by_name
 *			   dentry_index -- directory entry pointer used for read_directory_by_index
 *
 *   OUTPUTS: None
 *   RETURN VALUE: returns 0 or -1
 *   SIDE EFFECTS: Modifies global pointer to dentry struct
 */
int32_t read_dentry_by_index (uint32_t index, dentry_t* dentry_index)
{
	/*
	1) take in inode index in memory
	2) find specified inode
	3) return address where that inode is in memory
	*/
	if(index >= bootBlock->inode_count || dentry_index == NULL){return -1;}
	
	strcpy(dentry_index->filename, bootBlock->directories[index].filename );
	dentry_index->filetype = bootBlock->directories[index].filetype;
	dentry_index->inode_num = bootBlock->directories[index].inode_num;

	return 0;
}


/*
 * read_data
 *   DESCRIPTION: Reads data
 *   INPUTS:   inode -- the value of the index node number taken from the file
 *			   offset -- starting position in file in bytes
 *			   buf -- buffer to allocate read data to
 *			   length -- length of data requested
 *   OUTPUTS: None
 *   RETURN VALUE: returns NUMBER OF BYTES READ or -1 on error
 *   SIDE EFFECTS: Modifies memory pointed to by buffer
 */
int32_t read_data (uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length)
{	
	//check for valid inode number and array
	if(inode >= bootBlock->inode_count || buf == NULL){ return -1;}
	//Are we reading in anything
	if(length == 0){ return 0;}	
	//Get inode pointer using inode number given
	inode_t* node = (inode_t*)(inode_addr + FOURKB*inode);
	//starting past the end of the file
	if(offset > node->length){return -1;}
	//starting at the end of the file
	if(offset == node->length){return 0;}
	//check if file is too large 
	if((node->length)/FOURKB > DATB_SIZE){return -1;}
	//check if needing more bytes than used, read only to the end
	if(node->length < length+offset){ length = node->length - offset;}
	int32_t cur_block;    //keep track of blocks being accessed, one file may need more than 1
	int32_t readcount = 0;//keep track of bytes read in
	uint8_t* byte_address;
	
	//start reading in the data one byte at a time
	while(length > 0)
	{
		//determine if we need to go to the next data block
		cur_block = offset/FOURKB;
		//somehow went past the limits of the filesystem
		if(node->data_block_num[cur_block] > bootBlock->data_count){return -1;}
		//get pointer to data block needed
		byte_address = (uint8_t*)(data_addr + FOURKB*node->data_block_num[cur_block] + (offset%FOURKB));
		
		// update buffer
		*buf = *byte_address;//update byte
		length -= 1;	//decrement length by amount read in
		offset += 1;	//increment offset to point to current location in block
		readcount += 1;//increment number of bytes read in
		buf += 1;		//adjust pointer to new address in buffer to start next round of copying
	}								
	return readcount;
}


/*
 * file_length
 *   DESCRIPTION: gets the length of a file
 *   INPUTS:   inode -- the value of the index node number
 *   OUTPUTS: None
 *   RETURN VALUE: size of file
 *   SIDE EFFECTS: none
 */
uint32_t file_length(uint32_t inode)
{
	inode_t* ptr_inode = (inode_t*)(inode_addr + inode*FOURKB);	
	
	return ptr_inode->length;
}



