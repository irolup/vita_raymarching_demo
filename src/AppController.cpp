#include "AppController.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

const float AppController::DELTA_TIME = 1.0f / 60.0f;

const GLfloat AppController::QUAD_VERTICES[] = {
    //   x,   y,   u,  v
    -1.0f, -1.0f,  0.0f, 0.0f,
     1.0f, -1.0f,  1.0f, 0.0f,
     1.0f,  1.0f,  1.0f, 1.0f,
    -1.0f,  1.0f,  0.0f, 1.0f
};

const GLushort AppController::QUAD_INDICES[] = { 0, 1, 2,  0, 2, 3 };

AppController::AppController() 
    : m_initialized(false)
    , m_running(false)
    , m_vbo(0)
    , m_ibo(0)
    , m_time(0.0f)
    , m_cameraYaw(0.0f)
    , m_cameraPitch(1.2f)
#ifdef LINUX_BUILD
    , m_window(nullptr)
    , m_mouseX(0.0)
    , m_mouseY(0.0)
    , m_firstMouse(true)
    , m_leftMousePressed(false)
#endif
{
}

AppController::~AppController() {
    cleanup();
}

bool AppController::initialize() {
    if (m_initialized) {
        return true;
    }
    
#ifdef LINUX_BUILD

    if (!glfwInit()) {
        printf("Failed to initialize GLFW\n");
        return false;
    }
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    
    m_window = glfwCreateWindow(960, 544, "PSVita RayMarching - Linux", nullptr, nullptr);
    if (!m_window) {
        printf("Failed to create GLFW window\n");
        glfwTerminate();
        return false;
    }
    
    glfwMakeContextCurrent(m_window);
    glfwSetWindowUserPointer(m_window, this);
    glfwSetKeyCallback(m_window, keyCallback);
    glfwSetMouseButtonCallback(m_window, mouseButtonCallback);
    
    
    if (glewInit() != GLEW_OK) {
        printf("Failed to initialize GLEW\n");
        return false;
    }
    

    glfwSwapInterval(1);
#else

    vglInit(0x01000000);
    
    vglUseVram(GL_TRUE);
    vglWaitVblankStart(GL_TRUE);
#endif
    
    setupGL();
    
    m_shaderLoader.loadShader("raymarch");

    createQuadGeometry();

    m_shaderLoader.useShader();

    if (m_shaderLoader.uResolution >= 0) {
        glUniform2f(m_shaderLoader.uResolution, 960.0f, 544.0f);
    }
    if (m_shaderLoader.uFov >= 0) {
        glUniform1f(m_shaderLoader.uFov, 60.0f * (3.1415926f / 180.0f));
    }
    if (m_shaderLoader.uSunDir >= 0) {
        glUniform3f(m_shaderLoader.uSunDir, 0.6f, 0.8f, 0.4f);
    }
    
    if (m_shaderLoader.material_metallic >= 0) {
        glUniform1f(m_shaderLoader.material_metallic, 0.0f);
    }
    if (m_shaderLoader.material_roughness >= 0) {
        glUniform1f(m_shaderLoader.material_roughness, 0.5f);
    }
    if (m_shaderLoader.material_f0 >= 0) {
        glUniform1f(m_shaderLoader.material_f0, 0.02f);
    }
    if (m_shaderLoader.material_ambient >= 0) {
        glUniform1f(m_shaderLoader.material_ambient, 0.3f);
    }
    if (m_shaderLoader.material_brightness >= 0) {
        glUniform1f(m_shaderLoader.material_brightness, 1.0f);
    }
    
    if (m_shaderLoader.pointLightPos >= 0) {
        glUniform3f(m_shaderLoader.pointLightPos, 0.0f, 3.0f, 0.0f);
    }
    if (m_shaderLoader.pointLightColor >= 0) {
        glUniform3f(m_shaderLoader.pointLightColor, 0.4f, 0.4f, 0.4f);
    }
    if (m_shaderLoader.pointLightIntensity >= 0) {
        glUniform1f(m_shaderLoader.pointLightIntensity, 0.2f);
    }
    
#ifndef LINUX_BUILD

    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
#endif
    
    m_initialized = true;
    return true;
}

void AppController::run() {
    if (!m_initialized) {
        return;
    }
    
    m_running = true;
    
#ifndef LINUX_BUILD
    SceCtrlData pad;
    memset(&pad, 0, sizeof(pad));
#endif
    
    
#ifdef LINUX_BUILD
    while (m_running && !glfwWindowShouldClose(m_window)) {

        glfwPollEvents();
        

        if (!handleInput()) {
            break;
        }

        updateUniforms(m_time);

        render();
        
        glfwSwapBuffers(m_window);
        
        m_time += DELTA_TIME;
    }

#else
    while (m_running) {

        sceCtrlPeekBufferPositive(0, &pad, 1);
        
        if (!handleInput(pad)) {
            break;
        }
        
        updateUniforms(m_time, pad);
        
        render();
        
        vglSwapBuffers(GL_FALSE);
        
        m_time += DELTA_TIME;
    }
#endif
    
    printf("Exiting main loop...\n");
}

void AppController::cleanup() {
    if (m_vbo != 0) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
    
    if (m_ibo != 0) {
        glDeleteBuffers(1, &m_ibo);
        m_ibo = 0;
    }
    
    if (m_initialized) {
#ifdef LINUX_BUILD
        if (m_window) {
            glfwDestroyWindow(m_window);
            m_window = nullptr;
        }
        glfwTerminate();
#else
        vglEnd();
#endif
        m_initialized = false;
    }
}

void AppController::setupGL() {
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glViewport(0, 0, 960, 544);
}

void AppController::createQuadGeometry() {
    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(QUAD_VERTICES), QUAD_VERTICES, GL_STATIC_DRAW);
    
    glGenBuffers(1, &m_ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(QUAD_INDICES), QUAD_INDICES, GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(0); // aPos
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4, (const GLvoid*)(0));
    glEnableVertexAttribArray(1); // aUV
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4, (const GLvoid*)(sizeof(GLfloat) * 2));
}

#ifdef LINUX_BUILD
void AppController::updateUniforms(float time) {
    float radius = 3.5f;
    
    // Only update camera when left mouse button is pressed
    if (m_leftMousePressed) {
        double currentMouseX, currentMouseY;
        glfwGetCursorPos(m_window, &currentMouseX, &currentMouseY);
        
        if (m_firstMouse) {
            m_mouseX = currentMouseX;
            m_mouseY = currentMouseY;
            m_firstMouse = false;
        }
        
        double deltaX = (currentMouseX - m_mouseX) * 0.002; 
        double deltaY = (currentMouseY - m_mouseY) * 0.002;
        m_mouseX = currentMouseX;
        m_mouseY = currentMouseY;
        
        m_cameraYaw += deltaX;
        m_cameraPitch -= deltaY;
        
        m_cameraPitch = fmax(0.1f, fmin(3.0f, m_cameraPitch));
    } else {
        // Reset first mouse flag when not pressing, so next click starts fresh
        m_firstMouse = true;
    }
    
    float cx = sinf(m_cameraYaw) * radius;
    float cz = cosf(m_cameraYaw) * radius;
    float cy = m_cameraPitch;
    
    if (m_shaderLoader.uCamOrigin >= 0) {
        glUniform3f(m_shaderLoader.uCamOrigin, cx, cy, cz);
    }
    if (m_shaderLoader.uCamTarget >= 0) {
        glUniform3f(m_shaderLoader.uCamTarget, 0.0f, 0.6f, 0.0f);
    }
    
    if (m_shaderLoader.uTime >= 0) {
        glUniform1f(m_shaderLoader.uTime, time);
    }
}
#else
void AppController::updateUniforms(float time, const SceCtrlData& pad) {
    float radius = 3.5f;
    
    float ax = (pad.lx - 128) / 128.0f;
    float ay = (pad.ly - 128) / 128.0f;
    
    m_cameraYaw += ax * 0.05f;
    m_cameraPitch += ay * 0.05f;
    
    m_cameraPitch = fmax(0.1f, fmin(3.0f, m_cameraPitch));
    
    float cx = sinf(m_cameraYaw) * radius;
    float cz = cosf(m_cameraYaw) * radius;
    float cy = m_cameraPitch;
    
    if (m_shaderLoader.uCamOrigin >= 0) {
        glUniform3f(m_shaderLoader.uCamOrigin, cx, cy, cz);
    }
    if (m_shaderLoader.uCamTarget >= 0) {
        glUniform3f(m_shaderLoader.uCamTarget, 0.0f, 0.6f, 0.0f);
    }
    
    if (m_shaderLoader.uTime >= 0) {
        glUniform1f(m_shaderLoader.uTime, time);
    }
}
#endif

void AppController::render() {
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
}

#ifdef LINUX_BUILD
bool AppController::handleInput() {

    return m_running;
}

void AppController::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    AppController* app = static_cast<AppController*>(glfwGetWindowUserPointer(window));
    
    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_ESCAPE:
            case GLFW_KEY_Q:
                app->m_running = false;
                break;
        }
    }
}

void AppController::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    AppController* app = static_cast<AppController*>(glfwGetWindowUserPointer(window));
    
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            app->m_leftMousePressed = true;
        } else if (action == GLFW_RELEASE) {
            app->m_leftMousePressed = false;
        }
    }
}
#else
bool AppController::handleInput(const SceCtrlData& pad) {
    if (pad.buttons & SCE_CTRL_START) {
        return false;
    }
    
    return true;
}
#endif
