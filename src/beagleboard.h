#ifndef __BEAGLEBOARD_H__
#define __BEAGLEBOARD_H__

/* Are we building for an ARM board or x86? */
#if defined(__i386__) || defined(__x86_64__)
#define PC_PLATFORM 1 
#else
#define BEAGLEBONE_BLACK 1 
#endif

#endif /* __BEAGLEBOARD_H__ */

