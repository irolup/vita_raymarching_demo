#pragma once

#ifdef LINUX_BUILD
    #include <GL/glew.h>
    #include <GL/gl.h>
    #include <GLFW/glfw3.h>
#else
    // Enable VitaGL GLSL translator support
    #define GL_GLEXT_PROTOTYPES
    #define EGL_EGLEXT_PROTOTYPES
    #include <vitaGL.h>
    #include <psp2/ctrl.h>
#endif

#include "ShaderLoader.h"

class AppController {
public:
    AppController();
    ~AppController();

    bool initialize();

    void run();

    void cleanup();

private:
    void setupGL();
    
    void createQuadGeometry();
    
#ifdef LINUX_BUILD
    void updateUniforms(float time);
#else
    void updateUniforms(float time, const SceCtrlData& pad);
#endif
    
    void render();
    
#ifdef LINUX_BUILD
    bool handleInput();
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
#else
    bool handleInput(const SceCtrlData& pad);
#endif
    
    bool m_initialized;
    bool m_running;
    
    ShaderLoader m_shaderLoader;
    GLuint m_vbo;
    GLuint m_ibo;
    
#ifdef LINUX_BUILD
    GLFWwindow* m_window;
    double m_mouseX, m_mouseY;
    bool m_firstMouse;
    bool m_leftMousePressed;
#endif
    
    // Timing
    float m_time;
    static const float DELTA_TIME;
    
    // Camera state
    float m_cameraYaw;
    float m_cameraPitch;
    
    // Quad geometry data
    static const GLfloat QUAD_VERTICES[];
    static const GLushort QUAD_INDICES[];
};
