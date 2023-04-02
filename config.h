#ifndef _CONFIG_H_
#define _CONFIG_H_

/* #defines ================================================================= */

#define TRUE 1
#define FALSE 0
#define FAILED -1

/*  Separator for parsing input files.
 */
#define SEPARATOR " "

/*  Configure debug mode. Lots of useful information will be printed to stdout.
 */
#define DEBUG 1

/*  In MB. The maximum amount of memory in the simulated system. The allowed
    max is 2^16 = 65536 because we use uint16_t for pcb_t.memory_size,
    block.location, and block.size.
*/
#define MAX_MEMORY 2048

#endif
/* =============================================================================
   Written by David Sha.
============================================================================= */