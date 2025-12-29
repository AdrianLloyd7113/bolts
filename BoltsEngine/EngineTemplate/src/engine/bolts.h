#pragma once

#include <glm/glm.hpp>
#include <vector>

struct GLFWwindow;
const float WINDOW_WIDTH = 1200;
const float WINDOW_HEIGHT = 900;

extern std::string skyboxPath;
extern bool skyboxEnabled;
extern bool gameActive;

extern glm::vec3 cameraPos;
extern glm::vec3 cameraFront;
extern glm::vec3 cameraUp;
extern unsigned int shaderProgram;
extern unsigned int backgroundShaderProgram;
extern unsigned int backgroundVAO, backgroundVBO;
extern unsigned int crosshairVAO, crosshairVBO, uiShaderProgram;
extern GLFWwindow* window;

extern float pitchLimit;
extern float yawLimit;

#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <limits>

class Shape {
public:
    virtual ~Shape() = default;
    [[nodiscard]] virtual std::vector<glm::vec3> getVertices() const = 0;
    virtual void draw(unsigned int shaderProgram, glm::vec4 color) const = 0;
};

class Triangle : public Shape {
public:
    glm::vec3 a, b, c;

    Triangle(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3)
            : a(p1), b(p2), c(p3) {}

    [[nodiscard]] std::vector<glm::vec3> getVertices() const override {
        return { a, b, c };
    }
    void draw(unsigned int currentShaderProgram, glm::vec4 color) const override{
        float vertices[] = {
                a.x, a.y, a.z, 0.0f, 1.0f, 0.0f,
                b.x, b.y, b.z, 0.0f, 1.0f, 0.0f,
                c.x, c.y, c.z, 0.0f, 1.0f, 0.0f
        };
        unsigned int VAO, VBO;

        int colorLoc = glGetUniformLocation(currentShaderProgram, "uColor");
        glUniform4f(colorLoc, color.r, color.g, color.b, color.a);

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glUseProgram(currentShaderProgram);

        //light position
        glm::vec3 lightPos = glm::vec3(0.0f, 100.0f, 0.0f); // above the scene
        glUniform3fv(glGetUniformLocation(currentShaderProgram, "lightPos"), 1, &lightPos[0]);

        //view position
        glUniform3fv(glGetUniformLocation(currentShaderProgram, "viewPos"), 1, &cameraPos[0]);

        glDrawArrays(GL_TRIANGLES, 0, 3);

        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
    }
};

class Rectangle : public Shape {
    glm::vec3 topLeft;
    glm::vec3 bottomRight;

public:
    Triangle t1;
    Triangle t2;

    Rectangle(glm::vec3 tl, glm::vec3 br)
            : topLeft(tl),
              bottomRight(br),
              t1(tl,glm::vec3(tl.x, br.y, br.z),glm::vec3(br.x, tl.y, tl.z)),
              t2(glm::vec3(tl.x, br.y, br.z),br,glm::vec3(br.x, tl.y, tl.z)) {}

    [[nodiscard]] std::vector<glm::vec3> getVertices() const override {
        return {
                t1.a, t1.b, t1.c,
                t2.a, t2.b, t2.c
        };
    }

    void draw(unsigned int currentShaderProgram, glm::vec4 color) const override{
        t1.draw(currentShaderProgram, color);
        t2.draw(currentShaderProgram, color);
    }
};

class Physical {
public:
    std::vector<std::shared_ptr<Shape>> mesh;
    glm::vec4 colour;

    float width;
    float height;
    float depth;

    Physical(const std::vector<std::shared_ptr<Shape>>& initMesh, glm::vec4 colour) :
            mesh(initMesh), colour(colour){

        computeBounds();

    }

    void draw(unsigned int currentShaderProgram){
        for (const auto& shape : mesh) {
            shape->draw(currentShaderProgram, colour);
        }
    }

private:
    void computeBounds() {
        glm::vec3 minBounds = glm::vec3(std::numeric_limits<float>::max());
        glm::vec3 maxBounds = glm::vec3(std::numeric_limits<float>::lowest());

        for (const auto& shape : mesh) {
            for (const auto& v : shape->getVertices()) {
                minBounds = glm::min(minBounds, v);
                maxBounds = glm::max(maxBounds, v);
            }
        }

        width = maxBounds.x - minBounds.x;
        height = maxBounds.y - minBounds.y;
        depth = maxBounds.z - minBounds.z;
    }
};

class Camera{
public:
    glm::vec3 pos;
    glm::vec3 front;
    glm::vec3 up;

    Camera(glm::vec3 position, glm::vec3 frontDir, glm::vec3 upDir) {
        pos = position;
        front = frontDir;
        up = upDir;
    }
};

extern std::vector<std::unique_ptr<Physical>> physicalWorld;
extern std::vector<std::unique_ptr<Shape>> surfaces;

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

//Cameras
void setCamera(Camera target);

//Skyboxes
unsigned int initSkybox(const char* faces[6]);
void renderSkybox();

void drawScene();

extern bool isPaused;

extern bool escPressedLastFrame;

extern float lastX;
extern float lastY;
extern bool firstMouse;
extern float yaw;
extern float pitch;
extern float sensitivity;
extern float playerSpeed;

extern const char* vertexShaderSource;
extern const char* fragmentShaderSource;
extern const char* backgroundVertexShader;
extern const char* backgroundFragmentShader;
extern const char* uiVertexShaderSource;
extern const char* uiFragmentShaderSource;