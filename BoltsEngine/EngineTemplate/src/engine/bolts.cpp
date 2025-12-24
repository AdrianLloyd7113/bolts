#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "bolts.h"
#include <vector>
#include <iostream>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include "bolts.h"

std::vector<Rectangle> surfaces;
bool escPressedLastFrame = false;

float pitchLimit;
float yawLimit;

unsigned int shaderProgram;
unsigned int backgroundShaderProgram;
unsigned int backgroundVAO, backgroundVBO;
unsigned int crosshairVAO, crosshairVBO, uiShaderProgram;
GLFWwindow* window;

bool skyboxEnabled;
bool gameActive;

unsigned int createShaderProgram() {
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);

    unsigned int returnShaderProgram = glCreateProgram();
    glAttachShader(returnShaderProgram, vertexShader);
    glAttachShader(returnShaderProgram, fragmentShader);
    glLinkProgram(returnShaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return returnShaderProgram;
}

//mouse movement
void mouseInput(GLFWwindow* currentWindow, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xOffset = xpos - lastX;
    float yOffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    xOffset *= sensitivity;
    yOffset *= sensitivity;

    yaw   += xOffset;
    pitch += yOffset;

    //pitch and yaw limits for more restrictive cameras;
    //set to -1 to ignore

    if (pitchLimit != -1.0){
        if (pitch > pitchLimit)
            pitch = pitchLimit;
        if (pitch < -pitchLimit)
            pitch = -pitchLimit;
    }

    if (yawLimit != -1.0){
        if (yaw > yawLimit)
            yaw = yawLimit;
        if (yaw < -yawLimit)
            yaw = -yawLimit;
    }

    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(direction);
}

//resize
void framebuffer_size_callback(GLFWwindow* currentWindow, int width, int height) {
    glViewport(0, 0, width, height);
}

//background shader program
unsigned int createBackgroundShaderProgram() {
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &backgroundVertexShader, nullptr);
    glCompileShader(vertexShader);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &backgroundFragmentShader, nullptr);
    glCompileShader(fragmentShader);

    unsigned int bgShaderProgram = glCreateProgram();
    glAttachShader(bgShaderProgram, vertexShader);
    glAttachShader(bgShaderProgram, fragmentShader);
    glLinkProgram(bgShaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return bgShaderProgram;
}

unsigned int createUIShaderProgram() {
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &uiVertexShaderSource, nullptr);
    glCompileShader(vertexShader);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &uiFragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);

    unsigned int returnShaderProgram = glCreateProgram();
    glAttachShader(returnShaderProgram, vertexShader);
    glAttachShader(returnShaderProgram, fragmentShader);
    glLinkProgram(returnShaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return returnShaderProgram;
}

void drawTriangle(Triangle toDraw, unsigned int currentShaderProgram, glm::vec4 color) {
    float vertices[] = {
            toDraw.a.x, toDraw.a.y, toDraw.a.z, 0.0f, 1.0f, 0.0f,
            toDraw.b.x, toDraw.b.y, toDraw.b.z, 0.0f, 1.0f, 0.0f,
            toDraw.c.x, toDraw.c.y, toDraw.c.z, 0.0f, 1.0f, 0.0f
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

void drawRectangle(Rectangle toDraw, unsigned int currentShaderProgram, glm::vec4 colour){
    drawTriangle(toDraw.a, currentShaderProgram, colour);
    drawTriangle(toDraw.b, currentShaderProgram, colour);
}

void renderPauseMenu(unsigned int pauseShaderProgram) {
    // Simple translucent rectangle in front of everything
    glm::vec4 pauseOverlayColor(0.0f, 0.0f, 0.0f, 0.5f); // translucent black

    // Draw a 2D quad in NDC [-1, 1] space over screen
    float overlayVertices[] = {
            -0.5f,  0.3f, 0.0f,
            -0.5f, -0.3f, 0.0f,
            0.5f, -0.3f, 0.0f,

            -0.5f,  0.3f, 0.0f,
            0.5f, -0.3f, 0.0f,
            0.5f,  0.3f, 0.0f,
    };

    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(overlayVertices), overlayVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glDisable(GL_DEPTH_TEST); // Pause screen should overlay everything
    glUseProgram(pauseShaderProgram);
    int colorLoc = glGetUniformLocation(pauseShaderProgram, "uColor");
    glUniform4f(colorLoc, pauseOverlayColor.r, pauseOverlayColor.g, pauseOverlayColor.b, pauseOverlayColor.a);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glEnable(GL_DEPTH_TEST); // Re-enable for rest of rendering

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

//engine startup
void startEngine(){
    gameActive = true;

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        gameActive = false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    //generate window
    GLFWmonitor* primary = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primary);
    int screenWidth = mode->width;
    int screenHeight = mode->height;
    window = glfwCreateWindow(screenWidth, screenHeight, "Asteroid Game", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        gameActive = false;
    }
    glfwMakeContextCurrent(window);

    //set up mouse control
    glfwSetCursorPosCallback(window, mouseInput);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    //allow resize
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    //glad init
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        gameActive = false;
    }

    glDisable(GL_CULL_FACE);
    shaderProgram = createShaderProgram();

    //background setup
    float backgroundVertices[] = {
            -1.0f,  1.0f, 0.0f,
            -1.0f, -1.0f, 0.0f,
            1.0f, -1.0f, 0.0f,

            -1.0f,  1.0f, 0.0f,
            1.0f, -1.0f, 0.0f,
            1.0f,  1.0f, 0.0f
    };

    backgroundShaderProgram = createBackgroundShaderProgram();

    glGenVertexArrays(1, &backgroundVAO);
    glGenBuffers(1, &backgroundVBO);
    glBindVertexArray(backgroundVAO);

    glBindBuffer(GL_ARRAY_BUFFER, backgroundVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(backgroundVertices), backgroundVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    const float crossSize = 0.025f; // previously 0.05f
    const float lineWidth = 0.0025f; // previously 0.005f

    float crosshairVertices[] = {
            // vertical bar
            -lineWidth,  crossSize, 0.0f,
            -lineWidth, -crossSize, 0.0f,
            lineWidth, -crossSize, 0.0f,

            -lineWidth,  crossSize, 0.0f,
            lineWidth, -crossSize, 0.0f,
            lineWidth,  crossSize, 0.0f,

            // horizontal bar
            -crossSize,  lineWidth, 0.0f,
            -crossSize, -lineWidth, 0.0f,
            crossSize, -lineWidth, 0.0f,

            -crossSize,  lineWidth, 0.0f,
            crossSize, -lineWidth, 0.0f,
            crossSize,  lineWidth, 0.0f
    };


    glGenVertexArrays(1, &crosshairVAO);
    glGenBuffers(1, &crosshairVBO);
    glBindVertexArray(crosshairVAO);

    glBindBuffer(GL_ARRAY_BUFFER, crosshairVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(crosshairVertices), crosshairVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    uiShaderProgram = createUIShaderProgram();

    if (skyboxEnabled){
        const char* skyboxFaces[6] = {"skybox/right.jpg",
                                      "skybox/left.jpg",
                                      "skybox/top.jpg",
                                      "skybox/bottom.jpg",
                                      "skybox/front.jpg",
                                      "skybox/back.jpg"};
        initSkybox(skyboxFaces);
    }
}

//core engine mechanics
void engineUpdate() {
    if (glfwWindowShouldClose(window)) {
        gameActive = false;
        return;
    }

    surfaces.clear();

    keyboardInput(window);
    glfwPollEvents();
}

//rendering
void engineBeginFrame(){
    const glm::vec4 bg(0, 0, 0.431, 1);
    glClearColor(bg.r, bg.g, bg.b, bg.a);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (skyboxEnabled) renderSkybox();
    else{
        glDepthMask(GL_FALSE);
        glUseProgram(backgroundShaderProgram);
        glBindVertexArray(backgroundVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glDepthMask(GL_TRUE);
    }

    glUseProgram(shaderProgram);

    glm::mat4 projection = glm::perspective(
            glm::radians(45.0f),
            (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT,
            0.1f,
            500.0f
    );

    glm::mat4 view = glm::lookAt(
            cameraPos,
            cameraPos + cameraFront,
            cameraUp
    );

    glUniformMatrix4fv(
            glGetUniformLocation(shaderProgram, "view"),
            1, GL_FALSE, &view[0][0]
    );
    glUniformMatrix4fv(
            glGetUniformLocation(shaderProgram, "projection"),
            1, GL_FALSE, &projection[0][0]
    );
}
void engineEndFrame(){
    glfwSwapBuffers(window);
}

//UI handler
void handleUI(){
    glUseProgram(uiShaderProgram);
    glBindVertexArray(crosshairVAO);
    glDisable(GL_DEPTH_TEST);

    int colorLoc = glGetUniformLocation(uiShaderProgram, "uColor");
    glUniform4f(colorLoc, 1, 1, 1, 1);

    glDrawArrays(GL_TRIANGLES, 0, 12);
    glEnable(GL_DEPTH_TEST);
}


bool isPaused = false;
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 FragPos;
out vec3 Normal;

uniform mat4 projection;
uniform mat4 view;

void main() {
    FragPos = aPos;
    Normal = aNormal;
    gl_Position = projection * view * vec4(aPos, 1.0);
}
)";
const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec4 uColor;
uniform vec3 lightPos;
uniform vec3 viewPos; // camera position

void main() {
    // Ambient
    vec3 ambient = 0.2 * uColor.rgb;

    // Diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * uColor.rgb;

    // Specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = spec * vec3(1.0);

    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
}
)";
const char* backgroundVertexShader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
out vec2 uv;

void main() {
    uv = aPos.xy * 0.5 + 0.5; // convert from [-1,1] to [0,1]
    gl_Position = vec4(aPos, 1.0);
}
)";
const char* backgroundFragmentShader = R"(
#version 330 core
out vec4 FragColor;
in vec2 uv;

void main() {
    vec3 topColor = vec3(0.0, 0.1, 0.3); // deep space blue
    vec3 bottomColor = vec3(0.0, 0.0, 0.0); // black
    vec3 color = mix(bottomColor, topColor, uv.y);
    FragColor = vec4(color, 1.0);
}
)";
const char* uiVertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
void main() {
    gl_Position = vec4(aPos, 1.0);
}
)";
const char* uiFragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
uniform vec4 uColor;
void main() {
    FragColor = uColor;
}
)";
//camera variables
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float lastX = WINDOW_WIDTH / 2.0f;
float lastY = WINDOW_HEIGHT / 2.0f;
bool firstMouse = true;
float yaw = -90.0f;
float pitch = 0.0f;
float sensitivity = 0.1f;

//Skyboxes

// Add these to bolts.h
unsigned int skyboxVAO, skyboxVBO;
unsigned int skyboxShaderProgram;
unsigned int cubemapTexture;

// Skybox shader sources
const char* skyboxVertexShader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 projection;
uniform mat4 view;

void main() {
    TexCoords = aPos;
    vec4 pos = projection * mat4(mat3(view)) * vec4(aPos, 1.0);
    gl_Position = pos.xyww; // Ensures skybox is always at max depth
}
)";

const char* skyboxFragmentShader = R"(
#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube skybox;

void main() {
    FragColor = texture(skybox, TexCoords);
}
)";

unsigned int initSkybox(const char* faces[6]) {
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &skyboxVertexShader, nullptr);
    glCompileShader(vertexShader);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &skyboxFragmentShader, nullptr);
    glCompileShader(fragmentShader);

    skyboxShaderProgram = glCreateProgram();
    glAttachShader(skyboxShaderProgram, vertexShader);
    glAttachShader(skyboxShaderProgram, fragmentShader);
    glLinkProgram(skyboxShaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    float skyboxVertices[] = {
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f,  1.0f
    };

    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    glGenTextures(1, &cubemapTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);

    int successCount = 0;
    for (unsigned int i = 0; i < 6; i++) {
        int width, height, nrChannels;
        stbi_set_flip_vertically_on_load(false);
        unsigned char *data = stbi_load(faces[i], &width, &height, &nrChannels, 0);
        if (data) {
            GLenum format = (nrChannels == 3) ? GL_RGB : GL_RGBA;
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                         0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
            std::cout << "Loaded skybox face " << i << ": " << faces[i]
                      << " (" << width << "x" << height << ", " << nrChannels << " channels)" << std::endl;
            successCount++;
        } else {
            std::cerr << "Failed to load skybox texture " << i << ": " << faces[i] << std::endl;
            std::cerr << "Error: " << stbi_failure_reason() << std::endl;

            unsigned char fallbackData[12] = {
                    255, 0, 0, 255,
                    0, 255, 0, 255,
                    0, 0, 255, 255
            };
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                         0, GL_RGB, 2, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, fallbackData);
        }

        std::cout << faces[i]
                  << " " << width << "x" << height
                  << " channels=" << nrChannels << std::endl;
    }

    std::cout << "Successfully loaded " << successCount << "/6 skybox faces" << std::endl;

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    return cubemapTexture;
}

void renderSkybox() {
    glDepthFunc(GL_LEQUAL);
    glUseProgram(skyboxShaderProgram);

    GLint isLinked = 0;
    glGetProgramiv(skyboxShaderProgram, GL_LINK_STATUS, &isLinked);
    if (isLinked == GL_FALSE) {
        std::cerr << "Skybox shader not linked!" << std::endl;
    }

    GLint samplerLoc = glGetUniformLocation(skyboxShaderProgram, "skybox");
    glUniform1i(samplerLoc, 0);

    glm::mat4 view = glm::mat4(glm::mat3(glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp)));
    glm::mat4 projection = glm::perspective(glm::radians(45.0f),
                                            (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT,
                                            0.1f, 500.0f);

    GLint viewLoc = glGetUniformLocation(skyboxShaderProgram, "view");
    GLint projLoc = glGetUniformLocation(skyboxShaderProgram, "projection");

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, &projection[0][0]);

    glBindVertexArray(skyboxVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);

    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glDepthFunc(GL_LESS);
}
