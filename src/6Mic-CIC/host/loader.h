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
 *        Must be called before any other function in this file.
 * 
 * @param HOST_PRU_buf A pointer which the function will point to the buffer to which the PRU writes the audio samples.
 * @param HOST_PRU_buf_len A pointer to the length of the buffer allocated by the function.
 * @return int 0 in case of success, non-zero otherwise.
 */
int PRU_proc_init(volatile void ** HOST_PRU_buf, unsigned int * HOST_PRU_buf_len);

/**
 * @brief Loads and starts the PRU firmware.
 * 
 * @param void 
 * @return int 0 in case of success, non-zero otherwise.
 */
int load_program(void);

/**
 * @brief Stops the PRU firware and the PRUSS driver.
 * 
 * @param void
 */
void stop_program(void);