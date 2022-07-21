#ifndef SDL_android_h_
#define SDL_android_h_

#ifdef __cplusplus
extern "C" {
#endif

#include <jni.h>

JNIEnv *Android_JNI_GetEnv(void);
int Android_JNI_SetupThread(void);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*SDL_android_h_*/