#include "ShaderLoader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

static void fatal_error(const char* fmt, ...) {
    va_list list;
    char string[512];

    va_start(list, fmt);
    vsnprintf(string, sizeof(string), fmt, list);
    va_end(list);

#ifdef LINUX_BUILD
    printf("FATAL ERROR: %s\n", string);
    exit(1);
#else
    SceMsgDialogUserMessageParam msg_param;
    memset(&msg_param, 0, sizeof(msg_param));
    msg_param.buttonType = SCE_MSG_DIALOG_BUTTON_TYPE_OK;
    msg_param.msg = (SceChar8*)string;

    SceMsgDialogParam param;
    sceMsgDialogParamInit(&param);
    _sceCommonDialogSetMagicNumber(&param.commonParam);
    param.mode = SCE_MSG_DIALOG_MODE_USER_MSG;
    param.userMsgParam = &msg_param;

    sceMsgDialogInit(&param);
    while (sceMsgDialogGetStatus() != SCE_COMMON_DIALOG_STATUS_FINISHED) {
        glClear(GL_COLOR_BUFFER_BIT);
        vglSwapBuffers(GL_TRUE);
    }
    sceMsgDialogTerm();
    sceKernelExitProcess(0);
    while (1);
#endif
}

ShaderLoader::ShaderLoader() {
    vshader = 0;
    fshader = 0;
    program = 0;
    uResolution = -1;
    uTime = -1;
    uCamOrigin = -1;
    uCamTarget = -1;
    uFov = -1;
    uSunDir = -1;

    material_metallic = -1;
    material_roughness = -1;
    material_f0 = -1;
    material_ambient = -1;
    material_brightness = -1;

    pointLightPos = -1;
    pointLightColor = -1;
    pointLightIntensity = -1;
}

ShaderLoader::~ShaderLoader() {
    if (program) glDeleteProgram(program);
    if (vshader) glDeleteShader(vshader);
    if (fshader) glDeleteShader(fshader);
}

void ShaderLoader::loadShader(const char* name) {
    char fname[256];
    FILE* f;
    char* vshadersrc;
    char* fshadersrc;
    int32_t vsize, fsize;

#ifdef LINUX_BUILD
    sprintf(fname, "assets/shaders/%s_glsl.vert", name);
#else
    sprintf(fname, "app0:%s.vert", name);
#endif
    f = fopen(fname, "r");
    if (!f) fatal_error("Cannot open %s", fname);
    fseek(f, 0, SEEK_END);
    vsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    vshadersrc = (char*)malloc(vsize);
    fread(vshadersrc, 1, vsize, f);
    fclose(f);

#ifdef LINUX_BUILD
    sprintf(fname, "assets/shaders/%s_glsl.frag", name);
#else
    sprintf(fname, "app0:%s.frag", name);
#endif
    f = fopen(fname, "r");
    if (!f) fatal_error("Cannot open %s", fname);
    fseek(f, 0, SEEK_END);
    fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    fshadersrc = (char*)malloc(fsize);
    fread(fshadersrc, 1, fsize, f);
    fclose(f);

    vshader = glCreateShader(GL_VERTEX_SHADER);
    fshader = glCreateShader(GL_FRAGMENT_SHADER);
    program = glCreateProgram();

    if (vshader == 0 || fshader == 0 || program == 0) {
        fatal_error("Failed to create OpenGL objects: vs=%u fs=%u prog=%u", vshader, fshader, program);
    }

#ifdef LINUX_BUILD
    vshadersrc = (char*)realloc(vshadersrc, vsize + 1);
    vshadersrc[vsize] = '\0';
    fshadersrc = (char*)realloc(fshadersrc, fsize + 1);
    fshadersrc[fsize] = '\0';
    
    glShaderSource(vshader, 1, (const char**)&vshadersrc, nullptr);
    glCompileShader(vshader);

    GLint vcompiled = 0;
    glGetShaderiv(vshader, GL_COMPILE_STATUS, &vcompiled);
    if (!vcompiled) {
        GLint logLength = 0;
        glGetShaderiv(vshader, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength > 0) {
            char* log = (char*)malloc(logLength);
            glGetShaderInfoLog(vshader, logLength, nullptr, log);
            fatal_error("Vertex shader compilation failed:\n%s", log);
            free(log);
        } else {
            fatal_error("Vertex shader compilation failed (no log available)");
        }
    }

    glShaderSource(fshader, 1, (const char**)&fshadersrc, nullptr);
    glCompileShader(fshader);

    GLint fcompiled = 0;
    glGetShaderiv(fshader, GL_COMPILE_STATUS, &fcompiled);
    if (!fcompiled) {
        GLint logLength = 0;
        glGetShaderiv(fshader, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength > 0) {
            char* log = (char*)malloc(logLength);
            glGetShaderInfoLog(fshader, logLength, nullptr, log);
            fatal_error("Fragment shader compilation failed:\n%s", log);
            free(log);
        } else {
            fatal_error("Fragment shader compilation failed (no log available)");
        }
    }
#else
    printf("Loading shaders: vsize=%d, fsize=%d\n", vsize, fsize);
    glShaderSource(vshader, 1, &vshadersrc, &vsize);
    glCompileShader(vshader);

    GLint vcompiled = 0;
    glGetShaderiv(vshader, GL_COMPILE_STATUS, &vcompiled);
    if (!vcompiled) {
        fatal_error("Vertex shader compilation failed");
    }

    glShaderSource(fshader, 1, &fshadersrc, &fsize);
    glCompileShader(fshader);

    GLint fcompiled = 0;
    glGetShaderiv(fshader, GL_COMPILE_STATUS, &fcompiled);
    if (!fcompiled) {
        fatal_error("Fragment shader compilation failed");
    }
#endif

    glAttachShader(program, vshader);
    glAttachShader(program, fshader);

    glBindAttribLocation(program, 0, "aPos");
    glBindAttribLocation(program, 1, "aUV");

    glLinkProgram(program);

    GLint linked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked) {
#ifdef LINUX_BUILD
        GLint logLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength > 0) {
            char* log = (char*)malloc(logLength);
            glGetProgramInfoLog(program, logLength, nullptr, log);
            fatal_error("Shader program linking failed:\n%s", log);
            free(log);
        } else {
            fatal_error("Shader program linking failed (no log available)");
        }
#else
        fatal_error("Shader program linking failed");
#endif
    }

    printf("Shader program linked successfully: %u\n", program);

    uResolution = glGetUniformLocation(program, "uResolution");
    uTime = glGetUniformLocation(program, "uTime");
    uCamOrigin = glGetUniformLocation(program, "uCamOrigin");
    uCamTarget = glGetUniformLocation(program, "uCamTarget");
    uFov = glGetUniformLocation(program, "uFov");
    uSunDir = glGetUniformLocation(program, "uSunDir");

    material_metallic = glGetUniformLocation(program, "material_metallic");
    material_roughness = glGetUniformLocation(program, "material_roughness");
    material_f0 = glGetUniformLocation(program, "material_f0");
    material_ambient = glGetUniformLocation(program, "material_ambient");
    material_brightness = glGetUniformLocation(program, "material_brightness");

    pointLightPos = glGetUniformLocation(program, "pointLightPos");
    pointLightColor = glGetUniformLocation(program, "pointLightColor");
    pointLightIntensity = glGetUniformLocation(program, "pointLightIntensity");

    printf("Uniform locations: res=%d time=%d origin=%d target=%d fov=%d sun=%d\n",
           uResolution, uTime, uCamOrigin, uCamTarget, uFov, uSunDir);

    free(vshadersrc);
    free(fshadersrc);
}

void ShaderLoader::useShader() {
    glUseProgram(program);
}