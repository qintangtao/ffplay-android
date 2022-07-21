#include "SDL_android.h"
#include <jni.h>
#include <pthread.h>
#include "j4a_allclasses.h"
#include "SDL_log.h"


/*******************************************************************************
                               Globals
*******************************************************************************/
static pthread_key_t mThreadKey;
static pthread_once_t key_once = PTHREAD_ONCE_INIT;
static JavaVM *mJavaVM = NULL;

static int
Android_JNI_SetEnv(JNIEnv *env) {
    int status = pthread_setspecific(mThreadKey, env);
    if (status < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed pthread_setspecific() in Android_JNI_SetEnv() (err=%d)", status);
    }
    return status;
}

JNIEnv* Android_JNI_GetEnv(void)
{
    JNIEnv *env = pthread_getspecific(mThreadKey);
    if (env == NULL) {
        int status;

        if (mJavaVM == NULL) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed, there is no JavaVM");
            return NULL;
        }

        status = (*mJavaVM)->AttachCurrentThread(mJavaVM, &env, NULL);
        if (status < 0) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to attach current thread (err=%d)", status);
            return NULL;
        }

        if (Android_JNI_SetEnv(env) < 0) {
            return NULL;
        }
    }

    return env;
}

int Android_JNI_SetupThread(void)
{
    JNIEnv *env;
    int status;

    if (mJavaVM == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed, there is no JavaVM");
        return 0;
    }

    status = (*mJavaVM)->AttachCurrentThread(mJavaVM, &env, NULL);
    if (status < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to attach current thread (err=%d)", status);
        return 0;
    }

    if (Android_JNI_SetEnv(env) < 0) {
        return 0;
    }

    return 1;
}

static void
Android_JNI_ThreadDestroyed(void *value)
{
    JNIEnv *env = (JNIEnv *) value;
    if (env != NULL) {
        (*mJavaVM)->DetachCurrentThread(mJavaVM);
        Android_JNI_SetEnv(NULL);
    }
}

static void
Android_JNI_CreateKey(void)
{
    int status = pthread_key_create(&mThreadKey, Android_JNI_ThreadDestroyed);
    if (status < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Error initializing mThreadKey with pthread_key_create() (err=%d)", status);
    }
}

static void
Android_JNI_CreateKey_once(void)
{
    int status = pthread_once(&key_once, Android_JNI_CreateKey);
    if (status < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Error initializing mThreadKey with pthread_once() (err=%d)", status);
    }
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved)
{
    mJavaVM = vm;
    JNIEnv *env = NULL;

    if ((*mJavaVM)->GetEnv(mJavaVM, (void **)&env, JNI_VERSION_1_4) != JNI_OK) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to get JNI Env");
        return JNI_VERSION_1_6;
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "J4A_LoadAll__catchAll");
    if (0 != J4A_LoadAll__catchAll(env)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed J4A_LoadAll__catchAll");
        return JNI_VERSION_1_6;
    }

    Android_JNI_CreateKey_once();

    return JNI_VERSION_1_6;
}