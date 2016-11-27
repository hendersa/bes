#ifndef __BESPRU_H__
#define __BESPRU_H__
#include <stdint.h>

uint32_t pruSetup(void);
uint32_t checkPRUEvents(void);
void pruShutdown(void);

#endif /* __BESPRU_H__ */

