#ifndef SHADER_LOADER_H
#define SHADER_LOADER_H

#include <vitasdk.h>
#include <vitaGL.h>


enum ShaderType {
    RAYMARCHER = 0
};

class ShaderLoader {
public:
    ShaderLoader();
    ~ShaderLoader();

    void loadShader(const char* name);

    GLuint getProgram() const { return program; }

    void useShader();

    GLint uResolution;
    GLint uTime;
    GLint uCamOrigin;
    GLint uCamTarget;
    GLint uFov;
    GLint uSunDir;

    GLint material_metallic;
    GLint material_roughness;
    GLint material_f0;
    GLint material_ambient;
    GLint material_brightness;

    GLint pointLightPos;
    GLint pointLightColor;
    GLint pointLightIntensity;

private:
    GLuint vshader;
    GLuint fshader;
    GLuint program;
};

#endif // SHADER_LOADER_H