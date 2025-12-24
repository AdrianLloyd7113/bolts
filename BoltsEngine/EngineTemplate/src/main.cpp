#include "../include/glad/glad.h"
#include "../include/GLFW/glfw3.h"
#include "../include/glm/glm.hpp"
#include "../include/glm/gtc/matrix_transform.hpp"
#include "engine/bolts.h"
#include <vector>
#include <iostream>

//Sets master variables for the engine; can be used for setting other
//critical values
void setPrimaryVars(){
    //Allows a skybox, or not;
    //Skybox source textures must be found under /skybox/ in project root
    skyboxEnabled = false;

    //Limits on camera movement (change these to restrict in-game camera yaw/pitch)
    pitchLimit = -1.0;
    yawLimit = -1.0;
}

//Game loop
void update() {
    //Core engine update function
    engineUpdate();

    //TODO: Gameplay/player functionality goes here...

    //Frame rendering
    engineBeginFrame();

    if (!isPaused) {
        //TODO: Game environment logic and rendering goes here...
    } else {
        renderPauseMenu(shaderProgram);
    }

    //Engine UI and frame end handling
    handleUI();
    engineEndFrame();
}


int main() {
    setPrimaryVars();
    startEngine();

    while (gameActive){
        update();
    }

    return 0;
}