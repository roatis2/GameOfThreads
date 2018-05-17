#include "tests.h"
#include "x86_desc.h"
#include "lib.h"
#include "rtc.h"
#include "keyboard.h"
#include "filesystem.h"
#include "syscall.h"

#define PASS 1
#define FAIL 0

/* format these macros as you see fit */
#define TEST_HEADER 	\
	printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)
#define TEST_OUTPUT(name, result)	\
	printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL");

static inline void assertion_failure(){
	/* Use exception #15 for assertions, otherwise
	   reserved by Intel */
	asm volatile("int $15");
}


/* Checkpoint 1 tests */

/* IDT Test - Example
 * 
 * Asserts that first 10 IDT entries are not NULL
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
 */
int idt_test(){
	TEST_HEADER;

	int i;
	int result = PASS;
	for (i = 0; i < 10; ++i){
		if ((idt[i].offset_15_00 == NULL) && 
			(idt[i].offset_31_16 == NULL)){
			assertion_failure();
			result = FAIL;
		}
	}

	return result;
}

/* paging_test2
 * 
 * Asserts that accessing memory outside allowable range page faults
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: Terminal stops running tests
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
 */
int paging_test2(){
	 TEST_HEADER;
	// Below the range of 4MB and 8MB
	int *derefPointer = NULL;
	int assignTo;
	
	assignTo = *derefPointer;
	return 0;
}
/* paging_test1 
 * 
 * Asserts that accessing memory in range is doable
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: Prints data from memory
 * Coverage: Load IDT, IDT definition
 */
int paging_test1(){
	 TEST_HEADER;
	// Within range of 4MB and 8MB ( only test that should work)
	int *derefPointer;
	int i = 0xB8000;
	derefPointer = &i;
	int assignTo;
	assignTo = *derefPointer;
	printf("Dereferenced video memory:  %x \n", assignTo);
	return PASS;
}
	
	
/* rtc_test
 * 
 * allows rtc to run handler
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: flashes screen for rtc
 * Coverage: 
 */
int rtc_test(){
	 TEST_HEADER;
	// Within range of 4MB and 8MB ( only test that should work)
	//uint32_t i;
	//while(i < 200){i++;}
	
	enable_rtc();
	
	//while(i > 0)
	//{ 
		//i--;
		//disable_irq(8);
	//}
	printf("RTC works \n");
	return PASS;
}

/* dividezero_test
 * 
 * test exception
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: flashes screen for rtc
 * Coverage: 
 */
int divide_test(){
	TEST_HEADER;

	int i, j, k;
	int result = PASS;
	i = 15;
	j = 0;
	k = i/j;
	return result;
	
}
// add more tests here

/* Checkpoint 2 tests */


/* write_test
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: change rtc frequency
 * Coverage: non power of 2, beyond limit, and valid rate
 */
int write_test()
{
	TEST_HEADER;
	int result = PASS;
	uint32_t frequency[1] = {3};

	int test = write_rtc(0,frequency,4); //non power of 2
	if(test == -1){printf("RTC write failed non power of 2 \n");}

	frequency[0] = 4000;
	test = write_rtc(0,frequency,4); //beyond limit
	if(test == -1){printf("RTC write failed rate out of range \n");}

	frequency[0] = 16;
	write_rtc(0,frequency,4);	//it will change the rtc frq to 16Hrz
	return result;
}


/* read_rtc_test
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: print for interrupt
 * Coverage: interrupt at specified rate
 */
int read_rtc_test()
{
	TEST_HEADER;
	int result = PASS;
	int temp;
	int cnt =0;
	uint32_t buf[1];

	while(cnt<20){					// will only print 16 rtc interupts then test is passed
		temp = read_rtc(0,buf,0);
		if (temp == 0)
			printf("rtc interupt occured");
		cnt++;
	}
	return result;
}

/* term_read_test
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: none
 * Coverage: can read keyboard buffer and return number of bytes read
 */
int term_read_test()
{
	TEST_HEADER;
	int result = PASS;
	int temp;
	int tests = 0;
	uint8_t buffer[128];

	while(tests < 5)
	{
		temp = terminal_read(0,buffer,120);
		if(temp >0)
		{
			printf("read in %d bytes \n", temp);
			tests++;
		}
		else
		{
			printf("read in 0 bytes\n");
		}
	}
	return result;
}


/* terminal_write_test
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: none
 * Coverage: read buffer and write to terminal, valid size passed to fuunction
 */
int terminal_write_test()
{
	TEST_HEADER;
	int result = PASS;
	int32_t temp = 0; 

	printf("print entire buffer size 40\n");
	uint8_t buffer[41] = {'0',' ',' ',' ','0',' ','0', '\n',
						  '0',' ',' ',' ','0',' ',' ', '\n', 
						  '0',' ','0',' ','0',' ','0', '\n',
						  '0',' ',' ',' ','0',' ','0', '\n',
					      '0',' ',' ',' ','0',' ','0', '\n',
					  	  '\0'};

	temp = terminal_write(0,buffer, 40); //all 40 characters
	if(temp == -1){printf("failed to print\n");}

	printf("try printing beyond buffer\n");
	temp = terminal_write(0,buffer, 60); //try to print beyond 40 
	if(temp == -1){printf("invalid nbytes size\n");}	//fail to print

	return result;
}

/* read_directory_test
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: none
 * Coverage: check if directory can be accessed
 */
int read_directory_test(){
	TEST_HEADER;
	int result = PASS;
	if(directory_open((uint8_t*)".") == 0){
		printf("\nFound a match \n");
	}
	else{
		printf("\nNot a match \n");
		result = FAIL;
	}
	return result;
}

/* open_directory_test
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: none
 * Coverage: Prove that all file names have been found without hard coding
 */
 /*
int open_directory_test(){
	TEST_HEADER;
	dir_read();
	return PASS;
}
*/
/* read_open_test
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: none
 * Coverage: Open a file
 */
int read_open_test(){
	TEST_HEADER;
	int result = PASS;

	uint8_t buf[201];
	int32_t print;
	uint8_t* filename;
	filename = (uint8_t*)"frame0.txt";
	printf("\nPreparing to enter file open\n");

	file_open(filename);
	printf("\nFile open for frame0 is successful\n");

	read_dentry_by_name(filename, &dentry);
	printf("\nwas able to read dentry by name\n");

	read_dentry_by_index(15, &dentry);
	printf("\nwas able to read dentry by index\n");

	printf("\nEntering read_data\n");
	print = read_data(dentry.inode_num,0,buf,200);
	if(print==-1)
	{
		printf("failed to read data\n");
	}

	printf("\nPrinted frame0.txt\n");

	int32_t i = 0;
	for(i = 0; i<print; i++)
		printf("%c", buf[i]);
	printf("\n");
	return result;
}


/* Checkpoint 3 tests */

int test_execute(){
	TEST_HEADER;
	printf("\ntesting execute \n");
	exec_test_shell();
	printf("\n made it here 4\n");
	return PASS;
}


/* Checkpoint 4 tests */
/* Checkpoint 5 tests */


/* Test suite entry point */
void launch_tests()
{
	//TEST_OUTPUT("idt_test", idt_test());
	// launch your tests here
	//TEST_OUTPUT("divide_test", divide_test());
	
	//Test accessing paging
	//TEST_OUTPUT("Paging_test_1", paging_test1());
	
	//Test page faulting
	//TEST_OUTPUT("Paging_test_2", paging_test2());
	
	//test RTC
	//TEST_OUTPUT("testing_RTC", rtc_test());

	//MP3.2 test block 
	//clear();
	
	//tests read rtc
	//TEST_OUTPUT("read_rtc_test", read_rtc_test());
	
	//test rtc write
	//TEST_OUTPUT("write_test", write_test());
	
	//tests read rtc again at new rate
	//TEST_OUTPUT("read_rtc_test", read_rtc_test());
	
	//TEST_OUTPUT("terminal_read_test", term_read_test());
	
	//TEST_OUTPUT("terminal_write_test", terminal_write_test());
	
	//TEST_OUTPUT("read_open_test", read_open_test());
	
	//TEST_OUTPUT("open_directory_test", open_directory_test());
	
	//TEST_OUTPUT("test_execute", test_execute());
	
	//TEST_OUTPUT("read_directory_test", read_directory_test());
	

}
