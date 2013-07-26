#include <jni.h>
#include <errno.h>

#include <EGL/egl.h>
#include <GLES/gl.h>

#include <android/sensor.h>
#include <android/log.h>
#include <android_native_app_glue.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "native-activity", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "native-activity", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "native-activity", __VA_ARGS__))

struct android_app* app;
EGLDisplay display;
EGLSurface surface;
EGLContext context;

static int init_display(){
  LOGI("Drawing frame");
  const EGLint attribs[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_BLUE_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_RED_SIZE, 8,
            EGL_NONE
    };
    EGLint w, h, dummy, format;
    EGLint numConfigs;
    EGLConfig config;
    // EGLSurface surface;
    // EGLContext context;

    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    eglInitialize(display, 0, 0);
    eglChooseConfig(display, attribs, &config, 1, &numConfigs);
    eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);
    ANativeWindow_setBuffersGeometry(app->window, 0, 0, format);
    surface = eglCreateWindowSurface(display, config, app->window, NULL);
    context = eglCreateContext(display, config, NULL, NULL);

    if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
        LOGW("Unable to eglMakeCurrent");
        return -1;
    }

    eglQuerySurface(display, surface, EGL_WIDTH, &w);
    eglQuerySurface(display, surface, EGL_HEIGHT, &h);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
    glEnable(GL_CULL_FACE);
    glShadeModel(GL_SMOOTH);
    glDisable(GL_DEPTH_TEST);
    return 0;
}

static void draw_frame() {
    LOGI("Drawing frame");
    if (display == NULL) {
      LOGE("Display not found");
      return;    
    }
    glClearColor(1.0f, 1.0f,1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    eglSwapBuffers(display, surface);
}

static void term_display() {
    if (display != EGL_NO_DISPLAY) {
        eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (context != EGL_NO_CONTEXT) {
            eglDestroyContext(display, context);
        }
        if (surface != EGL_NO_SURFACE) {
            eglDestroySurface(display, surface);
        }
        eglTerminate(display);
    }
    // animating = 0;
    display = EGL_NO_DISPLAY;
    context = EGL_NO_CONTEXT;
    surface = EGL_NO_SURFACE;
}


static int32_t handle_input(struct android_app* app, AInputEvent* event) {
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_KEY)
    {
      int key_val = AKeyEvent_getKeyCode(event);
      LOGI("Input Recieved %d", key_val);
    } 
    return 1;
}

static void handle_cmd(struct android_app* app, int32_t cmd) {
  switch (cmd) {
  case APP_CMD_SAVE_STATE:
      break;
  case APP_CMD_INIT_WINDOW:
      if (app->window != NULL) {
          LOGI("Initing window it was not null");
          init_display();
          draw_frame();
      }
      else{
          LOGE("Window was null");
      }
      break;
  case APP_CMD_TERM_WINDOW:
      term_display();
      break;
  case APP_CMD_GAINED_FOCUS:
      break;
  case APP_CMD_LOST_FOCUS:
      break;
  }
}

void android_main(struct android_app* state) {
  LOGI("Working");
  app_dummy();
  state->onAppCmd = handle_cmd;
  state->onInputEvent = handle_input;
  app = state;
  while (1) {
    int ident;
    int events;
    struct android_poll_source* source;
    while ((ident=ALooper_pollAll(-1, NULL, &events,
                (void**)&source)) >= 0) {
      if (source != NULL) {
        source->process(state, source);
      }
      if (state->destroyRequested != 0) {
        term_display();
        return;
      }
    }
  }
}


