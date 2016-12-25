/*
 * Based on PRU_memAccess_DDR_PRUsharedRAM.c from the PRUSS reference
 * code provided by TI. The original code is copyrighted by TI, and the
 * original licensing information follows:
*/

/*
 * Copyright (C) 2012 Texas Instruments Incorporated - http://www.ti.com/
 *
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
*/

/*
 * ============================================================================
 * Copyright (c) Texas Instruments Inc 2010-12
 *
 * Use of this software is controlled by the terms and conditions found in the
 * license agreement under which this software has been supplied or provided.
 * ============================================================================
 */

#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "beagleboard.h"
#include "besPru.h"

#if defined(BEAGLEBONE_BLACK)

#include "pru/prussdrv.h"
#include "pru/pruss_intc_mapping.h"

#define PRU_NUM 	 	0
#define OFFSET_SHAREDRAM 	2048
#define PRUSS0_SHARED_DATARAM	4

static void *sharedMem;
static uint32_t *sharedMem_int;
static uint32_t pruEnabled = 0;

uint32_t BESPRUSetup(void)
{
    unsigned int ret;
    tpruss_intc_initdata pruss_intc_initdata = PRUSS_INTC_INITDATA;

    if (pruEnabled) return 0;
fprintf(stderr, "BESPRUSetup: About to init the PRUSS\n");
    /* Initialize the PRU */
    prussdrv_init ();

    /* Open PRU Interrupt */
    ret = prussdrv_open(PRU_EVTOUT_0);
    if (ret)
    {
        fprintf(stderr, "prussdrv_open open failed\n");
        return(pruEnabled);
    }

    /* Get the interrupt initialized */
    prussdrv_pruintc_init(&pruss_intc_initdata);

    /* Execute gamepad firmware on PRU */
    fprintf(stderr, "\tINFO: Executing example.\r\n");
    prussdrv_exec_program (PRU_NUM, "./gamepad.bin");

    /* Allocate Shared PRU memory. */
    //prussdrv_map_prumem(PRUSS0_SHARED_DATARAM, &sharedMem);
    //sharedMem_int = (uint32_t *) sharedMem;

    /* Done! */
    pruEnabled = 1;
fprintf(stderr, "PRU enabled!\n");
    return pruEnabled;
}

uint32_t BESPRUCheckState(void)
{
    if (!pruEnabled) return 0x0; //FFFFFFFF;

    /* Allocate Shared PRU memory. */
    prussdrv_map_prumem(PRUSS0_SHARED_DATARAM, &sharedMem);
    sharedMem_int = (uint32_t *) sharedMem;

    return(sharedMem_int[OFFSET_SHAREDRAM]);
}

void BESPRUShutdown(void)
{
    if (pruEnabled)
    {
        /* Disable PRU and close memory mapping*/
        prussdrv_pru_disable(PRU_NUM);
        prussdrv_exit ();
        pruEnabled = 0;
    }
}

#else /* defined(BEAGLEBONE_BLACK) */

uint32_t BESPRUSetup(void) { return(0); }
uint32_t BESPRUCheckState(void) { return(0xFFFFFFFF); }
void BESPRUShutdown(void) {}

#endif /* defined(BEAGLEBONE_BLACK) */
