#pragma once

#include <glm/glm.hpp>
#include <vector>

struct GLFWwindow;
const float WINDOW_WIDTH = 1200;
const float WINDOW_HEIGHT = 900;

extern bool skyboxEnabled;
extern bool gameActive;

extern unsigned int shaderProgram;
extern unsigned int backgroundShaderProgram;
extern unsigned int backgroundVAO, backgroundVBO;
extern unsigned int crosshairVAO, crosshairVBO, uiShaderProgram;
extern GLFWwindow* window;

extern float pitchLimit;
extern float yawLimit;

class Triangle{
public:
    glm::vec3 a;
    glm::vec3 b;
    glm::vec3 c;

    Triangle(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3){
        a = p1;
        b = p2;
        c = p3;
    }

    Triangle() = default;

};

class Rectangle{
private:
    glm::vec3 topLeft;
    glm::vec3 bottomRight;
public:
    Triangle a;
    Triangle b;

    Rectangle(glm::vec3 topLeft, glm::vec3 bottomRight) {
        this->topLeft = topLeft;
        this->bottomRight = bottomRight;
        glm::vec3 topRight(bottomRight.x, topLeft.y, topLeft.z);
        glm::vec3 bottomLeft(topLeft.x, bottomRight.y, bottomRight.z);

        a = Triangle(topLeft, bottomLeft, topRight);
        b = Triangle(bottomLeft, bottomRight, topRight);
    }

    glm::vec3 getTopLeft(){
        return topLeft;
    }

    glm::vec3 getBottomRight(){
        return bottomRight;
    }

};

unsigned int createShaderProgram();
unsigned int createBackgroundShaderProgram();
unsigned int createUIShaderProgram();

void mouseInput(GLFWwindow* window, double xpos, double ypos);
void keyboardInput(GLFWwindow* currentWindow);
void framebuffer_size_callback(GLFWwindow* currentWindow, int width, int height);

void drawTriangle(Triangle toDraw, unsigned int currentShaderProgram, glm::vec4 color);
void drawRectangle(Rectangle toDraw, unsigned int currentShaderProgram, glm::vec4 color);
void renderPauseMenu(unsigned int pauseShaderProgram);

void startEngine();
void engineUpdate();

//rendering
void engineBeginFrame();
void engineEndFrame();
void handleUI();

//Skyboxes

unsigned int initSkybox(const char* faces[6]);
void renderSkybox();

extern bool isPaused;

extern bool escPressedLastFrame;

extern glm::vec3 cameraPos;
extern glm::vec3 cameraFront;
extern glm::vec3 cameraUp;

extern float lastX;
extern float lastY;
extern bool firstMouse;
extern float yaw;
extern float pitch;
extern float sensitivity;
extern const float playerSpeed;

extern const char* vertexShaderSource;
extern const char* fragmentShaderSource;
extern const char* backgroundVertexShader;
extern const char* backgroundFragmentShader;
extern const char* uiVertexShaderSource;
extern const char* uiFragmentShaderSource;

extern std::vector<Rectangle> surfaces;
