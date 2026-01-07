#pragma once

#include <glm/glm.hpp>
#include <vector>

//MISC VARIABLE DECLARATIONS

//Default window dimensions
struct GLFWwindow;
const float WINDOW_WIDTH = 1200;
const float WINDOW_HEIGHT = 900;

//Time
extern float deltaTime;
extern float lastFrameTime;

//Global game control variables
extern bool skyboxEnabled;
extern bool gameActive;

#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <limits>

//CAMERAS

//Global camera control variables
extern glm::vec3 cameraPos;
extern glm::vec3 cameraFront;
extern glm::vec3 cameraUp;

//Camera class to store positional and rotational data about cameras
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

//Function to set current camera to target Camera object
void setCamera(Camera target);

//GEOMETRY AND RENDERING

//Basic geometry classes (including generic "Shape")
class Shape {
public:
    virtual ~Shape() = default;
    [[nodiscard]] virtual std::vector<glm::vec3> getVertices() const = 0;
    virtual void draw(unsigned int shaderProgram, glm::vec4 color) const = 0;
    virtual void drawWithOffset(unsigned int shaderProgram, glm::vec4 color, float xOffset, float yOffset, float zOffset) const = 0;
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
        drawWithOffset(currentShaderProgram, color, 0, 0, 0);
    }

    void drawWithOffset(unsigned int currentShaderProgram, glm::vec4 color,
                        float xOffset, float yOffset, float zOffset) const override{
        float vertices[] = {
                a.x + xOffset, a.y + yOffset, a.z + zOffset, 0.0f, 1.0f, 0.0f,
                b.x + xOffset, b.y + yOffset, b.z + zOffset, 0.0f, 1.0f, 0.0f,
                c.x + xOffset, c.y + yOffset, c.z + zOffset, 0.0f, 1.0f, 0.0f
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
    void drawWithOffset(unsigned int currentShaderProgram, glm::vec4 color,
                        float xOffset, float yOffset, float zOffset) const override{
        t1.drawWithOffset(currentShaderProgram, color, xOffset, yOffset, zOffset);
        t2.drawWithOffset(currentShaderProgram, color, xOffset, yOffset, zOffset);
    }

};

//Fundamental shaders and rendering
extern const char* vertexShaderSource;
extern const char* fragmentShaderSource;
extern const char* backgroundVertexShader;
extern const char* backgroundFragmentShader;
extern const char* uiVertexShaderSource;
extern const char* uiFragmentShaderSource;

//Shader programs, VAOs, VBOs
extern unsigned int shaderProgram;
extern unsigned int backgroundShaderProgram;
extern unsigned int backgroundVAO, backgroundVBO;
extern unsigned int crosshairVAO, crosshairVBO, uiShaderProgram;
extern GLFWwindow* window;

unsigned int createShaderProgram();
unsigned int createBackgroundShaderProgram();
unsigned int createUIShaderProgram();
void framebuffer_size_callback(GLFWwindow* currentWindow, int width, int height);
void renderPauseMenu(unsigned int pauseShaderProgram);

//Physical class for 3D objects
class Physical {
public:
    std::vector<glm::vec3> forces;
    std::vector<std::shared_ptr<Shape>> mesh;
    glm::vec4 colour;

    bool isCollidable;

    float x = 0;
    float y = 0;
    float z = 0;

    float width;
    float height;
    float depth;

    Physical(const std::vector<std::shared_ptr<Shape>>& initMesh, glm::vec4 colour) :
            mesh(initMesh), colour(colour){
        computeBounds();

    }

    void draw(unsigned int currentShaderProgram){
        for (const auto& shape : mesh) {
            shape->drawWithOffset(currentShaderProgram, colour, x, y, z);
        }
    }

    void applyForce(glm::vec3 force){
        forces.push_back(force);
    }

    bool isColliding(Physical* collisionPhysical){
        if (collisionPhysical == this) return false;

        float minX = x - width / 2.0f;
        float maxX = x + width / 2.0f;
        float minY = y - height / 2.0f;
        float maxY = y + height / 2.0f;
        float minZ = z - depth / 2.0f;
        float maxZ = z + depth / 2.0f;

        float otherMinX = collisionPhysical->x - collisionPhysical->width / 2.0f;
        float otherMaxX = collisionPhysical->x + collisionPhysical->width / 2.0f;
        float otherMinY = collisionPhysical->y - collisionPhysical->height / 2.0f;
        float otherMaxY = collisionPhysical->y + collisionPhysical->height / 2.0f;
        float otherMinZ = collisionPhysical->z - collisionPhysical->depth / 2.0f;
        float otherMaxZ = collisionPhysical->z + collisionPhysical->depth / 2.0f;

        bool xOverlap = (minX <= otherMaxX) && (maxX >= otherMinX);
        bool yOverlap = (minY <= otherMaxY) && (maxY >= otherMinY);
        bool zOverlap = (minZ <= otherMaxZ) && (maxZ >= otherMinZ);

        return xOverlap && yOverlap && zOverlap;
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

//Vector for all global Physicals
extern std::vector<std::unique_ptr<Physical>> physicalWorld;

//Frame rendering
void engineBeginFrame();
void engineEndFrame();
void handleUI();

//Skyboxes
unsigned int initSkybox(const char* faces[6]);
void renderSkybox();

//PHYSICS

//Collision detection comparison
std::vector<Physical*> detectCollisionWithPhysical(Physical* physical);

//Physics simulation
void simulateFrame();

//Time function
float getDeltaTime();

//INPUT

//Helpful pausing/view switching variables
extern bool isPaused;
extern bool escPressedLastFrame;

//Mouse input variables
extern float lastX;
extern float lastY;
extern bool firstMouse;
extern float yaw;
extern float pitch;
extern float sensitivity;

extern float pitchLimit;
extern float yawLimit;

//Input handlers
void mouseInput(GLFWwindow* window, double xpos, double ypos);
void keyboardInput(GLFWwindow* currentWindow);

//CORE ENGINE LOOP FUNCTIONS

void startEngine();
void engineUpdate();
void drawScene();
