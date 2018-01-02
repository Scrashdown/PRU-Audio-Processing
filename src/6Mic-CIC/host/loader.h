/**
 * @brief Headers for functions required to load, start and stop the PRU firmware.
 * 
 * @author Loïc Droz
 */


#include <stddef.h>
#include <pruss/prussdrv.h>
#include <pruss/pruss_intc_mapping.h>


/**
 * @brief Initializes the prussdrv driver, the PRUSS interrupt controller, and the memory maps needed for the program to work.
 * 
 * @param HOST_PRU_buf A pointer which the function will point to the buffer to which the PRU writes the audio samples.
 * @param HOST_PRU_buf_len A pointer to the 
 * @return int 
 */
int PRU_proc_init(volatile void ** HOST_PRU_buf, unsigned int * HOST_PRU_buf_len);

/**
 * @brief 
 * 
 * @param void 
 * @return int 
 */
int load_program(void);

/**
 * 
 * 
 */
void stop_program(void);