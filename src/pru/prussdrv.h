/*
 * prussdrv.h
 *
 * Describes PRUSS userspace driver for Industrial Communications
 *
 * Copyright (C) 2010 Texas Instruments Incorporated - http://www.ti.com/
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
 * Copyright (c) Texas Instruments Inc 2010-11
 *
 * Use of this software is controlled by the terms and conditions found in the
 * license agreement under which this software has been supplied or provided.
 * ============================================================================
*/

#ifndef _PRUSSDRV_H
#define _PRUSSDRV_H

#include <stdint.h>
#include <sys/types.h>

#if defined (__cplusplus)
extern "C" {
#endif

#define NUM_PRU_HOSTIRQS        8
#define NUM_PRU_HOSTS          10
#define NUM_PRU_CHANNELS       10
#define NUM_PRU_SYS_EVTS       64

#define PRUSS0_PRU0_DATARAM     0
#define PRUSS0_PRU1_DATARAM     1
#define PRUSS0_PRU0_IRAM        2
#define PRUSS0_PRU1_IRAM        3

#define PRUSS_V1                1 // AM18XX
#define PRUSS_V2                2 // AM33XX

//Available in AM33xx series - begin
#define PRUSS0_SHARED_DATARAM   4
#define	PRUSS0_CFG              5
#define	PRUSS0_UART             6
#define	PRUSS0_IEP              7
#define	PRUSS0_ECAP             8
#define	PRUSS0_MII_RT           9
#define	PRUSS0_MDIO            10
//Available in AM33xx series - end

#define PRU_EVTOUT_0            0
#define PRU_EVTOUT_1            1
#define PRU_EVTOUT_2            2
#define PRU_EVTOUT_3            3
#define PRU_EVTOUT_4            4
#define PRU_EVTOUT_5            5
#define PRU_EVTOUT_6            6
#define PRU_EVTOUT_7            7

    typedef struct __sysevt_to_channel_map {
        int16_t sysevt;
        int16_t channel;
    } tsysevt_to_channel_map;
    typedef struct __channel_to_host_map {
        int16_t channel;
        int16_t host;
    } tchannel_to_host_map;
    typedef struct __pruss_intc_initdata {
        //Enabled SYSEVTs - Range:0..63
        //{-1} indicates end of list
        char sysevts_enabled[NUM_PRU_SYS_EVTS];
        //SysEvt to Channel map. SYSEVTs - Range:0..63 Channels -Range: 0..9
        //{-1, -1} indicates end of list
        tsysevt_to_channel_map sysevt_to_channel_map[NUM_PRU_SYS_EVTS];
        //Channel to Host map.Channels -Range: 0..9  HOSTs - Range:0..9
        //{-1, -1} indicates end of list
        tchannel_to_host_map channel_to_host_map[NUM_PRU_CHANNELS];
        //10-bit mask - Enable Host0-Host9 {Host0/1:PRU0/1, Host2..9 : PRUEVT_OUT0..7}
        uint32_t host_enable_bitmask;
    } tpruss_intc_initdata;

    int prussdrv_init(void);

    int prussdrv_open(uint32_t host_interrupt);

    /** Return version of PRU.  This must be called after prussdrv_open. */
    int prussdrv_version();

    /** Return string description of PRU version. */
    const char* prussdrv_strversion(int version);

    int prussdrv_pru_reset(uint32_t prunum);

    int prussdrv_pru_disable(uint32_t prunum);

    int prussdrv_pru_enable(uint32_t prunum);
    int prussdrv_pru_enable_at(uint32_t prunum, size_t addr);

    int prussdrv_pru_write_memory(uint32_t pru_ram_id,
                                  uint32_t wordoffset,
                                  const uint32_t *memarea,
                                  uint32_t bytelength);

    int prussdrv_pruintc_init(const tpruss_intc_initdata *prussintc_init_data);

    /** Find and return the channel a specified event is mapped to.
     * Note that this only searches for the first channel mapped and will not
     * detect error cases where an event is mapped erroneously to multiple
     * channels.
     * @return channel-number to which a system event is mapped.
     * @return -1 for no mapping found
     */
    int16_t prussdrv_get_event_to_channel_map( uint32_t eventnum );

    /** Find and return the host interrupt line a specified channel is mapped
     * to.  Note that this only searches for the first host interrupt line
     * mapped and will not detect error cases where a channel is mapped
     * erroneously to multiple host interrupt lines.
     * @return host-interrupt-line to which a channel is mapped.
     * @return -1 for no mapping found
     */
    int16_t prussdrv_get_channel_to_host_map( uint32_t channel );

    /** Find and return the host interrupt line a specified event is mapped
     * to.  This first finds the intermediate channel and then the host.
     * @return host-interrupt-line to which a system event is mapped.
     * @return -1 for no mapping found
     */
    int16_t prussdrv_get_event_to_host_map( uint32_t eventnum );

    int prussdrv_map_l3mem(void **address);

    int prussdrv_map_extmem(void **address);

    uint32_t prussdrv_extmem_size(void);

    int prussdrv_map_prumem(uint32_t pru_ram_id, void **address);

    int prussdrv_map_peripheral_io(uint32_t per_id, void **address);

    uint32_t prussdrv_get_phys_addr(const void *address);

    void *prussdrv_get_virt_addr(uint32_t phyaddr);

    /** Wait for the specified host interrupt.
     * @return the number of times the event has happened. */
    uint32_t prussdrv_pru_wait_event(uint32_t host_interrupt);

    int prussdrv_pru_event_fd(uint32_t host_interrupt);

    int prussdrv_pru_send_event(uint32_t eventnum);

    /** Clear the specified event and re-enable the host interrupt. */
    int prussdrv_pru_clear_event(uint32_t host_interrupt,
                                 uint32_t sysevent);

    int prussdrv_pru_send_wait_clear_event(uint32_t send_eventnum,
                                           uint32_t host_interrupt,
                                           uint32_t ack_eventnum);

    int prussdrv_exit(void);

    int prussdrv_exec_program(int32_t prunum, const char *filename);
    int prussdrv_exec_program_at(int32_t prunum, const char *filename, size_t addr);

    int prussdrv_exec_code(int32_t prunum, const uint32_t *code, int32_t codelen);
    int prussdrv_exec_code_at(int32_t prunum, const uint32_t *code, int32_t codelen, size_t addr);
    int prussdrv_load_data(int32_t prunum, const uint32_t *code, int32_t codelen);
    int prussdrv_load_datafile(int32_t prunum, const char *filename);

#if defined (__cplusplus)
}
#endif
#endif

