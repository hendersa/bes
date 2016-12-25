/* Beagle Entertainment System cartridge label display code.

  Written by Andrew Henderson (hendersa@icculus.org).

  This drives a 320x240 ILI9340-based TFT display. It is derived 
  from the Arduino driver code provided by AdaFruit. Per Adafruit's
  MIT licensing, the following header carries their redistribution
  text: */
 
/***************************************************
  This is an Arduino Library for the Adafruit 2.2" SPI display.
  This library works with the Adafruit 2.2" TFT Breakout w/SD card
  ----> http://www.adafruit.com/products/1480
 
  Check out the links above for our tutorials and wiring diagrams
  These displays use SPI to communicate, 4 or 5 pins are required to
  interface (RST is optional)
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <math.h>
#include <SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_ttf.h>

#include "gui.h"
#include "besCartDisplay.h"
#include "besControls.h"

/* GPIO_DC:  gpio1[16] : P9.15
   GPIO_RST: gpio0[14] : P9.26 */
#define GPIO_DC  48
#define GPIO_RST 14

#define BUF_SIZE 32
#define MAX_PATH 40

#define ILI9340_TFTWIDTH  240
#define ILI9340_TFTHEIGHT 320

/* Logically, this is swapped due to screen rotation */
#define CART_TFT_WIDTH  ILI9340_TFTHEIGHT
#define CART_TFT_HEIGHT ILI9340_TFTWIDTH

#define ILI9340_NOP     0x00
#define ILI9340_SWRESET 0x01
#define ILI9340_RDDID   0x04
#define ILI9340_RDDST   0x09

#define ILI9340_SLPIN   0x10
#define ILI9340_SLPOUT  0x11
#define ILI9340_PTLON   0x12
#define ILI9340_NORON   0x13

#define ILI9340_RDMODE  0x0A
#define ILI9340_RDMADCTL  0x0B
#define ILI9340_RDPIXFMT  0x0C
#define ILI9340_RDIMGFMT  0x0A
#define ILI9340_RDSELFDIAG  0x0F

#define ILI9340_INVOFF  0x20
#define ILI9340_INVON   0x21
#define ILI9340_GAMMASET 0x26
#define ILI9340_DISPOFF 0x28
#define ILI9340_DISPON  0x29

#define ILI9340_CASET   0x2A
#define ILI9340_PASET   0x2B
#define ILI9340_RAMWR   0x2C
#define ILI9340_RAMRD   0x2E

#define ILI9340_PTLAR   0x30
#define ILI9340_MADCTL  0x36
#define ILI9340_MADCTL_MY  0x80
#define ILI9340_MADCTL_MX  0x40
#define ILI9340_MADCTL_MV  0x20
#define ILI9340_MADCTL_ML  0x10
#define ILI9340_MADCTL_RGB 0x00
#define ILI9340_MADCTL_BGR 0x08
#define ILI9340_MADCTL_MH  0x04
#define ILI9340_PIXFMT  0x3A
#define ILI9340_FRMCTR1 0xB1
#define ILI9340_FRMCTR2 0xB2
#define ILI9340_FRMCTR3 0xB3
#define ILI9340_INVCTR  0xB4
#define ILI9340_DFUNCTR 0xB6
#define ILI9340_PWCTR1  0xC0
#define ILI9340_PWCTR2  0xC1
#define ILI9340_PWCTR3  0xC2
#define ILI9340_PWCTR4  0xC3
#define ILI9340_PWCTR5  0xC4
#define ILI9340_VMCTR1  0xC5
#define ILI9340_VMCTR2  0xC7
#define ILI9340_RDID1   0xDA
#define ILI9340_RDID2   0xDB
#define ILI9340_RDID3   0xDC
#define ILI9340_RDID4   0xDD
#define ILI9340_GMCTRP1 0xE0
#define ILI9340_GMCTRN1 0xE1
/*
#define ILI9340_PWCTR6  0xFC

*/

// Color definitions
#define	ILI9340_BLACK   0x0000
#define	ILI9340_BLUE    0x001F
#define	ILI9340_RED     0xF800
#define	ILI9340_GREEN   0x07E0
#define ILI9340_CYAN    0x07FF
#define ILI9340_MAGENTA 0xF81F
#define ILI9340_YELLOW  0xFFE0  
#define ILI9340_WHITE   0xFFFF

static SDL_Surface *cartScreen = NULL;
static SDL_Surface *menuTextImage[2] = {NULL, NULL};
static SDL_Surface *gameTextImage[2] = {NULL, NULL};
static SDL_Surface *tinySplash = NULL;

/* Console icons are SNES, NES, GBA, and GBC, in that order */
static SDL_Surface *consoleIcons = NULL;
static /*const*/ SDL_Rect consoleIconRect[4] =
  {{0,0,64,64}, {128,0,64,64}, {64,0,64,64}, {192,0,64,64}};

static int SPIOpen = 0;
static int fdSPI = 0;
//static int speedHz = 16000000; /* 16 MHz */
static int speedHz = 48000000;
static int mode = 0;
static int bitsPerWord = 8;
static int delay = 0;
static uint8_t txBuffer[16], rxBuffer[16];
static uint8_t txBufBig[CART_TFT_WIDTH*48*2], rxBufBig[CART_TFT_WIDTH*48*2];

static int SPIDataTxRx(const int txSize, const uint8_t *_txBuffer, 
  uint8_t *_rxBuffer)
{

  int ret;
  struct spi_ioc_transfer trans;

  trans.tx_buf = (unsigned long)_txBuffer;
  trans.rx_buf = (unsigned long)_rxBuffer;
  trans.len = txSize;
  trans.delay_usecs = delay;
  trans.speed_hz = speedHz;
  trans.bits_per_word = bitsPerWord;

  ret = ioctl(fdSPI, SPI_IOC_MESSAGE(1), &trans);
  if (ret < 1)
  {
    fprintf(stderr, "ERROR: SPIDataTxRx() -> SPI transfer failed\n");
    return(1);
  }

  return(0);
}

static int writecommand(const uint8_t reg)
{
  int result = 0;
  txBuffer[0] = reg;
  //digitalWrite(_dcPin, 0);
  BESGPIOToggle(GPIO_DC, 0);
  result = SPIDataTxRx(1, txBuffer, rxBuffer);
  BESGPIOToggle(GPIO_DC, 1);
}

static int writedata(const uint8_t *data, const uint32_t length)
{
  //digitalWrite(_dcPin, 1);
  //BESGPIOToggle(GPIO_DC, 1);
  if (length <= 16)
    return SPIDataTxRx(length, data, rxBuffer);
  else
    return SPIDataTxRx(length, data, rxBufBig);
}

static int setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1,
 uint16_t y1) 
{
  writecommand(ILI9340_CASET); // Column addr set
  txBuffer[0] = (x0 >> 8); //writedata(x0 >> 8);
  txBuffer[1] = (x0 & 0xFF); //writedata(x0 & 0xFF);     // XSTART 
  txBuffer[2] = (x1 >> 8); //writedata(x1 >> 8);
  txBuffer[3] = (x1 & 0xFF); //writedata(x1 & 0xFF);     // XEND
  writedata(txBuffer, 4);

  writecommand(ILI9340_PASET); // Row addr set
  txBuffer[0] = (y0 >> 8); //writedata(y0>>8);
  txBuffer[1] = (y0); //writedata(y0);     // YSTART
  txBuffer[2] = (y1 >> 8); //writedata(y1>>8);
  txBuffer[3] = (y1); //writedata(y1);     // YEND
  writedata(txBuffer, 4);

  writecommand(ILI9340_RAMWR); // write to RAM
}

int BESCartOpenDisplay(void)
{
  int ret;
  uint8_t id;
  SDL_Surface *tempSurface;

  /* Is the SPI bus already open? */
  if (SPIOpen) return(SPIOpen);

  /* Create the cart screen */
  cartScreen = SDL_CreateRGBSurface(SDL_SWSURFACE, CART_TFT_WIDTH,
    CART_TFT_HEIGHT, 16, ILI9340_RED, ILI9340_GREEN, ILI9340_BLUE, 0);

  /* Load the cart screen graphics */
  tinySplash = IMG_Load("gfx/splash_320x240_bg.png");

  /* Render the menu screen text */
  menuTextImage[0] = TTF_RenderText_Blended(fontFSB25, "Selecting Game Title", textColor);
  menuTextImage[1] = TTF_RenderText_Blended(fontFSB25, "In Main Menu", textColor); 

  /* Load the console icons */
  consoleIcons = IMG_Load("gfx/big_icons.png");

  /* Render the game screen text */
  gameTextImage[0] = TTF_RenderText_Blended(fontFSB25, "Now Playing:", textColor);

  /* Open up the SPI bus */
  fdSPI = open("/dev/spidev1.0", O_RDWR);
  if (fdSPI == -1)
  {
    fprintf(stderr, "ERROR: openSPIInterface() -> Can't open /dev/spidev1.0\n");
    return(SPIOpen);
  }

  /* Set up the SPI mode */
  ret = ioctl(fdSPI, SPI_IOC_WR_MODE, &mode);
  if (ret == -1)
  {
    fprintf(stderr, "ERROR: openSPIInterface() -> Can't set SPI_IOC_WR_MODE\n");
    close(fdSPI);
    return(SPIOpen);
  }
  ret = ioctl(fdSPI, SPI_IOC_RD_MODE, &mode);
  if (ret == -1)
  {
    fprintf(stderr, "ERROR: openSPIInterface() -> Can't set SPI_IOC_RD_MODE\n");
    close(fdSPI);
    return(SPIOpen);
  }

  /* Set the bits per word */
  ret = ioctl(fdSPI, SPI_IOC_WR_BITS_PER_WORD, &bitsPerWord);
  if (ret == -1)
  {
    fprintf(stderr, "ERROR: openSPIInterface() -> Can't set SPI_IOC_WR_BITS_PER_WORD\n");
    close(fdSPI);
    return(SPIOpen);
  }
  ret = ioctl(fdSPI, SPI_IOC_RD_BITS_PER_WORD, &bitsPerWord);
  if (ret == -1)
  {
    fprintf(stderr, "ERROR: openSPIInterface() -> Can't set SPI_IOC_RD_BITS_PER_WORD\n");
    close(fdSPI);
    return(SPIOpen);
  }

  /* Set the max speed in hz */
  ret = ioctl(fdSPI, SPI_IOC_WR_MAX_SPEED_HZ, &speedHz);
  if (ret == -1)
  {
    fprintf(stderr, "ERROR: openSPIInterface() -> Can't set SPI_IOC_WR_MAX_SPEED_HZ\n");
    close(fdSPI);
    return(SPIOpen);
  }

  ret = ioctl(fdSPI, SPI_IOC_RD_MAX_SPEED_HZ, &speedHz);
  if (ret == -1)
  {
    fprintf(stderr, "ERROR: openSPIInterface() -> Can't set SPI_IOC_RD_MAX_SPEED_HZ\n");
    close(fdSPI);
    return(SPIOpen);
  }

  /* Success! */
  SPIOpen = 1;

  /* Initialize the display */
  //digitalWrite(_rst, 1);
  BESGPIOToggle(GPIO_RST, 1);
  usleep(5 * 1000); //delay(5);
  //digitalWrite(_rst, 0);
  BESGPIOToggle(GPIO_RST, 0);
  usleep(20 * 1000); //delay(20);
  //digitalWrite(_rst, 1);
  BESGPIOToggle(GPIO_RST, 1);
  usleep(150 * 1000); //delay(150);

  writecommand(0xEF);
  txBuffer[0] = 0x03; //writedata(0x03);
  txBuffer[1] = 0x80; //writedata(0x80);
  txBuffer[2] = 0x02; //writedata(0x02);
  writedata(txBuffer, 3);

  writecommand(0xCF);  
  txBuffer[0] = 0x00; //writedata(0x00); 
  txBuffer[1] = 0xC1; //writedata(0XC1); 
  txBuffer[2] = 0x30; //writedata(0X30); 
  writedata(txBuffer, 3);

  writecommand(0xED);  
  txBuffer[0] = 0x64; //writedata(0x64); 
  txBuffer[1] = 0x03; //writedata(0x03); 
  txBuffer[2] = 0x12; //writedata(0X12); 
  txBuffer[3] = 0x81; //writedata(0X81); 
  writedata(txBuffer, 4);

  writecommand(0xE8);  
  txBuffer[0] = 0x85; //writedata(0x85); 
  txBuffer[1] = 0x00; //writedata(0x00); 
  txBuffer[2] = 0x78; //writedata(0x78); 
  writedata(txBuffer, 3);

  writecommand(0xCB);  
  txBuffer[0] = 0x39; //writedata(0x39); 
  txBuffer[1] = 0x2C; //writedata(0x2C); 
  txBuffer[2] = 0x00; //writedata(0x00); 
  txBuffer[3] = 0x34; //writedata(0x34); 
  txBuffer[4] = 0x02; //writedata(0x02); 
  writedata(txBuffer, 5);

  writecommand(0xF7);  
  txBuffer[0] = 0x20; //writedata(0x20); 
  writedata(txBuffer, 1);

  writecommand(0xEA);  
  txBuffer[0] = 0x00; //writedata(0x00); 
  txBuffer[1] = 0x00; //writedata(0x00); 
  writedata(txBuffer, 2);

  writecommand(ILI9340_PWCTR1);    //Power control 
  txBuffer[0] = 0x23; //writedata(0x23);   //VRH[5:0] 
  writedata(txBuffer, 1);

  writecommand(ILI9340_PWCTR2);    //Power control 
  txBuffer[0] = 0x10; //writedata(0x10);   //SAP[2:0];BT[3:0] 
  writedata(txBuffer, 1);
 
  writecommand(ILI9340_VMCTR1);    //VCM control 
  txBuffer[0] = 0x3E; //writedata(0x3e); //
  txBuffer[1] = 0x28; //writedata(0x28); 
  writedata(txBuffer, 2);

  writecommand(ILI9340_VMCTR2);    //VCM control2 
  txBuffer[0] = 0x86; //writedata(0x86);  //--
  writedata(txBuffer, 1);

  writecommand(ILI9340_MADCTL);    // Memory Access Control 
  //writedata(ILI9340_MADCTL_MX | ILI9340_MADCTL_BGR);
  /* AWH - Original was for 240x320, we want 320x240 rotation */
  txBuffer[0] = (ILI9340_MADCTL_MV | ILI9340_MADCTL_MX |
    ILI9340_MADCTL_MY | ILI9340_MADCTL_BGR); 
  writedata(txBuffer, 1);

  writecommand(ILI9340_PIXFMT);    
  txBuffer[0] = 0x55; //writedata(0x55); 
  writedata(txBuffer, 1);

  writecommand(ILI9340_FRMCTR1);    
  txBuffer[0] = 0x00; //writedata(0x00);  
  txBuffer[1] = 0x18; //writedata(0x18); 
  writedata(txBuffer, 2);

  writecommand(ILI9340_DFUNCTR);    // Display Function Control 
  txBuffer[0] = 0x08; //writedata(0x08); 
  txBuffer[1] = 0x82; //writedata(0x82);
  txBuffer[2] = 0x27; //writedata(0x27);  
  writedata(txBuffer, 3);

  writecommand(0xF2);    // 3Gamma Function Disable 
  txBuffer[0] = 0x00; //writedata(0x00); 
  writedata(txBuffer, 1);

  writecommand(ILI9340_GAMMASET);    //Gamma curve selected 
  txBuffer[0] = 0x01; //writedata(0x01); 
  writedata(txBuffer, 1);

  writecommand(ILI9340_GMCTRP1);    //Set Gamma 
  txBuffer[0] = 0x0F; //writedata(0x0F); 
  txBuffer[1] = 0x31; //writedata(0x31); 
  txBuffer[2] = 0x2B; //writedata(0x2B); 
  txBuffer[3] = 0x0C; //writedata(0x0C); 
  txBuffer[4] = 0x0E; //writedata(0x0E); 
  txBuffer[5] = 0x08; //writedata(0x08); 
  txBuffer[6] = 0x4E; //writedata(0x4E); 
  txBuffer[7] = 0xF1; //writedata(0xF1); 
  txBuffer[8] = 0x37; //writedata(0x37); 
  txBuffer[9] = 0x07; //writedata(0x07); 
  txBuffer[10] = 0x10; //writedata(0x10); 
  txBuffer[11] = 0x03; //writedata(0x03); 
  txBuffer[12] = 0x0E; //writedata(0x0E); 
  txBuffer[13] = 0x09; //writedata(0x09); 
  txBuffer[14] = 0x00; //writedata(0x00); 
  writedata(txBuffer, 15);

  writecommand(ILI9340_GMCTRN1);    //Set Gamma 
  txBuffer[0] = 0x00; //writedata(0x00); 
  txBuffer[1] = 0x0E; //writedata(0x0E); 
  txBuffer[2] = 0x14; //writedata(0x14); 
  txBuffer[3] = 0x03; //writedata(0x03); 
  txBuffer[4] = 0x11; //writedata(0x11); 
  txBuffer[5] = 0x07; //writedata(0x07); 
  txBuffer[6] = 0x31; //writedata(0x31); 
  txBuffer[7] = 0xC1; //writedata(0xC1); 
  txBuffer[8] = 0x48; //writedata(0x48); 
  txBuffer[9] = 0x08; //writedata(0x08); 
  txBuffer[10] = 0x0F; //writedata(0x0F); 
  txBuffer[11] = 0x0C; //writedata(0x0C); 
  txBuffer[12] = 0x31; //writedata(0x31); 
  txBuffer[13] = 0x36; //writedata(0x36); 
  txBuffer[14] = 0x0F; //writedata(0x0F); 
  writedata(txBuffer, 15);

  writecommand(ILI9340_SLPOUT);    //Exit Sleep 
  usleep(120 * 1000); //delay(120); 		
  writecommand(ILI9340_DISPON);    //Display on 

  return 0;
}

int BESCartCloseDisplay(void)
{
  /* Is SPI currently open? */
  if (!SPIOpen) return 1;

  /* Close SPI */
  close(fdSPI);

  /* Done! */
  SPIOpen = 0;
  return 0;
}

static int BESCartDrawScreen(void) {
  int y = 0;
  uint8_t buffer[320*240*2];

  fprintf(stderr, "DATA: Framebuffer data\n");
  fprintf(stderr, "cartScreen BPP: %d\n", cartScreen->format->BitsPerPixel);
  setAddrWindow(0, 0, (CART_TFT_WIDTH - 1), (CART_TFT_HEIGHT - 1));
  for (y=0; y < (320*240); y++)
  {
    buffer[y * 2] = *(uint8_t *)((cartScreen->pixels) + (y*2)+1);
    buffer[(y*2)+1] = *(uint8_t *)((cartScreen->pixels) + (y*2));
 
  }

  for (y=0; y < CART_TFT_HEIGHT; y += 48)
  {
    //setAddrWindow(0, y, 319, y+48);
    writedata(((const uint8_t *)(/*cartScreen->pixels*/buffer)) + (y * CART_TFT_WIDTH* 2), CART_TFT_WIDTH * 2 * 48);
  }
}

int BESCartDisplayMenu(void)
{
  SDL_Rect srcRect, dstRect = {0,0,0,0};

  /* Is the SPI bus open? */
  if (!SPIOpen) return(1);

  /* Clear the cart background */
  SDL_FillRect(cartScreen, NULL, 0x0);

  /* Draw the logo towards the top */
  srcRect.x = 0; srcRect.y = 60;
  srcRect.w = 320; srcRect.h = 180;
  SDL_BlitSurface(tinySplash, &srcRect, cartScreen, &dstRect);

  /* Draw menu text */
  dstRect.x = 40; dstRect.y = 120; 
  SDL_BlitSurface(menuTextImage[0], NULL, cartScreen, &dstRect);
  dstRect.x = 80; dstRect.y = 150;
  SDL_BlitSurface(menuTextImage[1], NULL, cartScreen, &dstRect);
 
  /* Render it to the TFT */
  BESCartDrawScreen();

  return(0); 
}

int BESCartDisplayGame(const enumPlatform_t platform, const std::string &name)
{
  SDL_Rect srcRect, dstRect = {0,0,0,0};
  SDL_Surface *gameName = NULL;

  /* Is the SPI bus open? */
  if (!SPIOpen) return(1);

  /* Clear the cart background */
  SDL_FillRect(cartScreen, NULL, 0x0);

  /* Draw the logo towards the top */
  srcRect.x = 0; srcRect.y = 60;
  srcRect.w = 320; srcRect.h = 180;
  SDL_BlitSurface(tinySplash, &srcRect, cartScreen, &dstRect);

  /* Draw the console icon */
  dstRect.x = 15; dstRect.y = 130;
  SDL_BlitSurface(consoleIcons, &consoleIconRect[platform], cartScreen, &dstRect);

  /* Draw the "Now Playing:" text */
  dstRect.x = 80; dstRect.y = 90;
  SDL_BlitSurface(gameTextImage[0], NULL, cartScreen, &dstRect);

  /* Render and draw the game name */
  dstRect.x = 95; dstRect.y = 150;
  gameName = TTF_RenderText_Blended(fontFSB16, name.c_str(), textColor); 
  SDL_BlitSurface(gameName, NULL, cartScreen, &dstRect);

  /* Render it to the TFT */
  BESCartDrawScreen();

  return(0);
}
 

