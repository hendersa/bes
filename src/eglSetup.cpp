#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include <EGL/egl.h>
#include <GLES/gl.h>
#include <SDL.h>
#include "gui.h"
#include "beagleboard.h"

#if defined(PC_PLATFORM)
#include <SDL_syswm.h>
static Display *x11Display = NULL;
#endif /* PC_PLATFORM */


static EGLDisplay display = EGL_NO_DISPLAY;
static EGLSurface surface = EGL_NO_SURFACE;
static EGLContext context = EGL_NO_CONTEXT;
static EGLSurface window  = EGL_NO_SURFACE;

void EGLShutdown(void) {
    fprintf(stderr, "Destroying context\n");

    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(display, context);
    eglDestroySurface(display, surface);
    eglTerminate(display);

    display = EGL_NO_DISPLAY;
    surface = EGL_NO_SURFACE;
    context = EGL_NO_CONTEXT;
    window  = EGL_NO_SURFACE;
    return;
}

void EGLFlip(void)
{
    if (!eglSwapBuffers(display, surface)) {
        fprintf(stderr, "eglSwapBuffers() returned error %d\n", eglGetError());
    }
}

int EGLInitialize(void)
{
    const EGLint attribs[] = {
#if defined(PC_PLATFORM)
	EGL_RED_SIZE,		5,
	EGL_GREEN_SIZE,		6,
	EGL_BLUE_SIZE,		5,
	EGL_SURFACE_TYPE,	EGL_WINDOW_BIT,
	EGL_RENDERABLE_TYPE,	EGL_OPENGL_ES_BIT,
	/*EGL_BIND_TO_TEXTURE_RGBA, EGL_DONT_CARE,*/
	EGL_NONE
#else
        EGL_SURFACE_TYPE, 	EGL_WINDOW_BIT,
	EGL_RENDERABLE_TYPE,	EGL_OPENGL_ES_BIT,
        EGL_BLUE_SIZE, 5,
        EGL_GREEN_SIZE, 6,
        EGL_RED_SIZE, 5,
        EGL_NONE
#endif /* PC_PLATFORM */
    };
    EGLConfig config;    
    EGLint numConfigs;
    EGLint format;
    EGLint width;
    EGLint height;
   
#if defined(PC_PLATFORM)
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    if (SDL_GetWMInfo(&wmInfo) <= 0) {
        fprintf(stderr, "Unable to get window handle\n");
        return 0;
    }
 
    fprintf(stderr, "Initializing context\n");

    x11Display = XOpenDisplay(NULL);
    if (!x11Display) {
        fprintf(stderr, "Unable to fetch X11 display\n");
        return 0;
    }
#endif /* PC_PLATFORM */   
    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    if (display == EGL_NO_DISPLAY) {
        fprintf(stderr, "eglGetDisplay() returned error %d\n", eglGetError());
        return 0;
    }
fprintf(stderr, "Finished eglGetDisplay()\n");
    if (!eglInitialize(display, NULL, NULL)) {
        fprintf(stderr, "eglInitialize() returned error %d\n", eglGetError());
        return 0;
    }
    eglBindAPI(EGL_OPENGL_ES_API);

fprintf(stderr, "Finished eglInitialize()\n");
    if (eglChooseConfig(display, attribs, &config, 1, &numConfigs) != 
        EGL_TRUE || (numConfigs == 0)) {
        fprintf(stderr, "eglChooseConfig() returned error %d\n", eglGetError());
        EGLShutdown();
        return 0;
    }
fprintf(stderr, "Finished eglChooseConfig() (numConfigs: %d)\n", numConfigs);

/* EXTRA START */
    if (!eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format)) {
        fprintf(stderr, "eglGetConfigAttrib() returned error %d\n", eglGetError());
        EGLShutdown();
        return 0;
    }
fprintf(stderr, "Finished eglGetConfigAttrib()\n");
/* EXTRA END */

#if defined(PC_PLATFORM)
    //wmInfo.info.x11.lock_func();
    surface = eglCreateWindowSurface(display, config, 
        (EGLNativeWindowType)wmInfo.info.x11.window, 0);
#else
    surface = eglCreateWindowSurface(display, config, window, 0);
#endif /* PC_PLATFORM */
    if (!surface) {
        fprintf(stderr, "eglCreateWindowSurface() returned error %d", eglGetError());
        EGLShutdown();
        return 0;
    }
fprintf(stderr, "Finished eglCreateWindowSurface()\n");
    EGLint contextParams[] = {EGL_CONTEXT_CLIENT_VERSION, 1, EGL_NONE};
    context = eglCreateContext(display, config, 0, contextParams);
    if (context == EGL_NO_CONTEXT) {
        fprintf(stderr, "eglCreateContext() returned error %d\n", eglGetError());
        EGLShutdown();
        return 0;
    }
fprintf(stderr, "Finished eglCreateContext()\n"); 

    EGLint ver = -1;
    eglQueryContext(display, context, EGL_CONTEXT_CLIENT_VERSION, &ver);
    fprintf(stderr, "EGL Context version: %d\n", ver);

    if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
        fprintf(stderr, "eglMakeCurrent() returned error %d\n", eglGetError());
        EGLShutdown();
        return 0;
    }
fprintf(stderr, "Finished eglMakeCurrent()\n");

//wmInfo.info.x11.unlock_func();
    if (!eglQuerySurface(display, surface, EGL_WIDTH, &width) ||
        !eglQuerySurface(display, surface, EGL_HEIGHT, &height)) {
        fprintf(stderr, "eglQuerySurface() returned error %d\n", eglGetError());
        EGLShutdown();
        return 0;
    }
fprintf(stderr, "Finished eglQuerySurface() width: %d height: %d\n", width, height);

    glEnable(GL_DEPTH_TEST);
    EGLSetupGL();

    return 1;
}

