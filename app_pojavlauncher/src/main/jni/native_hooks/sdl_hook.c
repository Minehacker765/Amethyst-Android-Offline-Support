
#include "environ/environ.h"
#include "utils.h"
#include "native_hooks.h"
#include "log.h"

#include <bytehook.h>
#include <dlfcn.h>
#include <jni.h>
#include <stdlib.h>

typedef bool (*SDL_InitSubSystem_Func)(uint32_t);

static bool custom_SDL_InitSubSystem_Func(uint32_t flags) {
    // Call notifyLauncher on SDL_InitSubSystem, this sets up all the JNI stuff needed by SDL.
    JNIEnv *dvm_env;
    dvm_env = get_attached_env(pojav_environ->dalvikJavaVMPtr);
    if (dvm_env == ((void *) 0)) {printf("SDL_InitSubSystem notify to launcher-side integration failed!\n");}

    // Just in case of bozo
    jint safeFlags;
    if (flags > INT32_MAX) {
        safeFlags = -1;
    } else safeFlags = (jint)flags;

    jint type = 0; // SDL
    jint action[] = {0, safeFlags}; // INIT, FLAG
    jintArray actionArray = (*dvm_env)->NewIntArray(dvm_env, 2);
    (*dvm_env)->SetIntArrayRegion(dvm_env, actionArray, 0, 2, action);
    jboolean result = (*dvm_env)->CallStaticBooleanMethod(dvm_env, pojav_environ->bridgeClazz,
            pojav_environ->method_notifyLauncher, type, actionArray);

    // Call original func after doing all the needed setup
    bool r = BYTEHOOK_CALL_PREV(custom_SDL_InitSubSystem_Func, SDL_InitSubSystem_Func, flags);
    BYTEHOOK_POP_STACK();
    return r;
}

void create_sdl_hooks(bytehook_hook_all_t bytehook_hook_all_p) {
    bytehook_stub_t stub_SDL_InitSubSystem = bytehook_hook_all_p(NULL, "SDL_InitSubSystem", &custom_SDL_InitSubSystem_Func, NULL, NULL);
    LOGI("Successfully initialized SDL hooks, stubs: %p", stub_SDL_InitSubSystem);
}