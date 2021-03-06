#include <jni.h>
#include <android/log.h>

#include <GLES2/gl2.h>
#include <app/tzod.h>
#include <app/View.h>
#include <fsjni/FileSystemJni.h>
#include <fsposix/FileSystemPosix.h>
#include <plat/ConsoleBuffer.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define  LOG_TAG    "libtzodjni"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)

static void printGLString(const char *name, GLenum s) {
    const char *v = (const char *) glGetString(s);
    LOGI("GL %s = %s\n", name, v);
}

#include <platjni/JniAppWindow.h>
#include <platjni/JniConsoleLog.h>
#include <android/asset_manager_jni.h>
#include <android/input.h>

static std::shared_ptr<FS::FileSystem> InitFileSystem(AAssetManager *assetManager)
{
    auto fs = std::make_shared<FS::FileSystemJni>(assetManager);
    FILE *f = fopen("/proc/self/cmdline", "r");
    char cmdline[256] = {};
    fread(cmdline, 1, 255, f);
    fclose(f);
    fs->Mount("user", std::make_shared<FS::FileSystemPosix>(std::string("/data/data/") + cmdline)->GetFileSystem("files", true /*create*/));
    return fs;
}

struct State
{
    Plat::ConsoleBuffer logger;
    std::shared_ptr<FS::FileSystem> fs;
    TzodApp app;
    JniAppWindow appWindow;
    TzodView view;
    std::mutex mutex;

    State(AAssetManager *assetManager)
        : logger(80, 100)
        , fs(InitFileSystem(assetManager))
        , app(*fs, logger)
        , view(*fs, logger, app, nullptr /*cmdClose*/)
    {
        logger.SetLog(new JniConsoleLog());
    }
};

std::unique_ptr<State> g_state;

extern "C" JNIEXPORT void JNICALL Java_com_neaoo_tzod_TZODJNILib_init(JNIEnv *env, jobject obj, jobject assetManager)
{
    g_state = std::make_unique<State>(AAssetManager_fromJava(env, assetManager));

    printGLString("Version", GL_VERSION);
    printGLString("Vendor", GL_VENDOR);
    printGLString("Renderer", GL_RENDERER);
    printGLString("Extensions", GL_EXTENSIONS);
}

extern "C" JNIEXPORT void JNICALL Java_com_neaoo_tzod_TZODJNILib_resize(JNIEnv *env, jobject obj, jint width, jint height)
{
    std::lock_guard<std::mutex> lock(g_state->mutex);
    g_state->appWindow.SetPixelSize(vec2d{static_cast<float>(width), static_cast<float>(height)});
}

extern "C" JNIEXPORT void JNICALL Java_com_neaoo_tzod_TZODJNILib_step(JNIEnv *env, jobject obj)
{
    std::lock_guard<std::mutex> lock(g_state->mutex);
    g_state->view.Step(g_state->app, 0.016F, g_state->appWindow.GetInput());
    g_state->view.GetAppWindowInputSink().OnRefresh(g_state->appWindow, g_state->appWindow.GetRender(), g_state->appWindow.GetRenderBinding());
    g_state->appWindow.Present();
}

#include <plat/Input.h>

extern "C" JNIEXPORT void JNICALL Java_com_neaoo_tzod_TZODJNILib_pointer(
        JNIEnv *env, jobject obj, jint actionMasked, jint pointerId, jfloat x, jfloat y)
{
    Plat::Msg action;
    switch (actionMasked)
    {
    case AMOTION_EVENT_ACTION_DOWN:
    case AMOTION_EVENT_ACTION_POINTER_DOWN:
        action = Plat::Msg::PointerDown;
        break;
    case AMOTION_EVENT_ACTION_UP:
    case AMOTION_EVENT_ACTION_POINTER_UP:
        action = Plat::Msg::PointerUp;
        break;
    case AMOTION_EVENT_ACTION_MOVE:
        action = Plat::Msg::PointerMove;
        break;
    default:
        return;
    }

    Plat::PointerType pointerType = Plat::PointerType::Touch;
    vec2d pxPointerPos = {x, y};
    vec2d pxPointerOffset = {};
    int buttons = 0;

    std::lock_guard<std::mutex> lock(g_state->mutex);
    g_state->view.GetAppWindowInputSink().OnPointer(g_state->appWindow, pointerType, action, pxPointerPos, pxPointerOffset, buttons, (unsigned) pointerId);
}
