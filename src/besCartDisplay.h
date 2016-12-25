#ifndef __BESCARTDISPLAY_H__
#define __BESCARTDISPLAY_H__
#include <string>

extern int BESCartDisplayMenu(void);
extern int BESCartDisplayGame(const enumPlatform_t platform, 
  const std::string &name);
extern int BESCartOpenDisplay(void);
extern int BESCartCloseDisplay(void);

#endif /* __BESCARTDISPLAY_H__ */

