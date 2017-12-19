#include <stddef.h>


/**
 * 
 * 
 */
int PRU_proc_init(volatile void ** HOST_PRU_buf, unsigned int HOST_PRU_buf_len);

/**
 * 
 * 
 */
int load_program(void);

/**
 * 
 * 
 */
void stop_program(void);