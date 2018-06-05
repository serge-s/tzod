#include <jni.h>
#include <android/log.h>

#include <GLES2/gl2.h>
#include <app/tzod.h>
#include <app/View.h>
#include <fs/FileSystem.h>
#include <ui/ConsoleBuffer.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define  LOG_TAG    "libtzodjni"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

static void printGLString(const char *name, GLenum s) {
    const char *v = (const char *) glGetString(s);
    LOGI("GL %s = %s\n", name, v);
}

static void checkGlError(const char* op) {
    for (GLint error = glGetError(); error; error
            = glGetError()) {
        LOGI("after %s() glError (0x%x)\n", op, error);
    }
}

auto gVertexShader =
    "attribute vec4 vPosition;\n"
    "void main() {\n"
    "  gl_Position = vPosition;\n"
    "}\n";

auto gFragmentShader =
    "precision mediump float;\n"
    "void main() {\n"
    "  gl_FragColor = vec4(0.0, 1.0, 0.0, 1.0);\n"
    "}\n";

GLuint loadShader(GLenum shaderType, const char* pSource) {
    GLuint shader = glCreateShader(shaderType);
    if (shader) {
        glShaderSource(shader, 1, &pSource, NULL);
        glCompileShader(shader);
        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            GLint infoLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
            if (infoLen) {
                char* buf = (char*) malloc(infoLen);
                if (buf) {
                    glGetShaderInfoLog(shader, infoLen, NULL, buf);
                    LOGE("Could not compile shader %d:\n%s\n",
                            shaderType, buf);
                    free(buf);
                }
                glDeleteShader(shader);
                shader = 0;
            }
        }
    }
    return shader;
}

GLuint createProgram(const char* pVertexSource, const char* pFragmentSource) {
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, pVertexSource);
    if (!vertexShader) {
        return 0;
    }

    GLuint pixelShader = loadShader(GL_FRAGMENT_SHADER, pFragmentSource);
    if (!pixelShader) {
        return 0;
    }

    GLuint program = glCreateProgram();
    if (program) {
        glAttachShader(program, vertexShader);
        checkGlError("glAttachShader");
        glAttachShader(program, pixelShader);
        checkGlError("glAttachShader");
        glLinkProgram(program);
        GLint linkStatus = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        if (linkStatus != GL_TRUE) {
            GLint bufLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
            if (bufLength) {
                char* buf = (char*) malloc(bufLength);
                if (buf) {
                    glGetProgramInfoLog(program, bufLength, NULL, buf);
                    LOGE("Could not link program:\n%s\n", buf);
                    free(buf);
                }
            }
            glDeleteProgram(program);
            program = 0;
        }
    }
    return program;
}

GLuint gProgram;
GLuint gvPositionHandle;

bool setupGraphics(int w, int h) {
    printGLString("Version", GL_VERSION);
    printGLString("Vendor", GL_VENDOR);
    printGLString("Renderer", GL_RENDERER);
    printGLString("Extensions", GL_EXTENSIONS);

    LOGI("setupGraphics(%d, %d)", w, h);
    gProgram = createProgram(gVertexShader, gFragmentShader);
    if (!gProgram) {
        LOGE("Could not create program.");
        return false;
    }
    gvPositionHandle = glGetAttribLocation(gProgram, "vPosition");
    checkGlError("glGetAttribLocation");
    LOGI("glGetAttribLocation(\"vPosition\") = %d\n",
            gvPositionHandle);

    glViewport(0, 0, w, h);
    checkGlError("glViewport");
    return true;
}

const GLfloat gTriangleVertices[] = { 0.0f, 0.5f, -0.5f, -0.5f,
        0.5f, -0.5f };

void renderFrame() {
    static float grey;
    grey += 0.01f;
    if (grey > 1.0f) {
        grey = 0.0f;
    }
    glClearColor(grey, grey, grey, 1.0f);
    checkGlError("glClearColor");
    glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    checkGlError("glClear");

    glUseProgram(gProgram);
    checkGlError("glUseProgram");

    glVertexAttribPointer(gvPositionHandle, 2, GL_FLOAT, GL_FALSE, 0, gTriangleVertices);
    checkGlError("glVertexAttribPointer");
    glEnableVertexAttribArray(gvPositionHandle);
    checkGlError("glEnableVertexAttribArray");
    glDrawArrays(GL_TRIANGLES, 0, 3);
    checkGlError("glDrawArrays");
}

namespace
{
    class ConsoleLog final
            : public UI::IConsoleLog
    {
    public:
        // IConsoleLog
        void WriteLine(int severity, std::string_view str) override
        {
            __android_log_print(severity ? ANDROID_LOG_ERROR : ANDROID_LOG_INFO, LOG_TAG,
                                "%.*s\n", static_cast<int>(str.size()), str.data());
        }
        void Release() override
        {
            delete this;
        }
    };
}

#include <plat/AppWindow.h>
#include <ui/Clipboard.h>
#include <ui/UIInput.h>
#include <video/RenderOpenGL.h>

struct JniClipboard : public UI::IClipboard
{
    std::string_view GetClipboardText() const { return {}; }
    void SetClipboardText(std::string text) {}
};

struct JniInput : public UI::IInput
{
    bool IsKeyPressed(UI::Key key) const override
    {
        return false;
    }
    bool IsMousePressed(int button) const override
    {
        return false;
    }
    vec2d GetMousePos() const override
    {
        return {};
    }
    UI::GamepadState GetGamepadState(unsigned int index) const override
    {
        return {};
    }
    bool GetSystemNavigationBackAvailable() const override
    {
        return true;
    }
};

class JniAppWindow : public AppWindow
{
    AppWindowInputSink *_inputSink = nullptr;
    JniClipboard _clipboard;
    JniInput _input;
    std::unique_ptr<IRender> _render;
public:
    JniAppWindow()
    {
        _render = RenderCreateOpenGL();
    }
    AppWindowInputSink* GetInputSink() const override
    {
        return _inputSink;
    }
    void SetInputSink(AppWindowInputSink *inputSink) override
    {
        _inputSink = inputSink;
    }
    int GetDisplayRotation() const override
    {
        return 0;
    }
    vec2d GetPixelSize() const override
    {
        return {500, 500};
    }
    float GetLayoutScale() const override
    {
        return 1;
    }
    UI::IClipboard& GetClipboard() override { return _clipboard; }
    UI::IInput& GetInput() override
    {
        return _input;
    }
    IRender& GetRender() override
    {
        return *_render;
    }
    void SetCanNavigateBack(bool canNavigateBack) override {}
    void SetMouseCursor(MouseCursor mouseCursor) override {}
    void MakeCurrent() override  {}
    void Present() override {}
};

struct State
{
    UI::ConsoleBuffer logger;
    std::shared_ptr<FS::FileSystem> fs;
    TzodApp app;
    JniAppWindow appWindow;
    TzodView view;

    State()
        : logger(80, 100)
        , fs(FS::CreateOSFileSystem("data"))
        , app(*fs, logger)
        , view(*fs, logger, app, appWindow)
    {
        logger.SetLog(new ConsoleLog());
    }
};

std::unique_ptr<State> g_state;

extern "C" JNIEXPORT void JNICALL Java_com_neaoo_tzod_TZODJNILib_init(JNIEnv * env, jobject obj,  jint width, jint height)
{
    g_state = std::make_unique<State>();
    setupGraphics(width, height);
}

extern "C" JNIEXPORT void JNICALL Java_com_neaoo_tzod_TZODJNILib_step(JNIEnv * env, jobject obj)
{
    renderFrame();
    g_state->view.Step(0.16);
}