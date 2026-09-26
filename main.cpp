// =========================================================================
// Computer Graphics Laboratory - CSE 444
// 3D Classroom Project
// 
// Key Requirements Implemented:
// 1. 3D Transformation: Hierarchical modeling (scale, rotate, translate) for all
//    classroom objects (desks, chairs, podium, blackboard, fan, door, windows, clock).
//    Interactive model transformation (X, Y, Z rotation, translation, scaling).
// 2. Viewing Transformation: Camera system supporting multiple preset angles:
//    - Back View (Main view down the central aisle)
//    - Side View (Profile view of desks, fan, and windows)
//    - Top View (Bird's-eye view of classroom arrangement)
//    - Teacher View (Front view looking at students from the podium)
//    - Free Look (First-person WASD + Mouse exploration)
// 3. Moving Object: Continuously rotating Ceiling Fan with speed & toggle controls.
// 4. Two Kinds of Light:
//    - Point Lights (4 ceiling fixtures providing soft room illumination)
//    - Spotlight (Focused directional cone light illuminating the blackboard)
// 5. Rich Color Palette: Distinct materials and colors for all surfaces.
// =========================================================================

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"
#include "camera.h"
#include "basic_camera.h"
#include "pointLight.h"
#include "spotLight.h"

#include <iostream>
#include <algorithm>

using namespace std;

// -------------------------------------------------------------------------
// Function Declarations
// -------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

// Drawing Helper Functions (All models composed hierarchically of unit cubes)
void drawCube(unsigned int& cubeVAO, Shader& lightingShader, glm::mat4 model, 
              float r, float g, float b, float spec = 0.3f, float shininess = 32.0f);
void drawFloor(unsigned int& cubeVAO, Shader& lightingShader, glm::mat4 roomBase);
void drawWallsAndCeiling(unsigned int& cubeVAO, Shader& lightingShader, glm::mat4 roomBase);
void drawStudentDesk(unsigned int& cubeVAO, Shader& lightingShader, glm::mat4 deskBase, bool hasRobotics = false);
void drawStudentChair(unsigned int& cubeVAO, Shader& lightingShader, glm::mat4 chairBase);
void drawTeacherPodium(unsigned int& cubeVAO, Shader& lightingShader, Shader& ourShader, glm::mat4 podiumBase);
void drawBlackboard(unsigned int& cubeVAO, Shader& lightingShader, Shader& ourShader, glm::mat4 boardBase, bool isWhiteboard);
void drawWallClock(unsigned int& cubeVAO, Shader& lightingShader, glm::mat4 clockBase, float clockSecondAngle);
void drawCeilingFan(unsigned int& cubeVAO, Shader& lightingShader, glm::mat4 fanBase, float fanAngle);
void drawWindow(unsigned int& cubeVAO, Shader& lightingShader, Shader& ourShader, glm::mat4 windowBase);
void drawDoor(unsigned int& cubeVAO, Shader& lightingShader, Shader& ourShader, glm::mat4 doorBase, float doorAngle);
void drawCeilingLightFixture(unsigned int& cubeVAO, Shader& lightingShader, Shader& ourShader, 
                             glm::mat4 lightBase, bool isLightOn);
void drawSpotlightFixture(unsigned int& cubeVAO, Shader& lightingShader, Shader& ourShader, 
                          glm::mat4 fixtureBase, bool isSpotlightOn);
void drawRobot(unsigned int& cubeVAO, Shader& lightingShader, Shader& ourShader, 
               glm::mat4 robotBase, float waveAngle);
void drawLaptop(unsigned int& cubeVAO, Shader& lightingShader, Shader& ourShader, glm::mat4 deskMat, 
                float posX = -0.28f, float posZ = 0.02f, float deskH = 0.72f, float rotY = 0.0f, float scaleLap = 1.0f);
void drawEmbeddedKitTable1(unsigned int& cubeVAO, Shader& lightingShader, Shader& ourShader, glm::mat4 deskMat);
void drawRoboticArmTable2(unsigned int& cubeVAO, Shader& lightingShader, Shader& ourShader, glm::mat4 deskMat);
void drawMobileRoverTable3(unsigned int& cubeVAO, Shader& lightingShader, Shader& ourShader, glm::mat4 deskMat);
void drawSpiderBotTable4(unsigned int& cubeVAO, Shader& lightingShader, Shader& ourShader, glm::mat4 deskMat);

// -------------------------------------------------------------------------
// Settings & Window Constants
// -------------------------------------------------------------------------
const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 800;

// -------------------------------------------------------------------------
// Interactive 3D Model Transformation State
// -------------------------------------------------------------------------
float rotateAngle_X = 0.0f;
float rotateAngle_Y = 0.0f;
float rotateAngle_Z = 0.0f;
float rotateAxis_X = 0.0f;
float rotateAxis_Y = 1.0f;
float rotateAxis_Z = 0.0f;

float translate_X = 0.0f;
float translate_Y = 0.0f;
float translate_Z = 0.0f;

float scale_X = 1.0f;
float scale_Y = 1.0f;
float scale_Z = 1.0f;

// -------------------------------------------------------------------------
// Camera & Viewing Transformations
// -------------------------------------------------------------------------
Camera camera(glm::vec3(0.0f, 2.10f, 4.8f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// The 4 Core Camera Views matching the 4 project screenshots (Image/front.png, back.png, left.png, right.png)
enum CameraViewMode {
    VIEW_FRONT = 0,   // Key 1: Front View (Image/front.png - looking at blackboard & teacher podium)
    VIEW_BACK = 1,    // Key 2: Back View (Image/back.png - looking from podium at student desks & entrance)
    VIEW_LEFT = 2,    // Key 3: Left View (Image/left.png - looking at windows & outdoor campus scenery)
    VIEW_RIGHT = 3    // Key 4: Right View (Image/right.png - looking at door, hallway corridor & robot)
};
CameraViewMode currentView = VIEW_FRONT;

void setCameraPreset(CameraViewMode mode) {
    currentView = mode;
    switch (mode) {
        case VIEW_FRONT: // 1. Front View (Image/front.png)
            camera.Position = glm::vec3(0.0f, 2.10f, 4.8f);
            camera.Yaw = -90.0f;
            camera.Pitch = 0.0f;
            camera.Zoom = 45.0f;
            camera.updateCameraVectors();
            cout << "[Camera View 1] FRONT VIEW (Looking at blackboard & teacher podium)" << endl;
            break;
        case VIEW_BACK: // 2. Back View (Image/back.png)
            camera.Position = glm::vec3(0.0f, 1.80f, -4.8f);
            camera.Yaw = 90.0f;
            camera.Pitch = 0.0f;
            camera.Zoom = 45.0f;
            camera.updateCameraVectors();
            cout << "[Camera View 2] BACK VIEW (From teacher podium looking at student desks & entrance)" << endl;
            break;
        case VIEW_LEFT: // 3. Left View (Image/left.png)
            camera.Position = glm::vec3(4.6f, 1.90f, -0.8f);
            camera.Yaw = 178.0f;
            camera.Pitch = -5.0f;
            camera.Zoom = 45.0f;
            camera.updateCameraVectors();
            cout << "[Camera View 3] LEFT VIEW (Looking at windows & outdoor campus scenery)" << endl;
            break;
        case VIEW_RIGHT: // 4. Right View (Image/right.png)
            camera.Position = glm::vec3(-4.6f, 2.00f, -0.8f);
            camera.Yaw = -2.0f;
            camera.Pitch = -5.0f;
            camera.Zoom = 45.0f;
            camera.updateCameraVectors();
            cout << "[Camera View 4] RIGHT VIEW (Looking at door, hallway corridor & waving robot)" << endl;
            break;
    }
}

// -------------------------------------------------------------------------
// Requirement 3: Moving Objects (Ceiling Fan, Clock Hand & Interactive Door)
// -------------------------------------------------------------------------

// Moving Object 1: Animated Rotating Ceiling Fan
float fanAngle = 0.0f;
float fanSpeed = 220.0f; // rotation speed in degrees per second
bool isFanOn = true;

// Moving Object 2: Animated Wall Clock Second Hand (Rotates CLOCKWISE around Z)
float clockSecondAngle = 0.0f;
float clockSpeed = 6.0f; // Real-world speed: 360 degrees in 60 seconds = 6.0 deg/sec

// Moving Object 3: Interactive Classroom Door (Open / Close smoothly)
bool isDoorOpen = false;
float doorAngle = 0.0f;
float targetDoorAngle = 0.0f;

// Moving Object 4: Animated 3D Classroom Robot Waving Hand ("Bye-Bye" Motion)
float robotWaveAngle = 0.0f;

// -------------------------------------------------------------------------
// Requirement 4: Lighting (Point Lights & Spotlight)
// -------------------------------------------------------------------------

// Four Point Light positions on the ceiling for even illumination
glm::vec3 pointLightPositions[] = {
    glm::vec3(-2.3f, 3.85f, -3.2f),
    glm::vec3( 2.3f, 3.85f, -3.2f),
    glm::vec3(-2.3f, 3.85f,  1.2f),
    glm::vec3( 2.3f, 3.85f,  1.2f)
};

PointLight pointlight1(
    pointLightPositions[0].x, pointLightPositions[0].y, pointLightPositions[0].z,
    0.15f, 0.15f, 0.13f,   // ambient
    0.85f, 0.82f, 0.75f,   // diffuse (warm classroom light)
    0.50f, 0.50f, 0.50f,   // specular
    1.0f, 0.09f, 0.032f,   // attenuation factors (constant, linear, quadratic)
    1
);

PointLight pointlight2(
    pointLightPositions[1].x, pointLightPositions[1].y, pointLightPositions[1].z,
    0.15f, 0.15f, 0.13f,
    0.85f, 0.82f, 0.75f,
    0.50f, 0.50f, 0.50f,
    1.0f, 0.09f, 0.032f,
    2
);

PointLight pointlight3(
    pointLightPositions[2].x, pointLightPositions[2].y, pointLightPositions[2].z,
    0.15f, 0.15f, 0.13f,
    0.85f, 0.82f, 0.75f,
    0.50f, 0.50f, 0.50f,
    1.0f, 0.09f, 0.032f,
    3
);

PointLight pointlight4(
    pointLightPositions[3].x, pointLightPositions[3].y, pointLightPositions[3].z,
    0.15f, 0.15f, 0.13f,
    0.85f, 0.82f, 0.75f,
    0.50f, 0.50f, 0.50f,
    1.0f, 0.09f, 0.032f,
    4
);

bool pointLightOn = true;
bool pointLight1On = true;
bool pointLight2On = true;
bool pointLight3On = true;
bool pointLight4On = true;

// Classroom Ceiling Multimedia Projector (Spotlight): Mounted at ceiling, pointed directly towards the front board
SpotLight blackboardSpotlight(
    0.0f, 3.85f, -3.2f,        // position
    0.0f, -0.45f, -0.89f,      // direction vector towards the blackboard center
    18.0f, 26.0f,              // inner and outer cutoff angles
    0.05f, 0.05f, 0.05f,       // ambient
    1.0f, 0.98f, 0.92f,        // diffuse (bright crisp projector beam)
    0.9f, 0.9f, 0.9f,          // specular
    1.0f, 0.07f, 0.017f,       // attenuation
    1
);
bool isProjectorOn = false;    // Classroom Projector & Whiteboard Screen toggle

// Timing variables
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// =========================================================================
// Main Entry Point
// =========================================================================
int main()
{
    // 1. Initialize and configure GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // 2. Create GLFW Window
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, 
        "CSE 444: 3D Robotics Classroom Model with Shading, Animation & Multi-Light", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // 3. Load OpenGL function pointers via GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Configure global OpenGL depth state
    glEnable(GL_DEPTH_TEST);

    // 4. Build and compile Shader programs
    // Uses Phong shading for smooth specular highlights and spotlight cones
    Shader lightingShader("vertexShaderForPhongShading.vs", "fragmentShaderForPhongShading.fs");
    Shader ourShader("vertexShader.vs", "fragmentShader.fs");

    // 5. Unit Cube Vertex Data (Positions & Surface Normals)
    float cube_vertices[] = {
        // positions          // normals
        // Back Face
        0.0f, 0.0f, 0.0f,     0.0f,  0.0f, -1.0f,
        1.0f, 0.0f, 0.0f,     0.0f,  0.0f, -1.0f,
        1.0f, 1.0f, 0.0f,     0.0f,  0.0f, -1.0f,
        0.0f, 1.0f, 0.0f,     0.0f,  0.0f, -1.0f,

        // Right Face
        1.0f, 0.0f, 0.0f,     1.0f,  0.0f,  0.0f,
        1.0f, 1.0f, 0.0f,     1.0f,  0.0f,  0.0f,
        1.0f, 0.0f, 1.0f,     1.0f,  0.0f,  0.0f,
        1.0f, 1.0f, 1.0f,     1.0f,  0.0f,  0.0f,

        // Front Face
        0.0f, 0.0f, 1.0f,     0.0f,  0.0f,  1.0f,
        1.0f, 0.0f, 1.0f,     0.0f,  0.0f,  1.0f,
        1.0f, 1.0f, 1.0f,     0.0f,  0.0f,  1.0f,
        0.0f, 1.0f, 1.0f,     0.0f,  0.0f,  1.0f,

        // Left Face
        0.0f, 0.0f, 1.0f,    -1.0f,  0.0f,  0.0f,
        0.0f, 1.0f, 1.0f,    -1.0f,  0.0f,  0.0f,
        0.0f, 1.0f, 0.0f,    -1.0f,  0.0f,  0.0f,
        0.0f, 0.0f, 0.0f,    -1.0f,  0.0f,  0.0f,

        // Top Face
        1.0f, 1.0f, 1.0f,     0.0f,  1.0f,  0.0f,
        1.0f, 1.0f, 0.0f,     0.0f,  1.0f,  0.0f,
        0.0f, 1.0f, 0.0f,     0.0f,  1.0f,  0.0f,
        0.0f, 1.0f, 1.0f,     0.0f,  1.0f,  0.0f,

        // Bottom Face
        0.0f, 0.0f, 0.0f,     0.0f, -1.0f,  0.0f,
        1.0f, 0.0f, 0.0f,     0.0f, -1.0f,  0.0f,
        1.0f, 0.0f, 1.0f,     0.0f, -1.0f,  0.0f,
        0.0f, 0.0f, 1.0f,     0.0f, -1.0f,  0.0f
    };

    unsigned int cube_indices[] = {
        0, 3, 2,  2, 1, 0,       // back
        4, 5, 7,  7, 6, 4,       // right
        8, 9, 10, 10, 11, 8,     // front
        12, 13, 14, 14, 15, 12,  // left
        16, 17, 18, 18, 19, 16,  // top
        20, 21, 22, 22, 23, 20   // bottom
    };

    unsigned int cubeVAO, cubeVBO, cubeEBO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glGenBuffers(1, &cubeEBO);

    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_indices), cube_indices, GL_STATIC_DRAW);

    // Position attribute (layout location = 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Normal attribute (layout location = 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Initial camera view (Key 1: Front View)
    setCameraPreset(VIEW_FRONT);

    cout << "==========================================================" << endl;
    cout << "  3D ROBOTICS CLASSROOM - INTERACTION & CONTROL GUIDE     " << endl;
    cout << "==========================================================" << endl;
    cout << " The 4 Core Views:" << endl;
    cout << "   [1] : View 1 - FRONT VIEW (Blackboard & Teacher Podium)" << endl;
    cout << "   [2] : View 2 - BACK VIEW  (Student Desks & Entrance from Podium)" << endl;
    cout << "   [3] : View 3 - LEFT VIEW  (Windows & Outdoor Campus Scenery)" << endl;
    cout << "   [4] : View 4 - RIGHT VIEW (Door, Hallway Corridor & Robot)" << endl;
    cout << "   [R] : Master Reset to View 1" << endl;
    cout << endl;
    cout << " Camera Navigation:" << endl;
    cout << "   [Up / Down Arrow]    : Move Forward / Backward" << endl;
    cout << "   [Left / Right Arrow] : Turn Left / Turn Right (Smooth Look)" << endl;
    cout << "   [Shift + Up/Down]    : Move Camera Height Up / Down" << endl;
    cout << "   [Mouse Drag]         : Free Look around" << endl;
    cout << endl;
    cout << " Interactive 3D Room Transformations:" << endl;
    cout << "   [T + Up / Down]      : Translate Room Up / Down (+Y / -Y)" << endl;
    cout << "   [T + Right / Left]   : Translate Room Right / Left (+X / -X)" << endl;
    cout << "   [T + PgUp / PgDn]    : Translate Room Closer / Farther (+Z / -Z)" << endl;
    cout << "   [X], [Y], [Z]        : Rotate Room on Axis (Hold Shift to reverse)" << endl;
    cout << "   [M + Up / Down]      : Scale Room Uniformly (Bigger / Smaller)" << endl;
    cout << endl;
    cout << " Moving Objects & Interactive Features:" << endl;
    cout << "   [D]                  : Open / Close Classroom Door" << endl;
    cout << "   [F]                  : Toggle Ceiling Fan On / Off" << endl;
    cout << "   [+] / [-]            : Increase / Decrease Fan Speed" << endl;
    cout << "   [5, 6, 7, 8]         : Toggle Point Lights 1, 2, 3, 4 Individually" << endl;
    cout << "   [L]                  : Toggle All Ceiling Point Lights (Master Switch)" << endl;
    cout << "   [P] (or [S])         : Toggle Projector & Switch Board to Whiteboard Screen" << endl;
    cout << "   [R]                  : Reset Camera, Room Transformations & Lights" << endl;
    cout << "==========================================================" << endl;

    // Initialize Projector to standby (OFF) so classroom starts in classic green chalkboard mode
    isProjectorOn = false;
    blackboardSpotlight.turnOff();

    // ---------------------------------------------------------------------
    // Main Render Loop
    // ---------------------------------------------------------------------
    while (!glfwWindowShouldClose(window))
    {
        // Per-frame timing calculation
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Process user input
        processInput(window);

        // 1. Update Moving Object: Ceiling Fan Rotation
        if (isFanOn) {
            fanAngle += fanSpeed * deltaTime;
            if (fanAngle >= 360.0f) {
                fanAngle -= 360.0f;
            }
        }

        // 2. Update Moving Object: Wall Clock Second Hand (Rotates CLOCKWISE around Z)
        clockSecondAngle -= clockSpeed * deltaTime;
        if (clockSecondAngle <= -360.0f) {
            clockSecondAngle += 360.0f;
        }

        // 3. Update Moving Object: Door Open / Close Smooth Transition
        targetDoorAngle = isDoorOpen ? 85.0f : 0.0f;
        doorAngle += (targetDoorAngle - doorAngle) * 5.0f * deltaTime;

        // 4. Update Moving Object: 3D Robot Hand Waving (Continuous "Bye-Bye" Motion)
        robotWaveAngle = sin(currentFrame * 5.5f) * 28.0f;

        // Render pass setup
        glClearColor(0.08f, 0.08f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Activate lighting shader and configure camera/eye position
        lightingShader.use();
        lightingShader.setVec3("viewPos", camera.Position);

        // Set up Point Lights in the shader
        pointlight1.setUpPointLight(lightingShader);
        pointlight2.setUpPointLight(lightingShader);
        pointlight3.setUpPointLight(lightingShader);
        pointlight4.setUpPointLight(lightingShader);

        // Set up Spotlight in the shader
        blackboardSpotlight.setUpSpotLight(lightingShader);

        // Viewing Transformation (Projection and View Matrices)
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), 
            (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        lightingShader.use();
        lightingShader.setMat4("projection", projection);

        glm::mat4 view = camera.GetViewMatrix();
        lightingShader.setMat4("view", view);

        // Also configure ourShader matrices for emissive/unlit components (sky, LEDs, lights)
        ourShader.use();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        // Global Classroom 3D Modeling Transformation
        // (Allows interactive rotation, translation, and scaling of the whole room)
        glm::mat4 identityMatrix = glm::mat4(1.0f);
        glm::mat4 translateMatrix = glm::translate(identityMatrix, glm::vec3(translate_X, translate_Y, translate_Z));
        glm::mat4 rotateXMatrix = glm::rotate(identityMatrix, glm::radians(rotateAngle_X), glm::vec3(1.0f, 0.0f, 0.0f));
        glm::mat4 rotateYMatrix = glm::rotate(identityMatrix, glm::radians(rotateAngle_Y), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 rotateZMatrix = glm::rotate(identityMatrix, glm::radians(rotateAngle_Z), glm::vec3(0.0f, 0.0f, 1.0f));
        glm::mat4 scaleMatrix   = glm::scale(identityMatrix, glm::vec3(scale_X, scale_Y, scale_Z));

        glm::mat4 roomBase = translateMatrix * rotateXMatrix * rotateYMatrix * rotateZMatrix * scaleMatrix;

        // -----------------------------------------------------------------
        // Draw Classroom Components
        // -----------------------------------------------------------------

        // 1. Floor, Walls, and Ceiling Shell
        drawFloor(cubeVAO, lightingShader, roomBase);
        drawWallsAndCeiling(cubeVAO, lightingShader, roomBase);

        // 2. Blackboard & Wall Clock on the front wall (with clockwise second hand)
        drawBlackboard(cubeVAO, lightingShader, ourShader, roomBase, isProjectorOn);
        drawWallClock(cubeVAO, lightingShader, roomBase, clockSecondAngle);

        // 3. Windows (Left Wall with Outdoor View) & Door (Right Wall with Open/Close, Color Shift & Hallway)
        drawWindow(cubeVAO, lightingShader, ourShader, roomBase);
        drawDoor(cubeVAO, lightingShader, ourShader, roomBase, doorAngle);

        // 4. Teacher's Podium & Laptop (Front of classroom)
        glm::mat4 podiumModel = glm::translate(roomBase, glm::vec3(0.0f, 0.0f, -4.2f));
        drawTeacherPodium(cubeVAO, lightingShader, ourShader, podiumModel);

        // 5. Student Desks & Chairs (2 Columns x 4 Rows = 8 sets with spacious aisles)
        float colX[2] = { -2.3f, 2.3f };
        float rowZ[4] = { -2.2f, -0.6f, 1.0f, 2.6f };

        for (int c = 0; c < 2; ++c) {
            for (int r = 0; r < 4; ++r) {
                // Determine if this desk is one of the 4 embedded robotics workstations:
                bool hasRobotics = ((c == 0 && (r == 0 || r == 1)) || (c == 1 && (r == 1 || r == 2)));

                // Desk (hasRobotics removes previous notebook tab from the desk surface)
                glm::mat4 deskMat = glm::translate(roomBase, glm::vec3(colX[c], 0.0f, rowZ[r]));
                drawStudentDesk(cubeVAO, lightingShader, deskMat, hasRobotics);

                // Matching Chair behind the desk (0.46m offset gives realistic tucked-in position and 66cm aisle clearance)
                glm::mat4 chairMat = glm::translate(roomBase, glm::vec3(colX[c], 0.0f, rowZ[r] + 0.46f));
                drawStudentChair(cubeVAO, lightingShader, chairMat);

                // Embedded Classroom: If table has robotics, place a student laptop + components (no overlap!)
                if (hasRobotics) {
                    drawLaptop(cubeVAO, lightingShader, ourShader, deskMat, -0.28f, 0.02f);
                }

                if (c == 0 && r == 0) {
                    // Left Front Table (1st): Microcontroller Kit (Arduino, Breadboard, Ultrasonic & Servo)
                    drawEmbeddedKitTable1(cubeVAO, lightingShader, ourShader, deskMat);
                } else if (c == 0 && r == 1) {
                    // Left 2nd Table: Articulated 3-DOF Robotic Arm with Gripper & Digital Multimeter
                    drawRoboticArmTable2(cubeVAO, lightingShader, ourShader, deskMat);
                } else if (c == 1 && r == 1) {
                    // Right 2nd Table: Robotic Quadruped Spider Bot & IoT Board with OLED Display
                    drawSpiderBotTable4(cubeVAO, lightingShader, ourShader, deskMat);
                } else if (c == 1 && r == 2) {
                    // Right 3rd Table: Autonomous 2-Wheel Mobile Robotics Rover & Parts Organizer
                    drawMobileRoverTable3(cubeVAO, lightingShader, ourShader, deskMat);
                }
            }
        }

        // 6. Moving Object: Rotating Ceiling Fan
        glm::mat4 fanMat = glm::translate(roomBase, glm::vec3(0.0f, 3.95f, -0.5f));
        drawCeilingFan(cubeVAO, lightingShader, fanMat, fanAngle);

        // 7. Lighting Fixtures
        // Rectangular ceiling point-light fixtures (each independently lit/dimmed)
        bool pointLightStates[4] = { pointLight1On, pointLight2On, pointLight3On, pointLight4On };
        for (int i = 0; i < 4; ++i) {
            glm::mat4 fixtureMat = glm::translate(roomBase, pointLightPositions[i]);
            drawCeilingLightFixture(cubeVAO, lightingShader, ourShader, fixtureMat, pointLightStates[i]);
        }

        // Ceiling Multimedia Projector (Spotlight source illuminating the board)
        glm::mat4 spotFixtureMat = glm::translate(roomBase, blackboardSpotlight.position);
        drawSpotlightFixture(cubeVAO, lightingShader, ourShader, spotFixtureMat, isProjectorOn);

        // 8. Moving Object: 3D Classroom Robot beside Blackboard (Waving Bye-Bye Hand!)
        glm::mat4 robotMat = glm::translate(roomBase, glm::vec3(3.2f, 0.0f, -5.2f));
        drawRobot(cubeVAO, lightingShader, ourShader, robotMat, robotWaveAngle);

        // Swap buffers and poll input events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup resources
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteBuffers(1, &cubeVBO);
    glDeleteBuffers(1, &cubeEBO);

    glfwTerminate();
    return 0;
}

// =========================================================================
// Drawing Functions: Modular Hierarchical Cube-based Models
// =========================================================================

// Base Cube Drawing Helper: Binds material properties and draws a transformed unit cube
void drawCube(unsigned int& cubeVAO, Shader& lightingShader, glm::mat4 model, 
              float r, float g, float b, float spec, float shininess)
{
    lightingShader.use();
    lightingShader.setVec3("material.ambient", glm::vec3(r, g, b));
    lightingShader.setVec3("material.diffuse", glm::vec3(r, g, b));
    lightingShader.setVec3("material.specular", glm::vec3(spec, spec, spec));
    lightingShader.setFloat("material.shininess", shininess);
    lightingShader.setMat4("model", model);

    glBindVertexArray(cubeVAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
}

// 1. Classroom Floor (Light Gray Tile Appearance)
void drawFloor(unsigned int& cubeVAO, Shader& lightingShader, glm::mat4 roomBase)
{
    // Floor slab: width = 10.0, length = 12.0, thickness = 0.1
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-5.0f, -0.1f, -6.0f));
    model = glm::scale(model, glm::vec3(10.0f, 0.1f, 12.0f));
    drawCube(cubeVAO, lightingShader, roomBase * model, 0.82f, 0.82f, 0.84f, 0.4f, 64.0f);
}

// 2. Classroom Walls and Ceiling
void drawWallsAndCeiling(unsigned int& cubeVAO, Shader& lightingShader, glm::mat4 roomBase)
{
    // Wall color: Warm Cream / Off-White
    float wr = 0.92f, wg = 0.90f, wb = 0.86f;

    // --- FRONT WALL (Mounts the Blackboard and Clock, Z = -6.0) ---
    glm::mat4 frontWall = glm::mat4(1.0f);
    frontWall = glm::translate(frontWall, glm::vec3(-5.0f, 0.0f, -6.1f));
    frontWall = glm::scale(frontWall, glm::vec3(10.0f, 4.0f, 0.1f));
    drawCube(cubeVAO, lightingShader, roomBase * frontWall, wr, wg, wb);

    // --- BACK WALL (Behind Students, Z = +6.0) ---
    glm::mat4 backWall = glm::mat4(1.0f);
    backWall = glm::translate(backWall, glm::vec3(-5.0f, 0.0f, 6.0f));
    backWall = glm::scale(backWall, glm::vec3(10.0f, 4.0f, 0.1f));
    drawCube(cubeVAO, lightingShader, roomBase * backWall, wr, wg, wb);

    // --- LEFT WALL SECTIONS (Enclosing the 2 Windows, X = -5.0) ---
    // Bottom sill wall section
    glm::mat4 leftWallBottom = glm::mat4(1.0f);
    leftWallBottom = glm::translate(leftWallBottom, glm::vec3(-5.1f, 0.0f, -6.0f));
    leftWallBottom = glm::scale(leftWallBottom, glm::vec3(0.1f, 1.4f, 12.0f));
    drawCube(cubeVAO, lightingShader, roomBase * leftWallBottom, wr, wg, wb);

    // Top lintel wall section above windows
    glm::mat4 leftWallTop = glm::mat4(1.0f);
    leftWallTop = glm::translate(leftWallTop, glm::vec3(-5.1f, 3.2f, -6.0f));
    leftWallTop = glm::scale(leftWallTop, glm::vec3(0.1f, 0.8f, 12.0f));
    drawCube(cubeVAO, lightingShader, roomBase * leftWallTop, wr, wg, wb);

    // Vertical wall pillars framing windows
    // Front corner pillar
    glm::mat4 leftPillar1 = glm::mat4(1.0f);
    leftPillar1 = glm::translate(leftPillar1, glm::vec3(-5.1f, 1.4f, -6.0f));
    leftPillar1 = glm::scale(leftPillar1, glm::vec3(0.1f, 1.8f, 1.2f));
    drawCube(cubeVAO, lightingShader, roomBase * leftPillar1, wr, wg, wb);

    // Middle pillar between Window 1 and Window 2
    glm::mat4 leftPillar2 = glm::mat4(1.0f);
    leftPillar2 = glm::translate(leftPillar2, glm::vec3(-5.1f, 1.4f, -2.4f));
    leftPillar2 = glm::scale(leftPillar2, glm::vec3(0.1f, 1.8f, 1.4f));
    drawCube(cubeVAO, lightingShader, roomBase * leftPillar2, wr, wg, wb);

    // Back pillar
    glm::mat4 leftPillar3 = glm::mat4(1.0f);
    leftPillar3 = glm::translate(leftPillar3, glm::vec3(-5.1f, 1.4f, 1.4f));
    leftPillar3 = glm::scale(leftPillar3, glm::vec3(0.1f, 1.8f, 4.6f));
    drawCube(cubeVAO, lightingShader, roomBase * leftPillar3, wr, wg, wb);

    // --- RIGHT WALL (With Door Cutout, X = +5.0) ---
    // Front section before door
    glm::mat4 rightFront = glm::mat4(1.0f);
    rightFront = glm::translate(rightFront, glm::vec3(5.0f, 0.0f, -6.0f));
    rightFront = glm::scale(rightFront, glm::vec3(0.1f, 4.0f, 2.0f));
    drawCube(cubeVAO, lightingShader, roomBase * rightFront, wr, wg, wb);

    // Section above door
    glm::mat4 rightAboveDoor = glm::mat4(1.0f);
    rightAboveDoor = glm::translate(rightAboveDoor, glm::vec3(5.0f, 2.8f, -4.0f));
    rightAboveDoor = glm::scale(rightAboveDoor, glm::vec3(0.1f, 1.2f, 1.5f));
    drawCube(cubeVAO, lightingShader, roomBase * rightAboveDoor, wr, wg, wb);

    // Back section after door
    glm::mat4 rightBack = glm::mat4(1.0f);
    rightBack = glm::translate(rightBack, glm::vec3(5.0f, 0.0f, -2.5f));
    rightBack = glm::scale(rightBack, glm::vec3(0.1f, 4.0f, 8.5f));
    drawCube(cubeVAO, lightingShader, roomBase * rightBack, wr, wg, wb);

    // --- CEILING ---
    glm::mat4 ceiling = glm::mat4(1.0f);
    ceiling = glm::translate(ceiling, glm::vec3(-5.0f, 4.0f, -6.0f));
    ceiling = glm::scale(ceiling, glm::vec3(10.0f, 0.1f, 12.0f));
    drawCube(cubeVAO, lightingShader, roomBase * ceiling, 0.94f, 0.94f, 0.94f);
}

// 3. Student Desk (Warm Honey Wooden Top, Dark Charcoal Legs & Under-desk Apron)
void drawStudentDesk(unsigned int& cubeVAO, Shader& lightingShader, glm::mat4 deskBase, bool hasRobotics)
{
    float topWidth = 1.15f;
    float topLength = 0.60f;
    float topThickness = 0.05f;
    float deskHeight = 0.72f;

    float legThick = 0.055f;
    float legHeight = deskHeight - topThickness;

    // Dark charcoal leg & frame color
    float lr = 0.16f, lg = 0.15f, lb = 0.14f;
    // Warm wood top color
    float tr = 0.82f, tg = 0.52f, tb = 0.28f;

    // 4 Legs
    float halfW = topWidth / 2.0f - 0.04f;
    float halfL = topLength / 2.0f - 0.04f;
    float legX[4] = { -halfW, halfW - legThick, -halfW, halfW - legThick };
    float legZ[4] = { -halfL, -halfL, halfL - legThick, halfL - legThick };

    for (int i = 0; i < 4; ++i) {
        glm::mat4 leg = glm::mat4(1.0f);
        leg = glm::translate(leg, glm::vec3(legX[i], 0.0f, legZ[i]));
        leg = glm::scale(leg, glm::vec3(legThick, legHeight, legThick));
        drawCube(cubeVAO, lightingShader, deskBase * leg, lr, lg, lb);
    }

    // Under-table structural apron rails
    glm::mat4 apronFront = glm::mat4(1.0f);
    apronFront = glm::translate(apronFront, glm::vec3(-halfW, legHeight - 0.06f, -halfL));
    apronFront = glm::scale(apronFront, glm::vec3(2 * halfW, 0.06f, 0.03f));
    drawCube(cubeVAO, lightingShader, deskBase * apronFront, lr, lg, lb);

    // Wooden Tabletop
    glm::mat4 top = glm::mat4(1.0f);
    top = glm::translate(top, glm::vec3(-topWidth / 2.0f, legHeight, -topLength / 2.0f));
    top = glm::scale(top, glm::vec3(topWidth, topThickness, topLength));
    drawCube(cubeVAO, lightingShader, deskBase * top, tr, tg, tb, 0.4f, 48.0f);

    // Small pencil case / notebook accessory on desk (only on regular non-robotics desks)
    if (!hasRobotics) {
        glm::mat4 book = glm::mat4(1.0f);
        book = glm::translate(book, glm::vec3(-0.25f, deskHeight, -0.1f));
        book = glm::scale(book, glm::vec3(0.25f, 0.015f, 0.18f));
        drawCube(cubeVAO, lightingShader, deskBase * book, 0.35f, 0.35f, 0.40f);
    }
}

// 4. Student Chair (Ergonomic Honey Wood Seat & Backrest, Continuous Charcoal Steel Frame - Zero Z-Fighting)
void drawStudentChair(unsigned int& cubeVAO, Shader& lightingShader, glm::mat4 chairBase)
{
    float seatW = 0.44f;
    float seatThick = 0.035f;
    float seatHeight = 0.44f;

    float legThick = 0.040f;
    float legH = seatHeight - seatThick; // 0.405m

    float lr = 0.16f, lg = 0.15f, lb = 0.14f; // Charcoal metal frame
    float wr = 0.82f, wg = 0.52f, wb = 0.28f; // Honey wood

    float hw = seatW / 2.0f - 0.03f; // 0.19m
    float hl = 0.18f;

    // 1. Front Legs (2 legs from floor to under the seat)
    // Left Front
    glm::mat4 frontLegL = glm::mat4(1.0f);
    frontLegL = glm::translate(frontLegL, glm::vec3(-hw, 0.0f, -hl));
    frontLegL = glm::scale(frontLegL, glm::vec3(legThick, legH, legThick));
    drawCube(cubeVAO, lightingShader, chairBase * frontLegL, lr, lg, lb);

    // Right Front
    glm::mat4 frontLegR = glm::mat4(1.0f);
    frontLegR = glm::translate(frontLegR, glm::vec3(hw - legThick, 0.0f, -hl));
    frontLegR = glm::scale(frontLegR, glm::vec3(legThick, legH, legThick));
    drawCube(cubeVAO, lightingShader, chairBase * frontLegR, lr, lg, lb);

    // 2. Rear Legs & Upright Frame (Continuous vertical steel posts from floor Y=0 to Y=0.74m)
    float postTotalH = 0.74f;

    // Left Rear Continuous Post
    glm::mat4 postLeft = glm::mat4(1.0f);
    postLeft = glm::translate(postLeft, glm::vec3(-hw, 0.0f, hl - legThick));
    postLeft = glm::scale(postLeft, glm::vec3(legThick, postTotalH, legThick));
    drawCube(cubeVAO, lightingShader, chairBase * postLeft, lr, lg, lb);

    // Right Rear Continuous Post
    glm::mat4 postRight = glm::mat4(1.0f);
    postRight = glm::translate(postRight, glm::vec3(hw - legThick, 0.0f, hl - legThick));
    postRight = glm::scale(postRight, glm::vec3(legThick, postTotalH, legThick));
    drawCube(cubeVAO, lightingShader, chairBase * postRight, lr, lg, lb);

    // Post Molded Black End Caps (Flush on top of the posts)
    glm::mat4 capL = glm::mat4(1.0f);
    capL = glm::translate(capL, glm::vec3(-hw - 0.001f, postTotalH, hl - legThick - 0.001f));
    capL = glm::scale(capL, glm::vec3(legThick + 0.002f, 0.006f, legThick + 0.002f));
    drawCube(cubeVAO, lightingShader, chairBase * capL, 0.10f, 0.10f, 0.10f);

    glm::mat4 capR = glm::mat4(1.0f);
    capR = glm::translate(capR, glm::vec3(hw - legThick - 0.001f, postTotalH, hl - legThick - 0.001f));
    capR = glm::scale(capR, glm::vec3(legThick + 0.002f, 0.006f, legThick + 0.002f));
    drawCube(cubeVAO, lightingShader, chairBase * capR, 0.10f, 0.10f, 0.10f);

    // 3. Wooden Seat (Contoured classroom seat positioned cleanly in front of rear posts)
    float seatFrontZ = -hl - 0.025f;
    float seatBackZ = hl - legThick - 0.006f; // Leaves 6mm clearance in front of rear posts
    float seatDepth = seatBackZ - seatFrontZ;

    glm::mat4 seat = glm::mat4(1.0f);
    seat = glm::translate(seat, glm::vec3(-seatW / 2.0f, legH, seatFrontZ));
    seat = glm::scale(seat, glm::vec3(seatW, seatThick, seatDepth));
    drawCube(cubeVAO, lightingShader, chairBase * seat, wr, wg, wb, 0.4f, 48.0f);

    // 4. Ergonomic Wooden Backrest (Mounted in FRONT of the upright posts with guaranteed depth separation)
    // Upright posts occupy Z: [hl - legThick, hl] = [0.140, 0.180]
    // Backrest occupies Z: [hl - legThick - 0.005f - 0.022f, hl - legThick - 0.005f] = [0.113, 0.135]
    // Result: 5mm clear air gap between backrest rear face and post front face -> ZERO Z-FIGHTING!
    float panelW = seatW;
    float panelH = 0.17f;
    float panelThick = 0.022f;
    float panelPosZ = hl - legThick - 0.005f - panelThick;
    float panelPosY = postTotalH - panelH - 0.020f; // Sits 2cm below post top

    glm::mat4 backPanel = glm::mat4(1.0f);
    backPanel = glm::translate(backPanel, glm::vec3(-panelW / 2.0f, panelPosY, panelPosZ));
    backPanel = glm::scale(backPanel, glm::vec3(panelW, panelH, panelThick));
    drawCube(cubeVAO, lightingShader, chairBase * backPanel, wr, wg, wb, 0.4f, 48.0f);

    // 5. Metal Backrest Mounting Brackets (Sleek tabs attaching wood to metal posts)
    float bracketW = legThick - 0.006f;
    float bracketH = 0.050f;
    float bracketZ = hl - legThick - 0.005f; // Spans the 5mm gap between panel and post

    glm::mat4 brkL = glm::mat4(1.0f);
    brkL = glm::translate(brkL, glm::vec3(-hw + 0.003f, panelPosY + 0.06f, bracketZ));
    brkL = glm::scale(brkL, glm::vec3(bracketW, bracketH, 0.006f));
    drawCube(cubeVAO, lightingShader, chairBase * brkL, lr, lg, lb);

    glm::mat4 brkR = glm::mat4(1.0f);
    brkR = glm::translate(brkR, glm::vec3(hw - legThick + 0.003f, panelPosY + 0.06f, bracketZ));
    brkR = glm::scale(brkR, glm::vec3(bracketW, bracketH, 0.006f));
    drawCube(cubeVAO, lightingShader, chairBase * brkR, lr, lg, lb);

    // 6. Lower Structural Leg Stretchers / Rungs (Connecting front and rear legs at Y=0.10m)
    float rungZStart = -hl + legThick;
    float rungZLength = (hl - legThick) - rungZStart; // Perfectly spans between legs without intersecting them!

    glm::mat4 rungLeft = glm::mat4(1.0f);
    rungLeft = glm::translate(rungLeft, glm::vec3(-hw + 0.004f, 0.10f, rungZStart));
    rungLeft = glm::scale(rungLeft, glm::vec3(legThick - 0.008f, 0.020f, rungZLength));
    drawCube(cubeVAO, lightingShader, chairBase * rungLeft, lr, lg, lb);

    glm::mat4 rungRight = glm::mat4(1.0f);
    rungRight = glm::translate(rungRight, glm::vec3(hw - legThick + 0.004f, 0.10f, rungZStart));
    rungRight = glm::scale(rungRight, glm::vec3(legThick - 0.008f, 0.020f, rungZLength));
    drawCube(cubeVAO, lightingShader, chairBase * rungRight, lr, lg, lb);
}

// =========================================================================
// Embedded Classroom: 3D Student Workstation Laptop & Robotics Hardware
// =========================================================================

// -------------------------------------------------------------------------
// 3D Student Workstation Laptop (Space-Grey Body, Keyboard, Trackpad & Glowing IDE Screen)
// -------------------------------------------------------------------------
void drawLaptop(unsigned int& cubeVAO, Shader& lightingShader, Shader& ourShader, glm::mat4 deskMat, 
                float posX, float posZ, float deskH, float rotY, float scaleLap)
{
    float lapW = 0.28f * scaleLap;   // Laptop width
    float lapD = 0.20f * scaleLap;   // Laptop depth
    float lapH = 0.012f * scaleLap;  // Base chassis thickness

    glm::mat4 lapPivot = glm::translate(deskMat, glm::vec3(posX, deskH, posZ));
    if (rotY != 0.0f) {
        lapPivot = glm::rotate(lapPivot, glm::radians(rotY), glm::vec3(0.0f, 1.0f, 0.0f));
    }

    // 1. Laptop Lower Base Chassis (Space-Grey Anodized Aluminum)
    lightingShader.use();
    glm::mat4 base = glm::mat4(1.0f);
    base = glm::translate(base, glm::vec3(-lapW / 2.0f, 0.0f, -lapD / 2.0f));
    base = glm::scale(base, glm::vec3(lapW, lapH, lapD));
    drawCube(cubeVAO, lightingShader, lapPivot * base, 0.24f, 0.25f, 0.27f, 0.6f, 64.0f);

    // 2. Recessed Keyboard Well & Matte Keys
    glm::mat4 kb = glm::mat4(1.0f);
    kb = glm::translate(kb, glm::vec3(-0.12f * scaleLap, lapH, -0.08f * scaleLap));
    kb = glm::scale(kb, glm::vec3(0.24f * scaleLap, 0.003f * scaleLap, 0.10f * scaleLap));
    drawCube(cubeVAO, lightingShader, lapPivot * kb, 0.12f, 0.12f, 0.13f, 0.2f, 16.0f);

    // Key Row Accents / Spacebar
    glm::mat4 spacebar = glm::mat4(1.0f);
    spacebar = glm::translate(spacebar, glm::vec3(-0.04f * scaleLap, lapH + 0.002f * scaleLap, -0.005f * scaleLap));
    spacebar = glm::scale(spacebar, glm::vec3(0.08f * scaleLap, 0.002f * scaleLap, 0.018f * scaleLap));
    drawCube(cubeVAO, lightingShader, lapPivot * spacebar, 0.18f, 0.18f, 0.20f);

    // 3. Smooth Trackpad
    glm::mat4 trackpad = glm::mat4(1.0f);
    trackpad = glm::translate(trackpad, glm::vec3(-0.045f * scaleLap, lapH + 0.001f * scaleLap, 0.025f * scaleLap));
    trackpad = glm::scale(trackpad, glm::vec3(0.09f * scaleLap, 0.002f * scaleLap, 0.065f * scaleLap));
    drawCube(cubeVAO, lightingShader, lapPivot * trackpad, 0.32f, 0.33f, 0.35f, 0.5f, 32.0f);

    // 4. Power & Battery Status LED (Tiny glowing green indicator on the edge)
    ourShader.use();
    glm::mat4 pwrLed = glm::mat4(1.0f);
    pwrLed = glm::translate(pwrLed, glm::vec3(lapW / 2.0f - 0.015f * scaleLap, lapH * 0.5f, lapD / 2.0f - 0.015f * scaleLap));
    pwrLed = glm::scale(pwrLed, glm::vec3(0.004f * scaleLap, 0.004f * scaleLap, 0.004f * scaleLap));
    ourShader.setMat4("model", lapPivot * pwrLed);
    ourShader.setVec3("color", glm::vec3(0.1f, 0.95f, 0.3f));
    glBindVertexArray(cubeVAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

    // 5. Open Laptop Screen Lid (Tilted -15 degrees towards back, facing the seated user)
    lightingShader.use();
    float hingeZ = -lapD / 2.0f + 0.01f * scaleLap;
    float hingeY = lapH;
    float lidH = 0.19f * scaleLap;
    float lidThick = 0.008f * scaleLap;

    glm::mat4 lidPivot = glm::translate(lapPivot, glm::vec3(0.0f, hingeY, hingeZ));
    lidPivot = glm::rotate(lidPivot, glm::radians(-15.0f), glm::vec3(1.0f, 0.0f, 0.0f));

    // Outer Lid Shell & Bezel
    glm::mat4 lid = glm::mat4(1.0f);
    lid = glm::translate(lid, glm::vec3(-lapW / 2.0f, 0.0f, -lidThick));
    lid = glm::scale(lid, glm::vec3(lapW, lidH, lidThick));
    drawCube(cubeVAO, lightingShader, lidPivot * lid, 0.22f, 0.23f, 0.25f, 0.6f, 64.0f);

    // 6. Glowing Screen Display (Code Editor / Terminal - rendered unlit with ourShader for vivid luminescence)
    ourShader.use();
    glm::mat4 screen = glm::mat4(1.0f);
    screen = glm::translate(screen, glm::vec3(-lapW / 2.0f + 0.015f * scaleLap, 0.012f * scaleLap, 0.001f));
    screen = glm::scale(screen, glm::vec3(lapW - 0.030f * scaleLap, lidH - 0.024f * scaleLap, 0.002f));
    ourShader.setMat4("model", lidPivot * screen);
    ourShader.setVec3("color", glm::vec3(0.08f, 0.35f, 0.62f)); // Glowing cyan/blue IDE screen
    glBindVertexArray(cubeVAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

    // IDE Code Lines Accent (Simulating lines of code on display)
    glm::mat4 code1 = glm::mat4(1.0f);
    code1 = glm::translate(code1, glm::vec3(-lapW / 2.0f + 0.03f * scaleLap, lidH - 0.05f * scaleLap, 0.002f));
    code1 = glm::scale(code1, glm::vec3(0.12f * scaleLap, 0.008f * scaleLap, 0.002f));
    ourShader.setMat4("model", lidPivot * code1);
    ourShader.setVec3("color", glm::vec3(0.40f, 0.85f, 1.0f)); // Bright syntax highlight line
    glBindVertexArray(cubeVAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

    glm::mat4 code2 = glm::mat4(1.0f);
    code2 = glm::translate(code2, glm::vec3(-lapW / 2.0f + 0.03f * scaleLap, lidH - 0.075f * scaleLap, 0.002f));
    code2 = glm::scale(code2, glm::vec3(0.16f * scaleLap, 0.008f * scaleLap, 0.002f));
    ourShader.setMat4("model", lidPivot * code2);
    ourShader.setVec3("color", glm::vec3(0.30f, 0.95f, 0.45f)); // Green code line
    glBindVertexArray(cubeVAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
}

// -------------------------------------------------------------------------
// Table 1 (Left Column, Front Desk): Embedded Microcontroller (Arduino Uno), Breadboard, Ultrasonic Sensor & Servo
// -------------------------------------------------------------------------
void drawEmbeddedKitTable1(unsigned int& cubeVAO, Shader& lightingShader, Shader& ourShader, glm::mat4 deskMat)
{
    float deskH = 0.72f;

    // USB Connection Cable from Student Laptop (X = -0.14f) to Arduino (X = 0.06f)
    lightingShader.use();
    glm::mat4 usbCable = glm::mat4(1.0f);
    usbCable = glm::translate(usbCable, glm::vec3(-0.14f, deskH + 0.006f, -0.08f));
    usbCable = glm::scale(usbCable, glm::vec3(0.20f, 0.006f, 0.006f));
    drawCube(cubeVAO, lightingShader, deskMat * usbCable, 0.10f, 0.10f, 0.12f); // Matte black USB cable

    // 1. Arduino / Microcontroller Development Board (Uno Cyan PCB) on Right-Center
    glm::mat4 pcb = glm::mat4(1.0f);
    pcb = glm::translate(pcb, glm::vec3(0.06f, deskH, -0.14f));
    pcb = glm::scale(pcb, glm::vec3(0.16f, 0.012f, 0.11f));
    drawCube(cubeVAO, lightingShader, deskMat * pcb, 0.05f, 0.50f, 0.65f, 0.6f, 64.0f); // Teal/cyan board

    // Microcontroller IC Chip (DIP package with silver pin legs)
    glm::mat4 ic = glm::mat4(1.0f);
    ic = glm::translate(ic, glm::vec3(0.09f, deskH + 0.012f, -0.085f));
    ic = glm::scale(ic, glm::vec3(0.07f, 0.010f, 0.022f));
    drawCube(cubeVAO, lightingShader, deskMat * ic, 0.12f, 0.12f, 0.12f, 0.2f, 16.0f);

    // USB-B Metal Port
    glm::mat4 usb = glm::mat4(1.0f);
    usb = glm::translate(usb, glm::vec3(0.05f, deskH + 0.012f, -0.125f));
    usb = glm::scale(usb, glm::vec3(0.030f, 0.022f, 0.025f));
    drawCube(cubeVAO, lightingShader, deskMat * usb, 0.75f, 0.75f, 0.78f, 0.8f, 64.0f);

    // DC Power Jack
    glm::mat4 dc = glm::mat4(1.0f);
    dc = glm::translate(dc, glm::vec3(0.05f, deskH + 0.012f, -0.065f));
    dc = glm::scale(dc, glm::vec3(0.030f, 0.022f, 0.022f));
    drawCube(cubeVAO, lightingShader, deskMat * dc, 0.15f, 0.15f, 0.15f);

    // Female Header Strips (Digital & Analog pins)
    glm::mat4 hdr1 = glm::mat4(1.0f);
    hdr1 = glm::translate(hdr1, glm::vec3(0.07f, deskH + 0.012f, -0.135f));
    hdr1 = glm::scale(hdr1, glm::vec3(0.13f, 0.018f, 0.014f));
    drawCube(cubeVAO, lightingShader, deskMat * hdr1, 0.10f, 0.10f, 0.10f);

    glm::mat4 hdr2 = glm::mat4(1.0f);
    hdr2 = glm::translate(hdr2, glm::vec3(0.07f, deskH + 0.012f, -0.045f));
    hdr2 = glm::scale(hdr2, glm::vec3(0.13f, 0.018f, 0.014f));
    drawCube(cubeVAO, lightingShader, deskMat * hdr2, 0.10f, 0.10f, 0.10f);

    // Status Indicator LEDs (Green Power LED & Amber RX LED)
    ourShader.use();
    glm::mat4 pwrLed = glm::mat4(1.0f);
    pwrLed = glm::translate(pwrLed, glm::vec3(0.16f, deskH + 0.013f, -0.12f));
    pwrLed = glm::scale(pwrLed, glm::vec3(0.010f, 0.010f, 0.010f));
    ourShader.setMat4("model", deskMat * pwrLed);
    ourShader.setVec3("color", glm::vec3(0.1f, 1.0f, 0.2f)); // Glowing Green LED
    glBindVertexArray(cubeVAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

    glm::mat4 rxLed = glm::mat4(1.0f);
    rxLed = glm::translate(rxLed, glm::vec3(0.18f, deskH + 0.013f, -0.12f));
    rxLed = glm::scale(rxLed, glm::vec3(0.010f, 0.010f, 0.010f));
    ourShader.setMat4("model", deskMat * rxLed);
    ourShader.setVec3("color", glm::vec3(1.0f, 0.75f, 0.1f)); // Glowing Amber LED
    glBindVertexArray(cubeVAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

    // 2. Prototyping Breadboard with Pin Rails & Jumper Wires (Positioned forward at Z = 0.01f)
    lightingShader.use();
    glm::mat4 bb = glm::mat4(1.0f);
    bb = glm::translate(bb, glm::vec3(0.06f, deskH, 0.01f));
    bb = glm::scale(bb, glm::vec3(0.18f, 0.016f, 0.15f));
    drawCube(cubeVAO, lightingShader, deskMat * bb, 0.95f, 0.94f, 0.92f, 0.1f, 8.0f); // Off-white breadboard

    // Power Rails (Red positive stripe, Blue negative stripe)
    glm::mat4 railRed = glm::mat4(1.0f);
    railRed = glm::translate(railRed, glm::vec3(0.07f, deskH + 0.0165f, 0.015f));
    railRed = glm::scale(railRed, glm::vec3(0.16f, 0.002f, 0.008f));
    drawCube(cubeVAO, lightingShader, deskMat * railRed, 0.85f, 0.15f, 0.15f);

    glm::mat4 railBlue = glm::mat4(1.0f);
    railBlue = glm::translate(railBlue, glm::vec3(0.07f, deskH + 0.0165f, 0.145f));
    railBlue = glm::scale(railBlue, glm::vec3(0.16f, 0.002f, 0.008f));
    drawCube(cubeVAO, lightingShader, deskMat * railBlue, 0.15f, 0.35f, 0.85f);

    // Jumper Wires (Red, Blue, Yellow arched wires connecting boards)
    glm::mat4 wire1 = glm::mat4(1.0f);
    wire1 = glm::translate(wire1, glm::vec3(0.10f, deskH + 0.018f, -0.04f));
    wire1 = glm::scale(wire1, glm::vec3(0.008f, 0.014f, 0.06f));
    drawCube(cubeVAO, lightingShader, deskMat * wire1, 0.9f, 0.2f, 0.1f); // Red jumper wire

    glm::mat4 wire2 = glm::mat4(1.0f);
    wire2 = glm::translate(wire2, glm::vec3(0.14f, deskH + 0.018f, -0.04f));
    wire2 = glm::scale(wire2, glm::vec3(0.008f, 0.016f, 0.06f));
    drawCube(cubeVAO, lightingShader, deskMat * wire2, 0.1f, 0.5f, 0.9f); // Blue jumper wire

    glm::mat4 wire3 = glm::mat4(1.0f);
    wire3 = glm::translate(wire3, glm::vec3(0.12f, deskH + 0.020f, 0.06f));
    wire3 = glm::scale(wire3, glm::vec3(0.06f, 0.008f, 0.008f));
    drawCube(cubeVAO, lightingShader, deskMat * wire3, 0.95f, 0.85f, 0.15f); // Yellow wire

    // 3. HC-SR04 Ultrasonic Distance Sensor Module (Positioned at right edge X = 0.32f)
    glm::mat4 usPcb = glm::mat4(1.0f);
    usPcb = glm::translate(usPcb, glm::vec3(0.32f, deskH, -0.12f));
    usPcb = glm::scale(usPcb, glm::vec3(0.012f, 0.05f, 0.09f));
    drawCube(cubeVAO, lightingShader, deskMat * usPcb, 0.05f, 0.45f, 0.70f); // Blue PCB vertical

    // Transducer "Eye" 1 (Emitter)
    glm::mat4 usEye1 = glm::mat4(1.0f);
    usEye1 = glm::translate(usEye1, glm::vec3(0.332f, deskH + 0.015f, -0.115f));
    usEye1 = glm::scale(usEye1, glm::vec3(0.028f, 0.028f, 0.032f));
    drawCube(cubeVAO, lightingShader, deskMat * usEye1, 0.80f, 0.82f, 0.85f, 0.7f, 32.0f);

    // Transducer "Eye" 2 (Receiver)
    glm::mat4 usEye2 = glm::mat4(1.0f);
    usEye2 = glm::translate(usEye2, glm::vec3(0.332f, deskH + 0.015f, -0.065f));
    usEye2 = glm::scale(usEye2, glm::vec3(0.028f, 0.028f, 0.032f));
    drawCube(cubeVAO, lightingShader, deskMat * usEye2, 0.80f, 0.82f, 0.85f, 0.7f, 32.0f);

    // 4. Micro Servo Motor (SG90 Blue body with white horn) (Positioned at X = 0.31f, Z = 0.03f)
    glm::mat4 servo = glm::mat4(1.0f);
    servo = glm::translate(servo, glm::vec3(0.31f, deskH, 0.03f));
    servo = glm::scale(servo, glm::vec3(0.045f, 0.055f, 0.025f));
    drawCube(cubeVAO, lightingShader, deskMat * servo, 0.10f, 0.35f, 0.85f, 0.5f, 32.0f); // Translucent blue servo body

    // White horn / servo arm
    glm::mat4 horn = glm::mat4(1.0f);
    horn = glm::translate(horn, glm::vec3(0.30f, deskH + 0.055f, 0.025f));
    horn = glm::scale(horn, glm::vec3(0.065f, 0.012f, 0.02f));
    drawCube(cubeVAO, lightingShader, deskMat * horn, 0.95f, 0.95f, 0.95f);
}

// -------------------------------------------------------------------------
// Table 2 (Left Column, Second Desk): 3-DOF Articulated Robotic Arm & Digital Multimeter
// -------------------------------------------------------------------------
void drawRoboticArmTable2(unsigned int& cubeVAO, Shader& lightingShader, Shader& ourShader, glm::mat4 deskMat)
{
    float deskH = 0.72f;

    // 1. Desktop Articulated Robotic Arm (Base at X = 0.08f to 0.24f, completely clear of laptop)
    lightingShader.use();
    // Heavy circular turntable base
    glm::mat4 base = glm::mat4(1.0f);
    base = glm::translate(base, glm::vec3(0.08f, deskH, -0.06f));
    base = glm::scale(base, glm::vec3(0.16f, 0.03f, 0.16f));
    drawCube(cubeVAO, lightingShader, deskMat * base, 0.18f, 0.18f, 0.20f, 0.5f, 32.0f);

    // Turntable rotating collar
    glm::mat4 collar = glm::mat4(1.0f);
    collar = glm::translate(collar, glm::vec3(0.12f, deskH + 0.03f, -0.02f));
    collar = glm::scale(collar, glm::vec3(0.08f, 0.035f, 0.08f));
    drawCube(cubeVAO, lightingShader, deskMat * collar, 0.28f, 0.28f, 0.32f);

    // Shoulder Joint Servo Housing
    glm::mat4 shoulder = glm::mat4(1.0f);
    shoulder = glm::translate(shoulder, glm::vec3(0.13f, deskH + 0.065f, -0.01f));
    shoulder = glm::scale(shoulder, glm::vec3(0.06f, 0.06f, 0.06f));
    drawCube(cubeVAO, lightingShader, deskMat * shoulder, 0.15f, 0.15f, 0.18f);

    // Lower Arm Link (Angled forward 35 degrees)
    glm::mat4 lowerArm = glm::mat4(1.0f);
    lowerArm = glm::translate(lowerArm, glm::vec3(0.14f, deskH + 0.11f, 0.01f));
    lowerArm = glm::rotate(lowerArm, glm::radians(35.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    lowerArm = glm::scale(lowerArm, glm::vec3(0.04f, 0.16f, 0.04f));
    drawCube(cubeVAO, lightingShader, deskMat * lowerArm, 0.92f, 0.55f, 0.08f, 0.6f, 64.0f); // Industrial Orange

    // Elbow Joint Servo
    glm::mat4 elbow = glm::mat4(1.0f);
    elbow = glm::translate(elbow, glm::vec3(0.13f, deskH + 0.22f, 0.09f));
    elbow = glm::scale(elbow, glm::vec3(0.06f, 0.05f, 0.055f));
    drawCube(cubeVAO, lightingShader, deskMat * elbow, 0.18f, 0.18f, 0.22f);

    // Forearm Link (Extending forward)
    glm::mat4 foreArm = glm::mat4(1.0f);
    foreArm = glm::translate(foreArm, glm::vec3(0.14f, deskH + 0.22f, 0.12f));
    foreArm = glm::rotate(foreArm, glm::radians(-40.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    foreArm = glm::scale(foreArm, glm::vec3(0.035f, 0.12f, 0.035f));
    drawCube(cubeVAO, lightingShader, deskMat * foreArm, 0.92f, 0.55f, 0.08f, 0.6f, 64.0f);

    // Wrist & Gripper Claw
    glm::mat4 wrist = glm::mat4(1.0f);
    wrist = glm::translate(wrist, glm::vec3(0.14f, deskH + 0.13f, 0.20f));
    wrist = glm::scale(wrist, glm::vec3(0.04f, 0.03f, 0.03f));
    drawCube(cubeVAO, lightingShader, deskMat * wrist, 0.18f, 0.18f, 0.20f);

    // Left Gripper Finger
    glm::mat4 finger1 = glm::mat4(1.0f);
    finger1 = glm::translate(finger1, glm::vec3(0.13f, deskH + 0.10f, 0.22f));
    finger1 = glm::scale(finger1, glm::vec3(0.012f, 0.035f, 0.04f));
    drawCube(cubeVAO, lightingShader, deskMat * finger1, 0.75f, 0.78f, 0.82f, 0.8f, 64.0f); // Metallic silver claw

    // Right Gripper Finger
    glm::mat4 finger2 = glm::mat4(1.0f);
    finger2 = glm::translate(finger2, glm::vec3(0.175f, deskH + 0.10f, 0.22f));
    finger2 = glm::scale(finger2, glm::vec3(0.012f, 0.035f, 0.04f));
    drawCube(cubeVAO, lightingShader, deskMat * finger2, 0.75f, 0.78f, 0.82f, 0.8f, 64.0f);

    // Red Sample Testing Block being held by the gripper
    glm::mat4 sampleBlock = glm::mat4(1.0f);
    sampleBlock = glm::translate(sampleBlock, glm::vec3(0.145f, deskH + 0.105f, 0.23f));
    sampleBlock = glm::scale(sampleBlock, glm::vec3(0.028f, 0.028f, 0.028f));
    drawCube(cubeVAO, lightingShader, deskMat * sampleBlock, 0.90f, 0.15f, 0.15f, 0.5f, 32.0f);

    // 2. Digital Multimeter (Safety Yellow Holster, Positioned at X = 0.32f to 0.42f)
    glm::mat4 dmm = glm::mat4(1.0f);
    dmm = glm::translate(dmm, glm::vec3(0.32f, deskH, -0.05f));
    dmm = glm::scale(dmm, glm::vec3(0.10f, 0.035f, 0.15f));
    drawCube(cubeVAO, lightingShader, deskMat * dmm, 0.96f, 0.82f, 0.08f); // Vivid multimeter yellow

    // LCD Screen Window (Unlit light grey screen)
    ourShader.use();
    glm::mat4 lcd = glm::mat4(1.0f);
    lcd = glm::translate(lcd, glm::vec3(0.34f, deskH + 0.036f, -0.04f));
    lcd = glm::scale(lcd, glm::vec3(0.06f, 0.005f, 0.045f));
    ourShader.setMat4("model", deskMat * lcd);
    ourShader.setVec3("color", glm::vec3(0.70f, 0.85f, 0.78f)); // LCD greenish-grey
    glBindVertexArray(cubeVAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

    // Multimeter Rotary Selection Dial
    lightingShader.use();
    glm::mat4 dial = glm::mat4(1.0f);
    dial = glm::translate(dial, glm::vec3(0.35f, deskH + 0.036f, 0.02f));
    dial = glm::scale(dial, glm::vec3(0.04f, 0.012f, 0.04f));
    drawCube(cubeVAO, lightingShader, deskMat * dial, 0.12f, 0.12f, 0.14f);

    // Probe Cables (Red and Black leads)
    glm::mat4 probeRed = glm::mat4(1.0f);
    probeRed = glm::translate(probeRed, glm::vec3(0.26f, deskH + 0.005f, 0.06f));
    probeRed = glm::scale(probeRed, glm::vec3(0.06f, 0.01f, 0.015f));
    drawCube(cubeVAO, lightingShader, deskMat * probeRed, 0.85f, 0.15f, 0.15f); // Red probe wire

    glm::mat4 probeBlack = glm::mat4(1.0f);
    probeBlack = glm::translate(probeBlack, glm::vec3(0.26f, deskH + 0.005f, 0.085f));
    probeBlack = glm::scale(probeBlack, glm::vec3(0.06f, 0.01f, 0.015f));
    drawCube(cubeVAO, lightingShader, deskMat * probeBlack, 0.15f, 0.15f, 0.15f); // Black probe wire
}

// -------------------------------------------------------------------------
// Table 3 (Right Column, Second Desk): Robotic Quadruped Spider Bot & Prototyping Board with OLED Display
// -------------------------------------------------------------------------
void drawSpiderBotTable4(unsigned int& cubeVAO, Shader& lightingShader, Shader& ourShader, glm::mat4 deskMat)
{
    float deskH = 0.72f;

    // 1. Robotic Quadruped / Spider Bot (Positioned at X = 0.03f to 0.29f, no overlap with laptop at X <= -0.14f)
    lightingShader.use();
    // Central Main Body Chassis (Dark graphite & cyan)
    glm::mat4 body = glm::mat4(1.0f);
    body = glm::translate(body, glm::vec3(0.10f, deskH + 0.045f, -0.05f));
    body = glm::scale(body, glm::vec3(0.12f, 0.04f, 0.12f));
    drawCube(cubeVAO, lightingShader, deskMat * body, 0.18f, 0.20f, 0.24f, 0.5f, 32.0f);

    // Cyan accent racing stripe on body
    glm::mat4 stripe = glm::mat4(1.0f);
    stripe = glm::translate(stripe, glm::vec3(0.14f, deskH + 0.086f, -0.05f));
    stripe = glm::scale(stripe, glm::vec3(0.04f, 0.005f, 0.12f));
    drawCube(cubeVAO, lightingShader, deskMat * stripe, 0.0f, 0.85f, 0.95f);

    // Glowing Blue Core Beacon
    ourShader.use();
    glm::mat4 core = glm::mat4(1.0f);
    core = glm::translate(core, glm::vec3(0.15f, deskH + 0.088f, -0.01f));
    core = glm::scale(core, glm::vec3(0.02f, 0.012f, 0.02f));
    ourShader.setMat4("model", deskMat * core);
    ourShader.setVec3("color", glm::vec3(0.1f, 0.8f, 1.0f)); // Glowing Cyan Core
    glBindVertexArray(cubeVAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

    // 4 Articulated Spider Legs (Hip + Thigh + Foot)
    lightingShader.use();
    // Leg 1: Front-Left
    glm::mat4 leg1_hip = glm::mat4(1.0f);
    leg1_hip = glm::translate(leg1_hip, glm::vec3(0.05f, deskH + 0.055f, -0.08f));
    leg1_hip = glm::scale(leg1_hip, glm::vec3(0.05f, 0.02f, 0.025f));
    drawCube(cubeVAO, lightingShader, deskMat * leg1_hip, 0.12f, 0.12f, 0.14f);

    glm::mat4 leg1_foot = glm::mat4(1.0f);
    leg1_foot = glm::translate(leg1_foot, glm::vec3(0.03f, deskH, -0.09f));
    leg1_foot = glm::scale(leg1_foot, glm::vec3(0.02f, 0.06f, 0.02f));
    drawCube(cubeVAO, lightingShader, deskMat * leg1_foot, 0.0f, 0.85f, 0.95f); // Cyan leg segment

    // Leg 2: Front-Right
    glm::mat4 leg2_hip = glm::mat4(1.0f);
    leg2_hip = glm::translate(leg2_hip, glm::vec3(0.22f, deskH + 0.055f, -0.08f));
    leg2_hip = glm::scale(leg2_hip, glm::vec3(0.05f, 0.02f, 0.025f));
    drawCube(cubeVAO, lightingShader, deskMat * leg2_hip, 0.12f, 0.12f, 0.14f);

    glm::mat4 leg2_foot = glm::mat4(1.0f);
    leg2_foot = glm::translate(leg2_foot, glm::vec3(0.27f, deskH, -0.09f));
    leg2_foot = glm::scale(leg2_foot, glm::vec3(0.02f, 0.06f, 0.02f));
    drawCube(cubeVAO, lightingShader, deskMat * leg2_foot, 0.0f, 0.85f, 0.95f);

    // Leg 3: Rear-Left
    glm::mat4 leg3_hip = glm::mat4(1.0f);
    leg3_hip = glm::translate(leg3_hip, glm::vec3(0.05f, deskH + 0.055f, 0.035f));
    leg3_hip = glm::scale(leg3_hip, glm::vec3(0.05f, 0.02f, 0.025f));
    drawCube(cubeVAO, lightingShader, deskMat * leg3_hip, 0.12f, 0.12f, 0.14f);

    glm::mat4 leg3_foot = glm::mat4(1.0f);
    leg3_foot = glm::translate(leg3_foot, glm::vec3(0.03f, deskH, 0.04f));
    leg3_foot = glm::scale(leg3_foot, glm::vec3(0.02f, 0.06f, 0.02f));
    drawCube(cubeVAO, lightingShader, deskMat * leg3_foot, 0.0f, 0.85f, 0.95f);

    // Leg 4: Rear-Right
    glm::mat4 leg4_hip = glm::mat4(1.0f);
    leg4_hip = glm::translate(leg4_hip, glm::vec3(0.22f, deskH + 0.055f, 0.035f));
    leg4_hip = glm::scale(leg4_hip, glm::vec3(0.05f, 0.02f, 0.025f));
    drawCube(cubeVAO, lightingShader, deskMat * leg4_hip, 0.12f, 0.12f, 0.14f);

    glm::mat4 leg4_foot = glm::mat4(1.0f);
    leg4_foot = glm::translate(leg4_foot, glm::vec3(0.27f, deskH, 0.04f));
    leg4_foot = glm::scale(leg4_foot, glm::vec3(0.02f, 0.06f, 0.02f));
    drawCube(cubeVAO, lightingShader, deskMat * leg4_foot, 0.0f, 0.85f, 0.95f);

    // Top Mini Pan-Tilt Camera Head
    glm::mat4 camHead = glm::mat4(1.0f);
    camHead = glm::translate(camHead, glm::vec3(0.14f, deskH + 0.088f, -0.065f));
    camHead = glm::scale(camHead, glm::vec3(0.04f, 0.035f, 0.035f));
    drawCube(cubeVAO, lightingShader, deskMat * camHead, 0.10f, 0.10f, 0.12f);

    // 2. Embedded IoT Prototyping Board with Glowing OLED Display (Positioned at X = 0.33f to 0.46f)
    glm::mat4 iotBoard = glm::mat4(1.0f);
    iotBoard = glm::translate(iotBoard, glm::vec3(0.33f, deskH, -0.08f));
    iotBoard = glm::scale(iotBoard, glm::vec3(0.13f, 0.014f, 0.10f));
    drawCube(cubeVAO, lightingShader, deskMat * iotBoard, 0.08f, 0.50f, 0.20f); // Forest green PCB

    // Metal RF Shielding Can (ESP32 module)
    glm::mat4 rfCan = glm::mat4(1.0f);
    rfCan = glm::translate(rfCan, glm::vec3(0.35f, deskH + 0.014f, -0.03f));
    rfCan = glm::scale(rfCan, glm::vec3(0.045f, 0.015f, 0.045f));
    drawCube(cubeVAO, lightingShader, deskMat * rfCan, 0.80f, 0.82f, 0.85f, 0.8f, 64.0f);

    // Glowing 0.96" OLED Display Screen (Rendered with unlit ourShader)
    ourShader.use();
    glm::mat4 oled = glm::mat4(1.0f);
    oled = glm::translate(oled, glm::vec3(0.40f, deskH + 0.016f, -0.07f));
    oled = glm::scale(oled, glm::vec3(0.045f, 0.008f, 0.035f));
    ourShader.setMat4("model", deskMat * oled);
    ourShader.setVec3("color", glm::vec3(0.15f, 0.65f, 1.0f)); // Bright Electric Blue OLED Screen!
    glBindVertexArray(cubeVAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

    // Rechargeable LiPo Battery Pack
    lightingShader.use();
    glm::mat4 lipo = glm::mat4(1.0f);
    lipo = glm::translate(lipo, glm::vec3(0.34f, deskH, 0.05f));
    lipo = glm::scale(lipo, glm::vec3(0.09f, 0.022f, 0.05f));
    drawCube(cubeVAO, lightingShader, deskMat * lipo, 0.85f, 0.85f, 0.85f, 0.7f, 32.0f); // Silver pouch cell
}

// -------------------------------------------------------------------------
// Table 4 (Right Column, Third Desk): Autonomous Mobile Robotics Rover (Line Follower / Obstacle Avoidance)
// -------------------------------------------------------------------------
void drawMobileRoverTable3(unsigned int& cubeVAO, Shader& lightingShader, Shader& ourShader, glm::mat4 deskMat)
{
    float deskH = 0.72f;

    // 1. Mobile Robot Car Platform (Positioned at X = 0.045f to 0.255f, no overlap with laptop at X <= -0.14f)
    lightingShader.use();
    // Lower Acrylic Chassis Plate
    glm::mat4 plate = glm::mat4(1.0f);
    plate = glm::translate(plate, glm::vec3(0.07f, deskH + 0.025f, -0.10f));
    plate = glm::scale(plate, glm::vec3(0.16f, 0.010f, 0.19f));
    drawCube(cubeVAO, lightingShader, deskMat * plate, 0.15f, 0.25f, 0.38f, 0.6f, 64.0f); // Dark translucent blue acrylic

    // Left Drive Wheel (Rubber Tire)
    glm::mat4 leftWheel = glm::mat4(1.0f);
    leftWheel = glm::translate(leftWheel, glm::vec3(0.045f, deskH, 0.0f));
    leftWheel = glm::scale(leftWheel, glm::vec3(0.025f, 0.065f, 0.065f));
    drawCube(cubeVAO, lightingShader, deskMat * leftWheel, 0.12f, 0.12f, 0.14f); // Black rubber tire

    // Left Yellow Motor Gearbox
    glm::mat4 leftMotor = glm::mat4(1.0f);
    leftMotor = glm::translate(leftMotor, glm::vec3(0.075f, deskH + 0.015f, 0.01f));
    leftMotor = glm::scale(leftMotor, glm::vec3(0.035f, 0.025f, 0.045f));
    drawCube(cubeVAO, lightingShader, deskMat * leftMotor, 0.95f, 0.80f, 0.10f); // Yellow TT motor

    // Right Drive Wheel (Rubber Tire)
    glm::mat4 rightWheel = glm::mat4(1.0f);
    rightWheel = glm::translate(rightWheel, glm::vec3(0.23f, deskH, 0.0f));
    rightWheel = glm::scale(rightWheel, glm::vec3(0.025f, 0.065f, 0.065f));
    drawCube(cubeVAO, lightingShader, deskMat * rightWheel, 0.12f, 0.12f, 0.14f);

    // Right Yellow Motor Gearbox
    glm::mat4 rightMotor = glm::mat4(1.0f);
    rightMotor = glm::translate(rightMotor, glm::vec3(0.195f, deskH + 0.015f, 0.01f));
    rightMotor = glm::scale(rightMotor, glm::vec3(0.035f, 0.025f, 0.045f));
    drawCube(cubeVAO, lightingShader, deskMat * rightMotor, 0.95f, 0.80f, 0.10f);

    // Front Caster Roller
    glm::mat4 caster = glm::mat4(1.0f);
    caster = glm::translate(caster, glm::vec3(0.135f, deskH, -0.08f));
    caster = glm::scale(caster, glm::vec3(0.03f, 0.025f, 0.03f));
    drawCube(cubeVAO, lightingShader, deskMat * caster, 0.85f, 0.75f, 0.25f); // Brass caster ball

    // Battery Holder Box (4x AA black box on upper deck)
    glm::mat4 battBox = glm::mat4(1.0f);
    battBox = glm::translate(battBox, glm::vec3(0.09f, deskH + 0.035f, 0.01f));
    battBox = glm::scale(battBox, glm::vec3(0.11f, 0.030f, 0.07f));
    drawCube(cubeVAO, lightingShader, deskMat * battBox, 0.15f, 0.15f, 0.15f);

    // Red Motor Driver Shield (L298N module)
    glm::mat4 driver = glm::mat4(1.0f);
    driver = glm::translate(driver, glm::vec3(0.09f, deskH + 0.035f, -0.06f));
    driver = glm::scale(driver, glm::vec3(0.07f, 0.015f, 0.06f));
    drawCube(cubeVAO, lightingShader, deskMat * driver, 0.80f, 0.15f, 0.15f); // Red driver board

    // Black Finned Aluminum Heat Sink on motor driver
    glm::mat4 heatSink = glm::mat4(1.0f);
    heatSink = glm::translate(heatSink, glm::vec3(0.105f, deskH + 0.05f, -0.05f));
    heatSink = glm::scale(heatSink, glm::vec3(0.04f, 0.035f, 0.04f));
    drawCube(cubeVAO, lightingShader, deskMat * heatSink, 0.10f, 0.10f, 0.10f, 0.8f, 64.0f);

    // Front Bumper Obstacle Sensor (Ultrasonic eyes looking forward)
    glm::mat4 eyes = glm::mat4(1.0f);
    eyes = glm::translate(eyes, glm::vec3(0.10f, deskH + 0.04f, -0.11f));
    eyes = glm::scale(eyes, glm::vec3(0.10f, 0.03f, 0.015f));
    drawCube(cubeVAO, lightingShader, deskMat * eyes, 0.75f, 0.78f, 0.82f);

    // Glowing Power Indicator on Rover
    ourShader.use();
    glm::mat4 roverLed = glm::mat4(1.0f);
    roverLed = glm::translate(roverLed, glm::vec3(0.16f, deskH + 0.045f, -0.03f));
    roverLed = glm::scale(roverLed, glm::vec3(0.012f, 0.012f, 0.012f));
    ourShader.setMat4("model", deskMat * roverLed);
    ourShader.setVec3("color", glm::vec3(0.1f, 0.95f, 0.2f)); // Glowing Green LED
    glBindVertexArray(cubeVAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

    // 2. Hardware / Electronics Component Organizer Box (Positioned at X = 0.32f to 0.47f)
    lightingShader.use();
    glm::mat4 box = glm::mat4(1.0f);
    box = glm::translate(box, glm::vec3(0.32f, deskH, -0.06f));
    box = glm::scale(box, glm::vec3(0.15f, 0.030f, 0.14f));
    drawCube(cubeVAO, lightingShader, deskMat * box, 0.35f, 0.50f, 0.65f, 0.6f, 32.0f); // Translucent blue parts box

    // Compartment dividers
    glm::mat4 div1 = glm::mat4(1.0f);
    div1 = glm::translate(div1, glm::vec3(0.39f, deskH + 0.015f, -0.06f));
    div1 = glm::scale(div1, glm::vec3(0.012f, 0.025f, 0.14f));
    drawCube(cubeVAO, lightingShader, deskMat * div1, 0.25f, 0.40f, 0.55f);
}

// 5. Teacher's Podium with Laptop/Monitor
void drawTeacherPodium(unsigned int& cubeVAO, Shader& lightingShader, Shader& ourShader, glm::mat4 podiumBase)
{
    float podW = 2.0f;
    float podD = 0.9f;
    float podH = 0.85f;

    float pr = 0.70f, pg = 0.40f, pb = 0.22f; // Wood body

    // Front Modesty Panel (Facing the students)
    glm::mat4 frontPanel = glm::mat4(1.0f);
    frontPanel = glm::translate(frontPanel, glm::vec3(-podW / 2.0f, 0.0f, podD / 2.0f - 0.05f));
    frontPanel = glm::scale(frontPanel, glm::vec3(podW, podH, 0.05f));
    drawCube(cubeVAO, lightingShader, podiumBase * frontPanel, pr, pg, pb);

    // Left Side Panel
    glm::mat4 leftSide = glm::mat4(1.0f);
    leftSide = glm::translate(leftSide, glm::vec3(-podW / 2.0f, 0.0f, -podD / 2.0f));
    leftSide = glm::scale(leftSide, glm::vec3(0.05f, podH, podD));
    drawCube(cubeVAO, lightingShader, podiumBase * leftSide, pr, pg, pb);

    // Right Side Panel
    glm::mat4 rightSide = glm::mat4(1.0f);
    rightSide = glm::translate(rightSide, glm::vec3(podW / 2.0f - 0.05f, 0.0f, -podD / 2.0f));
    rightSide = glm::scale(rightSide, glm::vec3(0.05f, podH, podD));
    drawCube(cubeVAO, lightingShader, podiumBase * rightSide, pr, pg, pb);

    // Desktop Surface (Slight overhang)
    glm::mat4 top = glm::mat4(1.0f);
    top = glm::translate(top, glm::vec3(-podW / 2.0f - 0.05f, podH, -podD / 2.0f - 0.05f));
    top = glm::scale(top, glm::vec3(podW + 0.1f, 0.06f, podD + 0.1f));
    drawCube(cubeVAO, lightingShader, podiumBase * top, 0.76f, 0.46f, 0.25f, 0.4f, 48.0f);

    // --- Teacher's Laptop (Oriented facing the teacher, illuminated IDE screen) ---
    drawLaptop(cubeVAO, lightingShader, ourShader, podiumBase, 0.0f, -0.08f, podH + 0.06f, 180.0f, 1.25f);

    // Teacher's Chair (Rotated 180 degrees so it faces the desk and classroom)
    glm::mat4 tChair = glm::translate(podiumBase, glm::vec3(0.0f, 0.0f, -0.65f));
    tChair = glm::rotate(tChair, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    drawStudentChair(cubeVAO, lightingShader, tChair);
}

// 6. Classroom Front Board: Green Chalkboard / Whiteboard (Projector Screen)
void drawBlackboard(unsigned int& cubeVAO, Shader& lightingShader, Shader& ourShader, glm::mat4 boardBase, bool isWhiteboard)
{
    float bWidth = 4.4f;
    float bHeight = 2.1f;
    float posY = 1.4f;
    float posZ = -5.98f;

    // 1. Outer Border Frame
    glm::mat4 frame = glm::mat4(1.0f);
    frame = glm::translate(frame, glm::vec3(-bWidth / 2.0f, posY, posZ));
    frame = glm::scale(frame, glm::vec3(bWidth, bHeight, 0.04f));
    drawCube(cubeVAO, lightingShader, boardBase * frame, 0.45f, 0.25f, 0.12f);

    // 2. Board Surface (Switches between classic Green Chalkboard and pure Whiteboard / Projector Screen)
    float innerW = bWidth - 0.20f;
    float innerH = bHeight - 0.20f;
    glm::mat4 board = glm::mat4(1.0f);
    board = glm::translate(board, glm::vec3(-innerW / 2.0f, posY + 0.10f, posZ + 0.02f));
    board = glm::scale(board, glm::vec3(innerW, innerH, 0.03f));

    if (isWhiteboard) {
        // Pure White Board Surface (Projector ON)
        drawCube(cubeVAO, lightingShader, boardBase * board, 0.96f, 0.97f, 0.99f, 0.35f, 48.0f);
    } else {
        // Classic Dark Green Chalkboard Surface (Projector OFF)
        drawCube(cubeVAO, lightingShader, boardBase * board, 0.10f, 0.28f, 0.18f, 0.20f, 16.0f);
    }

    // 3. Accessory Shelf / Ledge along the bottom (Identical for both modes)
    glm::mat4 ledge = glm::mat4(1.0f);
    ledge = glm::translate(ledge, glm::vec3(-innerW / 2.0f, posY + 0.06f, posZ + 0.04f));
    ledge = glm::scale(ledge, glm::vec3(innerW, 0.04f, 0.10f));
    drawCube(cubeVAO, lightingShader, boardBase * ledge, 0.45f, 0.25f, 0.12f);

    // 4. Exactly ONE Duster (Eraser) on the shelf for both modes
    glm::mat4 duster = glm::mat4(1.0f);
    duster = glm::translate(duster, glm::vec3(0.20f, posY + 0.10f, posZ + 0.06f));
    duster = glm::scale(duster, glm::vec3(0.18f, 0.035f, 0.06f));
    drawCube(cubeVAO, lightingShader, boardBase * duster, 0.60f, 0.35f, 0.15f);

    // Duster bottom felt pad
    glm::mat4 dusterFelt = glm::mat4(1.0f);
    dusterFelt = glm::translate(dusterFelt, glm::vec3(0.20f, posY + 0.095f, posZ + 0.06f));
    dusterFelt = glm::scale(dusterFelt, glm::vec3(0.18f, 0.005f, 0.06f));
    drawCube(cubeVAO, lightingShader, boardBase * dusterFelt, 0.25f, 0.25f, 0.25f);

    // 5. Exactly ONE Marker on the shelf for both modes
    // Marker body
    glm::mat4 marker = glm::mat4(1.0f);
    marker = glm::translate(marker, glm::vec3(-0.25f, posY + 0.10f, posZ + 0.065f));
    marker = glm::scale(marker, glm::vec3(0.13f, 0.022f, 0.022f));
    drawCube(cubeVAO, lightingShader, boardBase * marker, 0.92f, 0.92f, 0.94f);

    // Marker cap / tip accent (Black cap)
    glm::mat4 markerCap = glm::mat4(1.0f);
    markerCap = glm::translate(markerCap, glm::vec3(-0.135f, posY + 0.10f, posZ + 0.065f));
    markerCap = glm::scale(markerCap, glm::vec3(0.025f, 0.024f, 0.024f));
    drawCube(cubeVAO, lightingShader, boardBase * markerCap, 0.15f, 0.15f, 0.15f);
}

// 7. Wall Clock with Animated Clockwise Second Hand
void drawWallClock(unsigned int& cubeVAO, Shader& lightingShader, glm::mat4 clockBase, float clockSecondAngle)
{
    float posX = 3.2f;
    float posY = 2.6f;
    float posZ = -5.98f;
    float size = 0.70f;

    // Square Outer Wooden Frame
    glm::mat4 frame = glm::mat4(1.0f);
    frame = glm::translate(frame, glm::vec3(posX - size / 2.0f, posY - size / 2.0f, posZ));
    frame = glm::scale(frame, glm::vec3(size, size, 0.04f));
    drawCube(cubeVAO, lightingShader, clockBase * frame, 0.42f, 0.24f, 0.12f);

    // Inner White Clock Face
    float faceSize = size - 0.14f;
    glm::mat4 face = glm::mat4(1.0f);
    face = glm::translate(face, glm::vec3(posX - faceSize / 2.0f, posY - faceSize / 2.0f, posZ + 0.02f));
    face = glm::scale(face, glm::vec3(faceSize, faceSize, 0.03f));
    drawCube(cubeVAO, lightingShader, clockBase * face, 0.95f, 0.95f, 0.92f);

    // Center Black Pin
    glm::mat4 pin = glm::mat4(1.0f);
    pin = glm::translate(pin, glm::vec3(posX - 0.02f, posY - 0.02f, posZ + 0.045f));
    pin = glm::scale(pin, glm::vec3(0.04f, 0.04f, 0.02f));
    drawCube(cubeVAO, lightingShader, clockBase * pin, 0.05f, 0.05f, 0.05f);

    // Black Clock Hands (Displaying 3:00 / "L" pattern as in Classroom.jpg)
    // Minute Hand (Pointing up to 12)
    glm::mat4 minHand = glm::mat4(1.0f);
    minHand = glm::translate(minHand, glm::vec3(posX - 0.015f, posY, posZ + 0.04f));
    minHand = glm::scale(minHand, glm::vec3(0.03f, 0.18f, 0.015f));
    drawCube(cubeVAO, lightingShader, clockBase * minHand, 0.05f, 0.05f, 0.05f);

    // Hour Hand (Pointing right to 3)
    glm::mat4 hourHand = glm::mat4(1.0f);
    hourHand = glm::translate(hourHand, glm::vec3(posX, posY - 0.015f, posZ + 0.04f));
    hourHand = glm::scale(hourHand, glm::vec3(0.13f, 0.03f, 0.015f));
    drawCube(cubeVAO, lightingShader, clockBase * hourHand, 0.05f, 0.05f, 0.05f);

    // Rotating Second Hand (Bright red needle rotating CLOCKWISE around Z-axis)
    glm::mat4 secHandPivot = glm::translate(clockBase, glm::vec3(posX, posY, posZ + 0.055f));
    secHandPivot = glm::rotate(secHandPivot, glm::radians(clockSecondAngle), glm::vec3(0.0f, 0.0f, 1.0f));

    glm::mat4 secHand = glm::mat4(1.0f);
    secHand = glm::translate(secHand, glm::vec3(-0.007f, -0.04f, 0.0f)); // slight tail behind center pivot
    secHand = glm::scale(secHand, glm::vec3(0.014f, 0.23f, 0.014f));
    drawCube(cubeVAO, lightingShader, secHandPivot * secHand, 0.88f, 0.12f, 0.12f);
}

// 8. Requirement 3: Moving Object - Rotating Ceiling Fan
void drawCeilingFan(unsigned int& cubeVAO, Shader& lightingShader, glm::mat4 fanBase, float fanAngle)
{
    float fr = 0.24f, fg = 0.16f, fb = 0.10f; // Deep mahogany brown

    // Ceiling Mount Canopy
    glm::mat4 canopy = glm::mat4(1.0f);
    canopy = glm::translate(canopy, glm::vec3(-0.10f, -0.05f, -0.10f));
    canopy = glm::scale(canopy, glm::vec3(0.20f, 0.05f, 0.20f));
    drawCube(cubeVAO, lightingShader, fanBase * canopy, fr, fg, fb);

    // Stationary Downrod Hanging from Ceiling
    glm::mat4 rod = glm::mat4(1.0f);
    rod = glm::translate(rod, glm::vec3(-0.035f, -0.45f, -0.035f));
    rod = glm::scale(rod, glm::vec3(0.07f, 0.40f, 0.07f));
    drawCube(cubeVAO, lightingShader, fanBase * rod, fr, fg, fb);

    // --- ROTATING ROTOR & BLADES ---
    // Rotates continuously about the vertical Y-axis
    glm::mat4 rotorMatrix = glm::translate(fanBase, glm::vec3(0.0f, -0.52f, 0.0f));
    rotorMatrix = glm::rotate(rotorMatrix, glm::radians(fanAngle), glm::vec3(0.0f, 1.0f, 0.0f));

    // Central Motor Hub
    glm::mat4 hub = glm::mat4(1.0f);
    hub = glm::translate(hub, glm::vec3(-0.16f, -0.06f, -0.16f));
    hub = glm::scale(hub, glm::vec3(0.32f, 0.12f, 0.32f));
    drawCube(cubeVAO, lightingShader, rotorMatrix * hub, fr, fg, fb);

    // 4 Symmetrical Fan Blades extending radially at 90-degree offsets
    float bladeLength = 1.15f;
    float bladeWidth = 0.18f;
    float bladeThick = 0.02f;

    for (int i = 0; i < 4; ++i) {
        glm::mat4 bladeMat = glm::rotate(rotorMatrix, glm::radians(i * 90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 blade = glm::mat4(1.0f);
        blade = glm::translate(blade, glm::vec3(0.16f, -0.02f, -bladeWidth / 2.0f));
        blade = glm::scale(blade, glm::vec3(bladeLength, bladeThick, bladeWidth));
        drawCube(cubeVAO, lightingShader, bladeMat * blade, 0.28f, 0.18f, 0.12f, 0.4f, 32.0f);
    }
}

// 9. Windows on Left Wall (Realistic Casing, Interior Sills, Sashes, Panes & Outdoor Scenery)
void drawWindow(unsigned int& cubeVAO, Shader& lightingShader, Shader& ourShader, glm::mat4 windowBase)
{
    float winZ[2] = { -4.8f, -1.0f }; // Start Z coordinates of the 2 window openings
    float winW = 2.4f;                 // Width of each window
    float winH = 1.8f;                 // Height of each window
    float winY = 1.4f;                 // Bottom sill Y level
    float winX = -5.0f;                // Wall X plane

    // Colors
    float fr = 0.94f, fg = 0.94f, fb = 0.95f; // Crisp white window frame & sashes
    float sr = 0.88f, sg = 0.85f, sb = 0.80f; // Light polished stone/wood interior sill
    float gr = 0.70f, gg = 0.86f, gb = 0.95f; // Daylight glass

    // -------------------------------------------------------------
    // A. Outdoor Scenery Backdrop (Outside the windows at X = -5.8f to -6.2f)
    // -------------------------------------------------------------
    // 1. Bright Sunny Blue Sky Panel (Rendered with unlit ourShader so it glows with daylight)
    ourShader.use();
    glm::mat4 sky = glm::mat4(1.0f);
    sky = glm::translate(sky, glm::vec3(-5.8f, 1.2f, -6.0f));
    sky = glm::scale(sky, glm::vec3(0.05f, 3.8f, 8.5f));
    ourShader.setMat4("model", windowBase * sky);
    ourShader.setVec3("color", glm::vec3(0.56f, 0.80f, 0.98f)); // Vivid sunny sky blue
    glBindVertexArray(cubeVAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

    // 2. Distant Sun / Horizon Glow
    glm::mat4 sun = glm::mat4(1.0f);
    sun = glm::translate(sun, glm::vec3(-5.75f, 3.2f, -4.5f));
    sun = glm::scale(sun, glm::vec3(0.04f, 1.2f, 2.0f));
    ourShader.setMat4("model", windowBase * sun);
    ourShader.setVec3("color", glm::vec3(1.0f, 0.98f, 0.85f)); // Warm daylight sun glow
    glBindVertexArray(cubeVAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

    // 3. Outdoor Ground Lawn / Grass (Positioned completely outside at X <= -5.35f to eliminate z-fighting)
    lightingShader.use();
    glm::mat4 grass = glm::mat4(1.0f);
    grass = glm::translate(grass, glm::vec3(-6.8f, 0.0f, -6.0f));
    grass = glm::scale(grass, glm::vec3(1.45f, 1.35f, 8.5f));
    drawCube(cubeVAO, lightingShader, windowBase * grass, 0.28f, 0.58f, 0.24f);

    // 4. Distant Trees / Foliage outside Window 1 & Window 2
    // Tree 1 (Outside Window 1)
    glm::mat4 trunk1 = glm::mat4(1.0f);
    trunk1 = glm::translate(trunk1, glm::vec3(-5.65f, 1.2f, -4.0f));
    trunk1 = glm::scale(trunk1, glm::vec3(0.08f, 0.7f, 0.16f));
    drawCube(cubeVAO, lightingShader, windowBase * trunk1, 0.35f, 0.22f, 0.14f);

    glm::mat4 foliage1 = glm::mat4(1.0f);
    foliage1 = glm::translate(foliage1, glm::vec3(-5.68f, 1.7f, -4.5f));
    foliage1 = glm::scale(foliage1, glm::vec3(0.12f, 1.1f, 1.2f));
    drawCube(cubeVAO, lightingShader, windowBase * foliage1, 0.22f, 0.52f, 0.20f);

    // Tree 2 (Outside Window 2)
    glm::mat4 trunk2 = glm::mat4(1.0f);
    trunk2 = glm::translate(trunk2, glm::vec3(-5.65f, 1.2f, 0.1f));
    trunk2 = glm::scale(trunk2, glm::vec3(0.08f, 0.7f, 0.16f));
    drawCube(cubeVAO, lightingShader, windowBase * trunk2, 0.35f, 0.22f, 0.14f);

    glm::mat4 foliage2 = glm::mat4(1.0f);
    foliage2 = glm::translate(foliage2, glm::vec3(-5.68f, 1.6f, -0.4f));
    foliage2 = glm::scale(foliage2, glm::vec3(0.12f, 1.2f, 1.3f));
    drawCube(cubeVAO, lightingShader, windowBase * foliage2, 0.18f, 0.48f, 0.18f);

    // -------------------------------------------------------------
    // B. Architectural Window Casings, Sills, and Glass Panes
    // -------------------------------------------------------------
    for (int i = 0; i < 2; ++i) {
        float z0 = winZ[i];

        // 1. Prominent Interior Window Sill Ledge (Protrudes nicely into the room)
        glm::mat4 sill = glm::mat4(1.0f);
        sill = glm::translate(sill, glm::vec3(winX - 0.06f, winY - 0.05f, z0 - 0.08f));
        sill = glm::scale(sill, glm::vec3(0.20f, 0.06f, winW + 0.16f));
        drawCube(cubeVAO, lightingShader, windowBase * sill, sr, sg, sb, 0.5f, 64.0f);

        // 2. Outer Frame Jambs & Header (Hollow perimeter casing)
        // Bottom Rail
        glm::mat4 bRail = glm::mat4(1.0f);
        bRail = glm::translate(bRail, glm::vec3(winX - 0.05f, winY, z0));
        bRail = glm::scale(bRail, glm::vec3(0.10f, 0.05f, winW));
        drawCube(cubeVAO, lightingShader, windowBase * bRail, fr, fg, fb);

        // Top Header
        glm::mat4 tRail = glm::mat4(1.0f);
        tRail = glm::translate(tRail, glm::vec3(winX - 0.05f, winY + winH - 0.05f, z0));
        tRail = glm::scale(tRail, glm::vec3(0.10f, 0.05f, winW));
        drawCube(cubeVAO, lightingShader, windowBase * tRail, fr, fg, fb);

        // Left Stile
        glm::mat4 lStile = glm::mat4(1.0f);
        lStile = glm::translate(lStile, glm::vec3(winX - 0.05f, winY, z0));
        lStile = glm::scale(lStile, glm::vec3(0.10f, winH, 0.05f));
        drawCube(cubeVAO, lightingShader, windowBase * lStile, fr, fg, fb);

        // Right Stile
        glm::mat4 rStile = glm::mat4(1.0f);
        rStile = glm::translate(rStile, glm::vec3(winX - 0.05f, winY, z0 + winW - 0.05f));
        rStile = glm::scale(rStile, glm::vec3(0.10f, winH, 0.05f));
        drawCube(cubeVAO, lightingShader, windowBase * rStile, fr, fg, fb);

        // 3. Sashes / Dividing Grids (6 Panes per window: 2 tiers x 3 columns)
        // Horizontal Transom Bar
        glm::mat4 hTransom = glm::mat4(1.0f);
        hTransom = glm::translate(hTransom, glm::vec3(winX - 0.04f, winY + winH * 0.55f - 0.02f, z0 + 0.05f));
        hTransom = glm::scale(hTransom, glm::vec3(0.08f, 0.04f, winW - 0.10f));
        drawCube(cubeVAO, lightingShader, windowBase * hTransom, fr, fg, fb);

        // 2 Vertical Mullion Bars
        for (int m = 1; m <= 2; ++m) {
            float mullionZ = z0 + (winW / 3.0f) * m - 0.02f;
            glm::mat4 vMullion = glm::mat4(1.0f);
            vMullion = glm::translate(vMullion, glm::vec3(winX - 0.04f, winY + 0.05f, mullionZ));
            vMullion = glm::scale(vMullion, glm::vec3(0.08f, winH - 0.10f, 0.04f));
            drawCube(cubeVAO, lightingShader, windowBase * vMullion, fr, fg, fb);
        }

        // 4. Glass Pane Layer
        glm::mat4 glass = glm::mat4(1.0f);
        glass = glm::translate(glass, glm::vec3(winX - 0.01f, winY + 0.05f, z0 + 0.05f));
        glass = glm::scale(glass, glm::vec3(0.02f, winH - 0.10f, winW - 0.10f));
        drawCube(cubeVAO, lightingShader, windowBase * glass, gr, gg, gb, 0.9f, 128.0f);
    }
}

// 10. Door on Right Wall (Wood Door, Glass Panel, Handle with Open/Close, Dynamic Color & Hallway Corridor)
void drawDoor(unsigned int& cubeVAO, Shader& lightingShader, Shader& ourShader, glm::mat4 doorBase, float doorAngle)
{
    float posX = 5.0f;
    float posY = 0.0f;
    float posZ = -4.0f;  // Opening spans Z in [-4.0f, -2.5f]
    float doorW = 1.5f;
    float doorH = 2.8f;

    // -------------------------------------------------------------
    // A. School Hallway / Corridor Outside the Doorway (X in [5.05f, 7.5f])
    // -------------------------------------------------------------
    // 1. Corridor Floor (Light Tiled Hallway Slab)
    glm::mat4 hallFloor = glm::mat4(1.0f);
    hallFloor = glm::translate(hallFloor, glm::vec3(posX + 0.05f, -0.1f, -5.5f));
    hallFloor = glm::scale(hallFloor, glm::vec3(2.5f, 0.1f, 4.5f));
    drawCube(cubeVAO, lightingShader, doorBase * hallFloor, 0.86f, 0.84f, 0.80f, 0.4f, 64.0f);

    // 2. Corridor Opposite Wall (Bright illuminated school corridor wall at X = 7.5f)
    glm::mat4 hallWall = glm::mat4(1.0f);
    hallWall = glm::translate(hallWall, glm::vec3(posX + 2.5f, 0.0f, -5.5f));
    hallWall = glm::scale(hallWall, glm::vec3(0.1f, 4.0f, 4.5f));
    drawCube(cubeVAO, lightingShader, doorBase * hallWall, 0.93f, 0.91f, 0.87f);

    // 3. Corridor Side Walls (Enclosing the hallway view)
    glm::mat4 hallSide1 = glm::mat4(1.0f);
    hallSide1 = glm::translate(hallSide1, glm::vec3(posX + 0.05f, 0.0f, -5.6f));
    hallSide1 = glm::scale(hallSide1, glm::vec3(2.5f, 4.0f, 0.1f));
    drawCube(cubeVAO, lightingShader, doorBase * hallSide1, 0.90f, 0.88f, 0.84f);

    glm::mat4 hallSide2 = glm::mat4(1.0f);
    hallSide2 = glm::translate(hallSide2, glm::vec3(posX + 0.05f, 0.0f, -1.0f));
    hallSide2 = glm::scale(hallSide2, glm::vec3(2.5f, 4.0f, 0.1f));
    drawCube(cubeVAO, lightingShader, doorBase * hallSide2, 0.90f, 0.88f, 0.84f);

    // 4. Corridor Ceiling
    glm::mat4 hallCeil = glm::mat4(1.0f);
    hallCeil = glm::translate(hallCeil, glm::vec3(posX + 0.05f, 4.0f, -5.5f));
    hallCeil = glm::scale(hallCeil, glm::vec3(2.5f, 0.1f, 4.5f));
    drawCube(cubeVAO, lightingShader, doorBase * hallCeil, 0.94f, 0.94f, 0.94f);

    // 5. Hallway Warm Emissive Light Fixture (Casts light into doorway)
    ourShader.use();
    glm::mat4 hallLight = glm::mat4(1.0f);
    hallLight = glm::translate(hallLight, glm::vec3(posX + 1.25f, 3.92f, -3.25f));
    hallLight = glm::scale(hallLight, glm::vec3(0.40f, 0.06f, 0.90f));
    ourShader.setMat4("model", doorBase * hallLight);
    ourShader.setVec3("color", glm::vec3(1.0f, 0.95f, 0.72f)); // Warm hallway glow
    glBindVertexArray(cubeVAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

    // 6. Opposite Classroom Door across the Hallway
    lightingShader.use();
    glm::mat4 oppDoor = glm::mat4(1.0f);
    oppDoor = glm::translate(oppDoor, glm::vec3(posX + 2.45f, 0.0f, -3.9f));
    oppDoor = glm::scale(oppDoor, glm::vec3(0.04f, 2.7f, 1.3f));
    drawCube(cubeVAO, lightingShader, doorBase * oppDoor, 0.48f, 0.28f, 0.15f);

    // -------------------------------------------------------------
    // B. Door Frame Casing (Jambs & Header framing the opening cleanly)
    // -------------------------------------------------------------
    float frameThick = 0.06f;
    float frameDepth = 0.14f;
    float frameX = posX - 0.02f;
    float fr = 0.36f, fg = 0.22f, fb = 0.12f; // Dark walnut frame wood

    // Left Jamb (at Z = posZ = -4.0f)
    glm::mat4 lJamb = glm::mat4(1.0f);
    lJamb = glm::translate(lJamb, glm::vec3(frameX, posY, posZ));
    lJamb = glm::scale(lJamb, glm::vec3(frameDepth, doorH, frameThick));
    drawCube(cubeVAO, lightingShader, doorBase * lJamb, fr, fg, fb);

    // Right Jamb (at Z = posZ + doorW - frameThick = -2.56f)
    glm::mat4 rJamb = glm::mat4(1.0f);
    rJamb = glm::translate(rJamb, glm::vec3(frameX, posY, posZ + doorW - frameThick));
    rJamb = glm::scale(rJamb, glm::vec3(frameDepth, doorH, frameThick));
    drawCube(cubeVAO, lightingShader, doorBase * rJamb, fr, fg, fb);

    // Top Header (at Y = doorH - frameThick = 2.74f)
    glm::mat4 tHeader = glm::mat4(1.0f);
    tHeader = glm::translate(tHeader, glm::vec3(frameX, posY + doorH - frameThick, posZ));
    tHeader = glm::scale(tHeader, glm::vec3(frameDepth, frameThick, doorW));
    drawCube(cubeVAO, lightingShader, doorBase * tHeader, fr, fg, fb);

    // -------------------------------------------------------------
    // C. Animated Door Leaf with Dynamic Open / Close Color
    // -------------------------------------------------------------
    // Hinge pivot at Z = posZ + doorW - frameThick = -2.56f, X = posX
    float hingeZ = posZ + doorW - frameThick;
    glm::mat4 doorHingeMat = glm::translate(doorBase, glm::vec3(posX, 0.0f, hingeZ));
    doorHingeMat = glm::rotate(doorHingeMat, glm::radians(doorAngle), glm::vec3(0.0f, 1.0f, 0.0f));

    // Dynamic Color Calculation:
    // When closed (0 deg): Classic deep rich mahogany classroom door
    // When open (85 deg): Illuminated warm golden honey-oak reflecting hallway daylight
    float openFactor = glm::clamp(doorAngle / 85.0f, 0.0f, 1.0f);
    glm::vec3 closedColor = glm::vec3(0.48f, 0.25f, 0.12f);
    glm::vec3 openColor   = glm::vec3(0.78f, 0.50f, 0.24f);
    glm::vec3 curDoorColor = glm::mix(closedColor, openColor, openFactor);

    float leafLen = doorW - 2.0f * frameThick; // approx 1.38f
    float leafH = doorH - frameThick - 0.01f;   // approx 2.73f
    float leafThick = 0.045f;

    // Main Door Panel
    glm::mat4 panel = glm::mat4(1.0f);
    panel = glm::translate(panel, glm::vec3(-leafThick / 2.0f, 0.01f, -leafLen));
    panel = glm::scale(panel, glm::vec3(leafThick, leafH, leafLen));
    drawCube(cubeVAO, lightingShader, doorHingeMat * panel, 
             curDoorColor.r, curDoorColor.g, curDoorColor.b, 0.4f, 32.0f);

    // Upper Glass Inspection Window with White Trim
    // Window frame
    glm::mat4 winTrim = glm::mat4(1.0f);
    winTrim = glm::translate(winTrim, glm::vec3(-leafThick / 2.0f - 0.005f, 1.40f, -leafLen * 0.65f - 0.18f));
    winTrim = glm::scale(winTrim, glm::vec3(leafThick + 0.01f, 0.70f, 0.38f));
    drawCube(cubeVAO, lightingShader, doorHingeMat * winTrim, 0.90f, 0.90f, 0.92f);

    // Window glass pane
    glm::mat4 doorGlass = glm::mat4(1.0f);
    doorGlass = glm::translate(doorGlass, glm::vec3(-leafThick / 2.0f - 0.008f, 1.45f, -leafLen * 0.65f - 0.14f));
    doorGlass = glm::scale(doorGlass, glm::vec3(leafThick + 0.016f, 0.60f, 0.30f));
    drawCube(cubeVAO, lightingShader, doorHingeMat * doorGlass, 0.55f, 0.78f, 0.92f, 0.8f, 128.0f);

    // Metallic Brass / Chrome Door Handle & Rosette Plate
    glm::mat4 handlePlate = glm::mat4(1.0f);
    handlePlate = glm::translate(handlePlate, glm::vec3(-leafThick / 2.0f - 0.015f, 1.10f, -leafLen + 0.08f));
    handlePlate = glm::scale(handlePlate, glm::vec3(leafThick + 0.03f, 0.16f, 0.06f));
    drawCube(cubeVAO, lightingShader, doorHingeMat * handlePlate, 0.82f, 0.78f, 0.40f, 0.9f, 128.0f);

    // Horizontal Lever
    glm::mat4 lever = glm::mat4(1.0f);
    lever = glm::translate(lever, glm::vec3(-leafThick / 2.0f - 0.06f, 1.16f, -leafLen + 0.08f));
    lever = glm::scale(lever, glm::vec3(leafThick + 0.12f, 0.03f, 0.12f));
    drawCube(cubeVAO, lightingShader, doorHingeMat * lever, 0.88f, 0.85f, 0.45f, 0.9f, 128.0f);
}

// 11. 3D Classroom Robot (Beside the Blackboard with Continuous Bye-Bye Waving Hand)
void drawRobot(unsigned int& cubeVAO, Shader& lightingShader, Shader& ourShader, glm::mat4 robotBase, float waveAngle)
{
    // Robot Color Palette
    float mr = 0.85f, mg = 0.88f, mb = 0.92f; // High-tech pearlescent silver/white body
    float dr = 0.22f, dg = 0.24f, db = 0.28f; // Dark graphite trim / joints
    float cr = 0.00f, cg = 0.90f, cb = 1.00f; // Electric cyan glowing visor / screen

    // 1. Mobile Wheeled / Tread Base Platform
    glm::mat4 base = glm::mat4(1.0f);
    base = glm::translate(base, glm::vec3(-0.30f, 0.0f, -0.25f));
    base = glm::scale(base, glm::vec3(0.60f, 0.12f, 0.50f));
    drawCube(cubeVAO, lightingShader, robotBase * base, dr, dg, db, 0.5f, 64.0f);

    // Tread rollers / side accent strips
    glm::mat4 leftTrack = glm::mat4(1.0f);
    leftTrack = glm::translate(leftTrack, glm::vec3(-0.33f, 0.01f, -0.27f));
    leftTrack = glm::scale(leftTrack, glm::vec3(0.08f, 0.10f, 0.54f));
    drawCube(cubeVAO, lightingShader, robotBase * leftTrack, 0.12f, 0.14f, 0.16f);

    glm::mat4 rightTrack = glm::mat4(1.0f);
    rightTrack = glm::translate(rightTrack, glm::vec3(0.25f, 0.01f, -0.27f));
    rightTrack = glm::scale(rightTrack, glm::vec3(0.08f, 0.10f, 0.54f));
    drawCube(cubeVAO, lightingShader, robotBase * rightTrack, 0.12f, 0.14f, 0.16f);

    // 2. Dual Robot Legs
    // Left Leg
    glm::mat4 lLeg = glm::mat4(1.0f);
    lLeg = glm::translate(lLeg, glm::vec3(-0.18f, 0.12f, -0.06f));
    lLeg = glm::scale(lLeg, glm::vec3(0.10f, 0.32f, 0.12f));
    drawCube(cubeVAO, lightingShader, robotBase * lLeg, dr, dg, db);

    // Right Leg
    glm::mat4 rLeg = glm::mat4(1.0f);
    rLeg = glm::translate(rLeg, glm::vec3(0.08f, 0.12f, -0.06f));
    rLeg = glm::scale(rLeg, glm::vec3(0.10f, 0.32f, 0.12f));
    drawCube(cubeVAO, lightingShader, robotBase * rLeg, dr, dg, db);

    // 3. Pelvis Joint
    glm::mat4 pelvis = glm::mat4(1.0f);
    pelvis = glm::translate(pelvis, glm::vec3(-0.22f, 0.44f, -0.10f));
    pelvis = glm::scale(pelvis, glm::vec3(0.44f, 0.08f, 0.20f));
    drawCube(cubeVAO, lightingShader, robotBase * pelvis, dr, dg, db);

    // 4. Main Torso / Body
    glm::mat4 torso = glm::mat4(1.0f);
    torso = glm::translate(torso, glm::vec3(-0.24f, 0.52f, -0.15f));
    torso = glm::scale(torso, glm::vec3(0.48f, 0.55f, 0.30f));
    drawCube(cubeVAO, lightingShader, robotBase * torso, mr, mg, mb, 0.6f, 64.0f);

    // Chest Interactive Screen (Unlit glowing panel with digital display)
    ourShader.use();
    glm::mat4 screen = glm::mat4(1.0f);
    screen = glm::translate(screen, glm::vec3(-0.16f, 0.65f, 0.152f));
    screen = glm::scale(screen, glm::vec3(0.32f, 0.24f, 0.01f));
    ourShader.setMat4("model", robotBase * screen);
    ourShader.setVec3("color", glm::vec3(0.08f, 0.12f, 0.18f)); // Dark screen glass
    glBindVertexArray(cubeVAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

    // Chest Status Indicator LEDs
    glm::mat4 led1 = glm::mat4(1.0f);
    led1 = glm::translate(led1, glm::vec3(-0.10f, 0.72f, 0.165f));
    led1 = glm::scale(led1, glm::vec3(0.05f, 0.05f, 0.01f));
    ourShader.setMat4("model", robotBase * led1);
    ourShader.setVec3("color", glm::vec3(0.1f, 0.95f, 0.2f)); // Glowing Green LED
    glBindVertexArray(cubeVAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

    glm::mat4 led2 = glm::mat4(1.0f);
    led2 = glm::translate(led2, glm::vec3(-0.02f, 0.72f, 0.165f));
    led2 = glm::scale(led2, glm::vec3(0.05f, 0.05f, 0.01f));
    ourShader.setMat4("model", robotBase * led2);
    ourShader.setVec3("color", glm::vec3(0.1f, 0.85f, 1.0f)); // Glowing Cyan LED
    glBindVertexArray(cubeVAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

    glm::mat4 led3 = glm::mat4(1.0f);
    led3 = glm::translate(led3, glm::vec3(0.06f, 0.72f, 0.165f));
    led3 = glm::scale(led3, glm::vec3(0.05f, 0.05f, 0.01f));
    ourShader.setMat4("model", robotBase * led3);
    ourShader.setVec3("color", glm::vec3(1.0f, 0.75f, 0.1f)); // Glowing Amber LED
    glBindVertexArray(cubeVAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

    // 5. Neck Joint
    lightingShader.use();
    glm::mat4 neck = glm::mat4(1.0f);
    neck = glm::translate(neck, glm::vec3(-0.07f, 1.07f, -0.06f));
    neck = glm::scale(neck, glm::vec3(0.14f, 0.08f, 0.12f));
    drawCube(cubeVAO, lightingShader, robotBase * neck, dr, dg, db);

    // 6. Robot Head
    glm::mat4 head = glm::mat4(1.0f);
    head = glm::translate(head, glm::vec3(-0.19f, 1.15f, -0.14f));
    head = glm::scale(head, glm::vec3(0.38f, 0.30f, 0.28f));
    drawCube(cubeVAO, lightingShader, robotBase * head, mr, mg, mb, 0.6f, 64.0f);

    // Ear bolts / sensors on sides of head
    glm::mat4 leftEar = glm::mat4(1.0f);
    leftEar = glm::translate(leftEar, glm::vec3(-0.22f, 1.25f, -0.04f));
    leftEar = glm::scale(leftEar, glm::vec3(0.04f, 0.10f, 0.08f));
    drawCube(cubeVAO, lightingShader, robotBase * leftEar, dr, dg, db);

    glm::mat4 rightEar = glm::mat4(1.0f);
    rightEar = glm::translate(rightEar, glm::vec3(0.18f, 1.25f, -0.04f));
    rightEar = glm::scale(rightEar, glm::vec3(0.04f, 0.10f, 0.08f));
    drawCube(cubeVAO, lightingShader, robotBase * rightEar, dr, dg, db);

    // Glowing Electric-Cyan Visor / Eyes (Rendered with unlit ourShader)
    ourShader.use();
    glm::mat4 visor = glm::mat4(1.0f);
    visor = glm::translate(visor, glm::vec3(-0.15f, 1.26f, 0.142f));
    visor = glm::scale(visor, glm::vec3(0.30f, 0.10f, 0.02f));
    ourShader.setMat4("model", robotBase * visor);
    ourShader.setVec3("color", glm::vec3(cr, cg, cb)); // Glowing Cyan Eye Visor!
    glBindVertexArray(cubeVAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

    // Antenna Mast
    lightingShader.use();
    glm::mat4 mast = glm::mat4(1.0f);
    mast = glm::translate(mast, glm::vec3(-0.015f, 1.45f, -0.015f));
    mast = glm::scale(mast, glm::vec3(0.03f, 0.16f, 0.03f));
    drawCube(cubeVAO, lightingShader, robotBase * mast, dr, dg, db);

    // Antenna Glowing Beacon Ball
    ourShader.use();
    glm::mat4 beacon = glm::mat4(1.0f);
    beacon = glm::translate(beacon, glm::vec3(-0.035f, 1.61f, -0.035f));
    beacon = glm::scale(beacon, glm::vec3(0.07f, 0.07f, 0.07f));
    ourShader.setMat4("model", robotBase * beacon);
    ourShader.setVec3("color", glm::vec3(1.0f, 0.25f, 0.25f)); // Pulsing red beacon tip
    glBindVertexArray(cubeVAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

    // -------------------------------------------------------------
    // 7. Left Arm (Resting peacefully at robot's side)
    // -------------------------------------------------------------
    lightingShader.use();
    // Shoulder Joint
    glm::mat4 lShoulder = glm::mat4(1.0f);
    lShoulder = glm::translate(lShoulder, glm::vec3(-0.31f, 0.94f, -0.04f));
    lShoulder = glm::scale(lShoulder, glm::vec3(0.07f, 0.07f, 0.08f));
    drawCube(cubeVAO, lightingShader, robotBase * lShoulder, dr, dg, db);

    // Left Upper Arm
    glm::mat4 lArm = glm::mat4(1.0f);
    lArm = glm::translate(lArm, glm::vec3(-0.30f, 0.68f, -0.03f));
    lArm = glm::scale(lArm, glm::vec3(0.06f, 0.26f, 0.06f));
    drawCube(cubeVAO, lightingShader, robotBase * lArm, mr, mg, mb);

    // Left Hand Gripper
    glm::mat4 lHand = glm::mat4(1.0f);
    lHand = glm::translate(lHand, glm::vec3(-0.30f, 0.58f, -0.03f));
    lHand = glm::scale(lHand, glm::vec3(0.06f, 0.10f, 0.06f));
    drawCube(cubeVAO, lightingShader, robotBase * lHand, dr, dg, db);

    // -------------------------------------------------------------
    // 8. Right Arm: CONTINUOUS "BYE-BYE" WAVING HAND (Moving Object!)
    // -------------------------------------------------------------
    // Shoulder Pivot: Positioned at right side of upper torso
    glm::vec3 rShoulderPos = glm::vec3(0.25f, 0.98f, 0.0f);
    glm::mat4 shoulderMat = glm::translate(robotBase, rShoulderPos);
    // Raise upper arm upward and slightly outward into waving posture
    shoulderMat = glm::rotate(shoulderMat, glm::radians(50.0f), glm::vec3(0.0f, 0.0f, -1.0f));
    shoulderMat = glm::rotate(shoulderMat, glm::radians(15.0f), glm::vec3(1.0f, 0.0f, 0.0f));

    // Right Shoulder Joint sphere/cube
    glm::mat4 rShoulderCube = glm::mat4(1.0f);
    rShoulderCube = glm::translate(rShoulderCube, glm::vec3(-0.035f, -0.035f, -0.035f));
    rShoulderCube = glm::scale(rShoulderCube, glm::vec3(0.07f, 0.07f, 0.07f));
    drawCube(cubeVAO, lightingShader, shoulderMat * rShoulderCube, dr, dg, db);

    // Right Upper Arm (extends outward)
    glm::mat4 rUpperArm = glm::mat4(1.0f);
    rUpperArm = glm::translate(rUpperArm, glm::vec3(-0.03f, 0.0f, -0.03f));
    rUpperArm = glm::scale(rUpperArm, glm::vec3(0.06f, 0.24f, 0.06f));
    drawCube(cubeVAO, lightingShader, shoulderMat * rUpperArm, mr, mg, mb);

    // Elbow Pivot: located at end of upper arm (Y = 0.24f)
    glm::mat4 elbowMat = glm::translate(shoulderMat, glm::vec3(0.0f, 0.24f, 0.0f));
    // Continuous Bye-Bye Waving: Waving hand oscillates back and forth!
    elbowMat = glm::rotate(elbowMat, glm::radians(waveAngle), glm::vec3(0.0f, 0.0f, 1.0f));

    // Elbow Joint
    glm::mat4 rElbowCube = glm::mat4(1.0f);
    rElbowCube = glm::translate(rElbowCube, glm::vec3(-0.035f, -0.035f, -0.035f));
    rElbowCube = glm::scale(rElbowCube, glm::vec3(0.07f, 0.07f, 0.07f));
    drawCube(cubeVAO, lightingShader, elbowMat * rElbowCube, dr, dg, db);

    // Right Forearm
    glm::mat4 rForearm = glm::mat4(1.0f);
    rForearm = glm::translate(rForearm, glm::vec3(-0.025f, 0.0f, -0.025f));
    rForearm = glm::scale(rForearm, glm::vec3(0.05f, 0.22f, 0.05f));
    drawCube(cubeVAO, lightingShader, elbowMat * rForearm, mr, mg, mb);

    // Wrist Pivot
    glm::mat4 wristMat = glm::translate(elbowMat, glm::vec3(0.0f, 0.22f, 0.0f));
    // Hand adds a friendly complementary wrist wave tilt
    wristMat = glm::rotate(wristMat, glm::radians(waveAngle * 0.4f), glm::vec3(0.0f, 0.0f, 1.0f));

    // Hand Palm
    glm::mat4 palm = glm::mat4(1.0f);
    palm = glm::translate(palm, glm::vec3(-0.04f, 0.0f, -0.02f));
    palm = glm::scale(palm, glm::vec3(0.08f, 0.07f, 0.04f));
    drawCube(cubeVAO, lightingShader, wristMat * palm, dr, dg, db);

    // 3 Articulated Waving Fingers (Giving bye-bye!)
    for (int f = 0; f < 3; ++f) {
        float fx = -0.035f + f * 0.028f;
        glm::mat4 finger = glm::mat4(1.0f);
        finger = glm::translate(finger, glm::vec3(fx, 0.07f, -0.015f));
        finger = glm::scale(finger, glm::vec3(0.02f, 0.06f, 0.03f));
        drawCube(cubeVAO, lightingShader, wristMat * finger, mr, mg, mb);
    }
}

// 11. Ceiling Point Light Fixture (Box casing with glowing emissive underside)
void drawCeilingLightFixture(unsigned int& cubeVAO, Shader& lightingShader, Shader& ourShader, 
                             glm::mat4 lightBase, bool isLightOn)
{
    // Dark brown/bronze fixture housing
    glm::mat4 casing = glm::mat4(1.0f);
    casing = glm::translate(casing, glm::vec3(-0.45f, -0.08f, -0.25f));
    casing = glm::scale(casing, glm::vec3(0.90f, 0.08f, 0.50f));
    drawCube(cubeVAO, lightingShader, lightBase * casing, 0.20f, 0.16f, 0.12f);

    // Glowing diffuser panel (rendered with unlit shader)
    ourShader.use();
    glm::mat4 diffuser = glm::mat4(1.0f);
    diffuser = glm::translate(diffuser, glm::vec3(-0.40f, -0.09f, -0.20f));
    diffuser = glm::scale(diffuser, glm::vec3(0.80f, 0.02f, 0.40f));
    ourShader.setMat4("model", lightBase * diffuser);

    if (isLightOn) {
        ourShader.setVec3("color", glm::vec3(1.0f, 0.88f, 0.50f)); // Warm yellow glow
    } else {
        ourShader.setVec3("color", glm::vec3(0.25f, 0.25f, 0.25f)); // Dark off
    }

    glBindVertexArray(cubeVAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
}

// 12. Classroom Ceiling Multimedia Digital Projector (Spotlight Source)
void drawSpotlightFixture(unsigned int& cubeVAO, Shader& lightingShader, Shader& ourShader, 
                          glm::mat4 fixtureBase, bool isSpotlightOn)
{
    // The fixtureBase is positioned at (0.0f, 3.85f, -3.2f)
    // 1. Ceiling Mount Flange & Extension Drop Pole (Reaching up to ceiling at Y = 4.0m)
    glm::mat4 pole = glm::mat4(1.0f);
    pole = glm::translate(pole, glm::vec3(-0.025f, 0.0f, -0.025f));
    pole = glm::scale(pole, glm::vec3(0.05f, 0.15f, 0.05f));
    drawCube(cubeVAO, lightingShader, fixtureBase * pole, 0.20f, 0.22f, 0.25f, 0.5f, 32.0f);

    glm::mat4 ceilingPlate = glm::mat4(1.0f);
    ceilingPlate = glm::translate(ceilingPlate, glm::vec3(-0.10f, 0.13f, -0.10f));
    ceilingPlate = glm::scale(ceilingPlate, glm::vec3(0.20f, 0.02f, 0.20f));
    drawCube(cubeVAO, lightingShader, fixtureBase * ceilingPlate, 0.15f, 0.17f, 0.20f);

    // 2. Projector Main Chassis (Sleek Matte White / Silver Body, tilted 25 deg towards the board)
    glm::mat4 projTilt = glm::mat4(1.0f);
    projTilt = glm::rotate(projTilt, glm::radians(25.0f), glm::vec3(1.0f, 0.0f, 0.0f));

    // Projector body: 38cm wide, 11cm high, 28cm deep
    glm::mat4 body = glm::translate(projTilt, glm::vec3(-0.19f, -0.11f, -0.14f));
    body = glm::scale(body, glm::vec3(0.38f, 0.11f, 0.28f));
    drawCube(cubeVAO, lightingShader, fixtureBase * body, 0.88f, 0.89f, 0.91f, 0.5f, 32.0f);

    // Ventilation Exhaust Grills on the side (Dark recessed slats)
    glm::mat4 ventLeft = glm::translate(projTilt, glm::vec3(-0.195f, -0.09f, -0.08f));
    ventLeft = glm::scale(ventLeft, glm::vec3(0.01f, 0.07f, 0.16f));
    drawCube(cubeVAO, lightingShader, fixtureBase * ventLeft, 0.15f, 0.15f, 0.16f);

    glm::mat4 ventRight = glm::translate(projTilt, glm::vec3(0.185f, -0.09f, -0.08f));
    ventRight = glm::scale(ventRight, glm::vec3(0.01f, 0.07f, 0.16f));
    drawCube(cubeVAO, lightingShader, fixtureBase * ventRight, 0.15f, 0.15f, 0.16f);

    // Projector Lens Barrel / Shroud (Front face, offset slightly right)
    glm::mat4 lensBarrel = glm::translate(projTilt, glm::vec3(0.04f, -0.085f, -0.17f));
    lensBarrel = glm::scale(lensBarrel, glm::vec3(0.10f, 0.075f, 0.04f));
    drawCube(cubeVAO, lightingShader, fixtureBase * lensBarrel, 0.15f, 0.15f, 0.18f);

    // Glowing Optical Projection Lens Element (Emissive front facing the board at -Z)
    ourShader.use();
    glm::mat4 lensGlass = glm::translate(projTilt, glm::vec3(0.05f, -0.075f, -0.175f));
    lensGlass = glm::scale(lensGlass, glm::vec3(0.08f, 0.055f, 0.01f));
    ourShader.setMat4("model", fixtureBase * lensGlass);

    if (isSpotlightOn) {
        ourShader.setVec3("color", glm::vec3(0.85f, 0.95f, 1.0f)); // Bright cyan-white projector light beam source
    } else {
        ourShader.setVec3("color", glm::vec3(0.12f, 0.15f, 0.20f)); // Dark glass when OFF
    }
    glBindVertexArray(cubeVAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

    // Projector Top Status LEDs (Power & Lamp indicator lights)
    glm::mat4 pwrLed = glm::translate(projTilt, glm::vec3(-0.12f, 0.002f, 0.06f));
    pwrLed = glm::scale(pwrLed, glm::vec3(0.015f, 0.005f, 0.015f));
    ourShader.setMat4("model", fixtureBase * pwrLed);
    if (isSpotlightOn) {
        ourShader.setVec3("color", glm::vec3(0.1f, 0.95f, 0.3f)); // Glowing Green Power LED
    } else {
        ourShader.setVec3("color", glm::vec3(0.85f, 0.2f, 0.1f)); // Amber/Red Standby LED
    }
    glBindVertexArray(cubeVAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
}

// =========================================================================
// Input Processing & Callbacks
// =========================================================================

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // ---------------------------------------------------------------------
    // 1. Camera Navigation using ONLY Keyboard Arrow Keys (Unchanged)
    // ---------------------------------------------------------------------
    float moveSpeed = camera.MovementSpeed;
    float turnSpeed = 65.0f; // degrees per second for smooth keyboard looking

    bool shiftPressed = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || 
                         glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);
    bool translateMode = (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS);
    bool scaleMode     = (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS);

    // If neither T (Translate) nor M (Scale) is held, arrow keys move the CAMERA:
    if (!translateMode && !scaleMode)
    {
        // Forward / Backward / Vertical Altitude Navigation
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        {
            if (shiftPressed) {
                // Shift + Up Arrow: Fly Upward
                camera.Position.y += moveSpeed * deltaTime;
            } else {
                camera.ProcessKeyboard(FORWARD, deltaTime);
            }
        }

        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        {
            if (shiftPressed) {
                // Shift + Down Arrow: Fly Downward
                camera.Position.y = std::max(0.2f, camera.Position.y - moveSpeed * deltaTime);
            } else {
                camera.ProcessKeyboard(BACKWARD, deltaTime);
            }
        }

        // Left / Right Turning & Strafing
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        {
            if (shiftPressed) {
                camera.ProcessKeyboard(LEFT, deltaTime);
            } else {
                camera.Yaw -= turnSpeed * deltaTime;
                camera.updateCameraVectors();
            }
        }

        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        {
            if (shiftPressed) {
                camera.ProcessKeyboard(RIGHT, deltaTime);
            } else {
                camera.Yaw += turnSpeed * deltaTime;
                camera.updateCameraVectors();
            }
        }
    }

    // ---------------------------------------------------------------------
    // 2. Interactive 3D Room Transformations (Mnemonic & Clean!)
    // ---------------------------------------------------------------------
    // Rotation: Keys X, Y, Z (Only active when NOT holding T for translation)
    float rotDir = shiftPressed ? -1.0f : 1.0f;
    if (!translateMode)
    {
        if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
        {
            rotateAngle_X += rotDir * 20.0f * deltaTime;
            rotateAxis_X = 1.0f; rotateAxis_Y = 0.0f; rotateAxis_Z = 0.0f;
        }
        if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS)
        {
            rotateAngle_Y += rotDir * 20.0f * deltaTime;
            rotateAxis_X = 0.0f; rotateAxis_Y = 1.0f; rotateAxis_Z = 0.0f;
        }
        if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
        {
            rotateAngle_Z += rotDir * 20.0f * deltaTime;
            rotateAxis_X = 0.0f; rotateAxis_Y = 0.0f; rotateAxis_Z = 1.0f;
        }
    }

    // Translation: T + Direction Keys (Super Simple - NO Complex Conditions!)
    // Hold T (Translate):
    //   - Up / Down Arrow    : Lift Room Upward (+Y) / Lower Downward (-Y)
    //   - Left / Right Arrow : Shift Room Left (-X) / Right (+X)
    //   - PageUp / PageDown  : Move Room Forward (+Z) / Backward (-Z)
    //   - (Also W / S)       : Move Room Forward (+Z) / Backward (-Z)
    if (translateMode)
    {
        // Vertical Translation (Y-axis)
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
            translate_Y += 1.5f * deltaTime; // T + Up: Lift Room Upward (+Y)
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
            translate_Y -= 1.5f * deltaTime; // T + Down: Lower Room Downward (-Y)

        // Horizontal Translation (X-axis)
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
            translate_X += 1.5f * deltaTime; // T + Right: Shift Room Right (+X)
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
            translate_X -= 1.5f * deltaTime; // T + Left: Shift Room Left (-X)

        // Depth Translation (Z-axis)
        if (glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            translate_Z += 1.5f * deltaTime; // T + PageUp or T + W: Forward / Closer (+Z)
        if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            translate_Z -= 1.5f * deltaTime; // T + PageDown or T + S: Backward / Receding (-Z)
    }

    // Scaling: M + Up / Down (M matches Magnify / Scale!)
    if (scaleMode)
    {
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        {
            scale_X += 0.5f * deltaTime;
            scale_Y += 0.5f * deltaTime;
            scale_Z += 0.5f * deltaTime;
        }
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        {
            scale_X = std::max(0.2f, scale_X - 0.5f * deltaTime);
            scale_Y = std::max(0.2f, scale_Y - 0.5f * deltaTime);
            scale_Z = std::max(0.2f, scale_Z - 0.5f * deltaTime);
        }
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action != GLFW_PRESS)
        return;

    // --- REQUIREMENT 2: THE 4 CORE CAMERA VIEWS (KEYS 1, 2, 3, 4) ---
    // Matches the 4 project screenshots: Image/front.png, back.png, left.png, right.png
    if (key == GLFW_KEY_1) setCameraPreset(VIEW_FRONT);    // Key 1: Front View (Image/front.png)
    if (key == GLFW_KEY_2) setCameraPreset(VIEW_BACK);     // Key 2: Back View (Image/back.png)
    if (key == GLFW_KEY_3) setCameraPreset(VIEW_LEFT);     // Key 3: Left View (Image/left.png)
    if (key == GLFW_KEY_4) setCameraPreset(VIEW_RIGHT);    // Key 4: Right View (Image/right.png)

    // --- RESET VIEW & TRANSFORMATION: KEY R ---
    // R matches Reset!
    if (key == GLFW_KEY_R)
    {
        setCameraPreset(VIEW_FRONT);
        rotateAngle_X = 0.0f; rotateAngle_Y = 0.0f; rotateAngle_Z = 0.0f;
        translate_X = 0.0f;   translate_Y = 0.0f;   translate_Z = 0.0f;
        scale_X = 1.0f;       scale_Y = 1.0f;       scale_Z = 1.0f;
        pointLightOn = true;
        pointLight1On = true; pointLight2On = true;
        pointLight3On = true; pointLight4On = true;
        pointlight1.turnOn(); pointlight2.turnOn();
        pointlight3.turnOn(); pointlight4.turnOn();
        isProjectorOn = false;
        blackboardSpotlight.turnOff();
        cout << "[Reset] Camera, Lights, Projector & Room Transformation reset to initial state" << endl;
    }

    // --- INTERACTIVE DOOR: KEY D ---
    // D matches Door!
    if (key == GLFW_KEY_D)
    {
        isDoorOpen = !isDoorOpen;
        cout << "[Door] " << (isDoorOpen ? "OPENING (Swinging Inward)" : "CLOSING") << endl;
    }

    // --- CEILING FAN: KEY F ---
    // F matches Fan!
    if (key == GLFW_KEY_F)
    {
        isFanOn = !isFanOn;
        cout << "[Ceiling Fan] " << (isFanOn ? "ROTATING (ON)" : "STOPPED (OFF)") << endl;
    }

    // --- FAN SPEED: KEYS + and - ---
    // + matches Plus (Increase) and - matches Minus (Decrease)
    if (key == GLFW_KEY_EQUAL || key == GLFW_KEY_KP_ADD)
    {
        fanSpeed += 60.0f;
        cout << "[Ceiling Fan] Speed increased to " << fanSpeed << " deg/s" << endl;
    }
    if (key == GLFW_KEY_MINUS || key == GLFW_KEY_KP_SUBTRACT)
    {
        fanSpeed = std::max(0.0f, fanSpeed - 60.0f);
        cout << "[Ceiling Fan] Speed decreased to " << fanSpeed << " deg/s" << endl;
    }

    // --- 4 DEDICATED KEYS FOR 4 POINT LIGHTS (KEYS 5, 6, 7, 8) ---
    // Key 5: Toggle Point Light 1 (Front-Left Ceiling Fixture)
    if (key == GLFW_KEY_5)
    {
        pointLight1On = !pointLight1On;
        if (pointLight1On) pointlight1.turnOn();
        else pointlight1.turnOff();
        pointLightOn = (pointLight1On || pointLight2On || pointLight3On || pointLight4On);
        cout << "[Point Light 1] Front-Left Ceiling Light: " << (pointLight1On ? "ON" : "OFF") << endl;
    }
    // Key 6: Toggle Point Light 2 (Front-Right Ceiling Fixture)
    if (key == GLFW_KEY_6)
    {
        pointLight2On = !pointLight2On;
        if (pointLight2On) pointlight2.turnOn();
        else pointlight2.turnOff();
        pointLightOn = (pointLight1On || pointLight2On || pointLight3On || pointLight4On);
        cout << "[Point Light 2] Front-Right Ceiling Light: " << (pointLight2On ? "ON" : "OFF") << endl;
    }
    // Key 7: Toggle Point Light 3 (Back-Left Ceiling Fixture)
    if (key == GLFW_KEY_7)
    {
        pointLight3On = !pointLight3On;
        if (pointLight3On) pointlight3.turnOn();
        else pointlight3.turnOff();
        pointLightOn = (pointLight1On || pointLight2On || pointLight3On || pointLight4On);
        cout << "[Point Light 3] Back-Left Ceiling Light: " << (pointLight3On ? "ON" : "OFF") << endl;
    }
    // Key 8: Toggle Point Light 4 (Back-Right Ceiling Fixture)
    if (key == GLFW_KEY_8)
    {
        pointLight4On = !pointLight4On;
        if (pointLight4On) pointlight4.turnOn();
        else pointlight4.turnOff();
        pointLightOn = (pointLight1On || pointLight2On || pointLight3On || pointLight4On);
        cout << "[Point Light 4] Back-Right Ceiling Light: " << (pointLight4On ? "ON" : "OFF") << endl;
    }

    // --- CEILING LIGHTS MASTER SWITCH: KEY L (UNCHANGED) ---
    // L matches Light (Ceiling point lights)!
    if (key == GLFW_KEY_L)
    {
        if (pointLightOn) {
            pointlight1.turnOff();
            pointlight2.turnOff();
            pointlight3.turnOff();
            pointlight4.turnOff();
            pointLightOn = false;
            pointLight1On = false;
            pointLight2On = false;
            pointLight3On = false;
            pointLight4On = false;
            cout << "[Point Lights] OFF" << endl;
        } else {
            pointlight1.turnOn();
            pointlight2.turnOn();
            pointlight3.turnOn();
            pointlight4.turnOn();
            pointLightOn = true;
            pointLight1On = true;
            pointLight2On = true;
            pointLight3On = true;
            pointLight4On = true;
            cout << "[Point Lights] ON" << endl;
        }
    }

    // --- PROJECTOR & WHITEBOARD SCREEN: KEY P (or S) ---
    // P matches Projector!
    // 1 single key controls both the projector spotlight AND changes the board color to white!
    if (key == GLFW_KEY_P || key == GLFW_KEY_S)
    {
        isProjectorOn = !isProjectorOn;
        if (isProjectorOn)
        {
            blackboardSpotlight.turnOn();
            cout << "[Projector & Screen] ON (Spotlight active, Board changed to Whiteboard / Projection Screen)" << endl;
        }
        else
        {
            blackboardSpotlight.turnOff();
            cout << "[Projector & Screen] OFF (Spotlight off, Board restored to Classic Green Chalkboard)" << endl;
        }
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    // Enable mouse look on left or right mouse button drag
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS ||
        glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
    {
        camera.ProcessMouseMovement(xoffset, yoffset);
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}
