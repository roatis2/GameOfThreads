/* file_system.h - Defines used in interactions with the file system
 * 
 */
#ifndef _FILESYSTEM_H
#define _FILESYSTEM_H


#include "types.h"
#include "syscall.h"

#define FOURKB     4096  //same as 0x1000
#define NUMDIR       63
#define NAMELEN      32
#define BOOT_RES     52
#define DENTRY_RES   24
#define DATB_SIZE  1023

/* Struct for inode */
typedef struct inode {

        int32_t  length;
        int32_t  data_block_num[DATB_SIZE];

}inode_t;

/* Struct for directory entry */
typedef struct dentry {
        int8_t   filename[NAMELEN];
        int32_t  filetype;
        int32_t  inode_num;
        int8_t   reserved[DENTRY_RES];   
}dentry_t;

/* Struct for the initial boot block */
typedef struct boot_block {

        int32_t  dir_count;
        int32_t  inode_count;
        int32_t  data_count;
        int8_t   reserved[BOOT_RES];
        dentry_t directories[NUMDIR];   

}boot_block_t;


/* Global variables for boot block, directory entry, and current inode */
boot_block_t* bootBlock;
dentry_t  dentry;
inode_t * cur_inode;
uint32_t Dat; //holds number of data block
uint32_t file_pos;



/* Initialize global variables in c */
extern void file_init_mod(uint32_t modStart);
/* Read the directive entry by name */
extern int32_t read_dentry_by_name (const uint8_t* fname, dentry_t* dentry_name);
/* Read the directive entry by index */
extern int32_t read_dentry_by_index (uint32_t index, dentry_t* dentry_index);
/* Read the data block */
extern int32_t read_data (uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length);


/* These are read,write,open,close operations for file */
int32_t file_read(int32_t fd, void* buf, int32_t nbytes);
int32_t file_write(int32_t fd, const void* buf, int32_t nbytes);
int32_t file_open(const uint8_t* filename);
int32_t file_close(int32_t fd);

/* These are read,write,open,close operations for directory */
int32_t directory_read(int32_t fd, void* buf, int32_t nbytes);
int32_t directory_write(int32_t fd, const void* buf, int32_t nbytes);
int32_t directory_open(const uint8_t* filename);
int32_t directory_close(int32_t fd);

//get file length
uint32_t file_length(uint32_t inode);


#endif

