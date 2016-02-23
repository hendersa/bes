#include <stdio.h>
#include <stdlib.h>
#include <GLES/gl.h>
#include <GLES/glext.h>
#include <SDL.h>
#include "gui.h"

#define LEFT -255
#define RIGHT 255
#define TOP -255
#define BOTTOM 255

extern SDL_Surface *fbscreen;

static GLuint textures[TEXTURE_LAST]={0,0,0,0,0};
/* C, D, A, A, D, B */
static GLfloat mainTexCoords[18];
static GLfloat mainTexPosCoords[12] = {
  -1.0f, -1.0f, 
   1.0f, -1.0f, 
  -1.0f,  1.0f, 
  -1.0f,  1.0f, 
   1.0f, -1.0f,
   1.0f,  1.0f }; 
static GLfloat pauseTexPosCoords[12] = {
  -1.0f, -1.0f,
   1.0f, -1.0f,
  -1.0f,  1.0f,
  -1.0f,  1.0f,
   1.0f, -1.0f,
   1.0f,  1.0f };

int texToUse = TEXTURE_256;
static float srcWidth = 1.0f;
static float srcHeight = 1.0f;
static float destScreenWidth = 1.0f;
static float destScreenHeight = 1.0f;
static float destAspectRatio = 1.0f;
static float srcAspectRatio = 1.0f;
static float scaleWidth = 1.0f;
static float scaleHeight = 1.0f;
static float translateX = -1.0f;
static float translateY = -1.0f;
float screenshotWidth = 1.0f;
float screenshotHeight = 1.0f;

static void setupMainTexCoords(void)
{
  /* C */
  mainTexCoords[0] = -1.0f;
  mainTexCoords[1] = 1.0f;
  mainTexCoords[2] = 0.0f;

  /* D */
  mainTexCoords[3] = 1.0f;
  mainTexCoords[4] = 1.0f;
  mainTexCoords[5] = 0.0f;

  /* A */
  mainTexCoords[6] = -1.0f;
  mainTexCoords[7] = -1.0f;
  mainTexCoords[8] = 0.0f;

  /* A */
  mainTexCoords[9] = -1.0f;
  mainTexCoords[10] = -1.0f;
  mainTexCoords[11] = 0.0f;

  /* D */
  mainTexCoords[12] = 1.0f;
  mainTexCoords[13] = 1.0f;
  mainTexCoords[14] = 0.0f;

  /* B */
  mainTexCoords[15] = 1.0f;
  mainTexCoords[16] = -1.0f; 
  mainTexCoords[17] = 0.0f;
}

static void setupMainTexRefCoords(void)
{
  float left, right, top, bottom;
  int texSize = 256 << texToUse;
  left = -1.0f * (srcWidth / texSize);
  right = 1.0f * (srcWidth / texSize);
  top = 1.0f * (srcHeight / texSize);
  bottom = -1.0f * (srcHeight / texSize);

  mainTexPosCoords[0] = left;
  mainTexPosCoords[1] = bottom;
  mainTexPosCoords[2] = right;
  mainTexPosCoords[3] = bottom;
  mainTexPosCoords[4] = left;
  mainTexPosCoords[5] = top;
  mainTexPosCoords[6] = left;
  mainTexPosCoords[7] = top;
  mainTexPosCoords[8] = right;
  mainTexPosCoords[9] = bottom; 
  mainTexPosCoords[10] = right;
  mainTexPosCoords[11] = top;
}

static void setupPauseTexRefCoords(void)
{
  float left, right, top, bottom;
  left = -1.0f * 0.0f; //(PAUSE_GUI_WIDTH / 512.0f);
  right = 1.0f * (PAUSE_GUI_WIDTH / 512.0f);
  top = 1.0f * (PAUSE_GUI_HEIGHT / 512.0f);
  bottom = -1.0f * 0.0f; // * (PAUSE_GUI_HEIGHT / 512.0f);

  pauseTexPosCoords[0] = left;
  pauseTexPosCoords[1] = bottom;
  pauseTexPosCoords[2] = right;
  pauseTexPosCoords[3] = bottom;
  pauseTexPosCoords[4] = left;
  pauseTexPosCoords[5] = top;
  pauseTexPosCoords[6] = left;
  pauseTexPosCoords[7] = top;
  pauseTexPosCoords[8] = right;
  pauseTexPosCoords[9] = bottom;
  pauseTexPosCoords[10] = right;
  pauseTexPosCoords[11] = top;

  fprintf(stderr, "setupPauseTexCoords() executed\n");
}

void EGLBlitGL(const void *buf) {
  EGLBlitGLCache(buf, 0);
}

void EGLBlitGLCache(const void *buf, const int cache)
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  switch(texToUse) {
    case TEXTURE_256:
      glBindTexture(GL_TEXTURE_2D, textures[TEXTURE_256]);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 256, 256, 0,
        GL_RGB, GL_UNSIGNED_SHORT_5_6_5, buf);
      if (cache)
        memcpy(tex256buffer, buf, 256 * 256 * 2);
      break;
    case TEXTURE_512:
      glBindTexture(GL_TEXTURE_2D, textures[TEXTURE_512]);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 512, 512, 0,
        GL_RGB, GL_UNSIGNED_SHORT_5_6_5, buf);
      if (cache)
        memcpy(tex512buffer, buf, 512 * 512 * 2);
      break;
    case TEXTURE_1024:
    default:
      glBindTexture(GL_TEXTURE_2D, textures[TEXTURE_1024]);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1024, 1024, 0,
        GL_RGB, GL_UNSIGNED_SHORT_5_6_5, buf);
      break;
  }

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glPushMatrix();
  glTranslatef(translateX, translateY, 0.0f);
  glScalef(scaleWidth, scaleHeight, 1.0f);
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  glFrontFace(GL_CW);
  glVertexPointer(3, GL_FLOAT, 0,mainTexCoords);
  glTexCoordPointer(2, GL_FLOAT, 0, mainTexPosCoords);
  glDrawArrays(GL_TRIANGLES,0,6);
  glDisableClientState(GL_VERTEX_ARRAY);
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
  glPopMatrix();
}

void EGLBlitGBAGL(const void *buf, const float pauseX, 
  const float pauseY, const float scaleX, const float scaleY)
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  EGLBlitPauseGL(buf, pauseX, pauseY, scaleX, scaleY);
}

void EGLBlitPauseGL(const void *buf, const float pauseX, 
  const float pauseY, const float scaleX, const float scaleY)
{
  glBindTexture(GL_TEXTURE_2D, textures[TEXTURE_PAUSE]);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 512, 512, 0,
    GL_RGB, GL_UNSIGNED_SHORT_5_6_5, buf);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glPushMatrix();
  glTranslatef(pauseX, pauseY, 0.0f);
  glScalef(scaleX, scaleY, 1.0f);
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  glFrontFace(GL_CW);
  glVertexPointer(3, GL_FLOAT, 0, mainTexCoords);
  glTexCoordPointer(2, GL_FLOAT, 0, pauseTexPosCoords);
  glDrawArrays(GL_TRIANGLES,0,6);
  glDisableClientState(GL_VERTEX_ARRAY);
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
  glPopMatrix();
}

void EGLSetupGL(void)
{
  int i;

  setupMainTexCoords();
  glGenTextures(TEXTURE_LAST, textures);

  for (i=0; i < TEXTURE_LAST; i++)
  {
    glBindTexture(GL_TEXTURE_2D, textures[i]);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
  }

  glEnable(GL_TEXTURE_2D);
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glDisable(GL_DITHER);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // Background color to red
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

void EGLSrcSizeGui(const unsigned int width, 
	const unsigned int height, const guiSize_t guiSize)
{
  screenshotWidth = srcWidth = (float)width;
  screenshotHeight = srcHeight = (float)height;
  srcAspectRatio = srcWidth / srcHeight;

  /* Determine the texture to render to */
  if ((width <= 256) && (height <= 256)) {
    texToUse = TEXTURE_256;
    memset(tex256buffer, 0x0, 256 * 256 * 2); 
  }
  else if ((width <= 512) && (height <= 512)) {
    texToUse = TEXTURE_512;
    memset(tex512buffer, 0x0, 512 * 512 * 2);
  }
  else
    texToUse = TEXTURE_1024;

  if (guiSize == GUI_SMALL)
  {
    translateX = -1.1f;
    translateY = 2.13f;
    scaleWidth = 4.4f;
    scaleHeight = 4.13f;
  }
  else
  {
    translateX = -1.0f;
    translateY = 1.0f;
    scaleWidth = scaleHeight = 2.0f;
  }

  setupMainTexRefCoords();
}

void EGLSrcSize(const unsigned int width, const unsigned int height)
{
	EGLSrcSizeGui(width, height, GUI_NORMAL);
}

void EGLDestSize(const unsigned int width, const unsigned int height)
{
  destScreenWidth = (float)width;
  destScreenHeight = (float)height;
  destAspectRatio = destScreenWidth / destScreenHeight;

  /* Calculate the pause menu texture coordinates */
  setupPauseTexRefCoords();  
}

