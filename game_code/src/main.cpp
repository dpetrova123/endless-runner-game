#define _USE_MATH_DEFINES
#include <cmath>

#include <OpenGLPrj.hpp>

#include <GLFW/glfw3.h>

#include <Camera.hpp>
#include <Shader.hpp>
#include <Player.hpp>
#include <AABB_CollisionDetection.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <map>
#include <iomanip>
#include <sstream>
#include <algorithm>

const std::string program_name = ("Endless Runner Game");

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow *window);
void RenderText(Shader &shader, std::string text, float x, float y, float scale, glm::vec3 color);

GLuint loadTexture(const char* path) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    int width, height, nrChannels;
    unsigned char *data = stbi_load(path, &width, &height, &nrChannels, 0);
    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::cout << "Failed to load texture: " << path << std::endl;
    }
    stbi_image_free(data);
    return textureID;
}

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 800;

unsigned int VAO, VBO;

struct Character {
    unsigned int TextureID; // ID handle of the glyph texture
    glm::ivec2   Size;      // Size of glyph
    glm::ivec2   Bearing;   // Offset from baseline to left/top of glyph
    unsigned int Advance;   // Horizontal offset to advance to next glyph
};

std::map<GLchar, Character> Characters;

//lanes
const std::vector<float> lanes = {-0.5f, 0.0f, 0.5f};
//int currentLaneIndex = 1; // 0.0f - middle lane
float laneSwitchSpeed = 7.5f;
float jumpSpeed = 3.0f;
float crouchSpeed = 3.0f;
bool leftKeyPressed = false;
bool rightKeyPressed = false;
bool upKeyPressed = false;
bool downKeyPressed = false;

float groundLevel = -0.1f;

// camera
static Camera camera(glm::vec3(0.0f, 0.5f, 2.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, -5.0f);

//player
static Player player(lanes, 1, laneSwitchSpeed, jumpSpeed, crouchSpeed);

// timing
static float deltaTime = 0.0f; // time between current frame and last frame
static float lastFrame = 0.0f;

int main() {
  // glfw: initialize and configure
  // ------------------------------
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
  glfwWindowHint(
      GLFW_OPENGL_FORWARD_COMPAT,
      GL_TRUE); // uncomment this statement to fix compilation on OS X
#endif

  // glfw window creation
  // --------------------
  GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT,
                                        program_name.c_str(), nullptr, nullptr);

  if (window == nullptr) {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }


  glfwMakeContextCurrent(window);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  //glfwSetCursorPosCallback(window, mouse_callback); //turn this off, only for debugging
  glfwSetInputMode(window, GLFW_REPEAT, GLFW_FALSE);
//  glfwSetScrollCallback(window, scroll_callback);

  // tell GLFW to capture our mouse
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  // glad: load all OpenGL function pointers
  // ---------------------------------------
  if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return -1;
  }


  // configure global opengl state
  // -----------------------------
  glEnable(GL_DEPTH_TEST);

  // build and compile our shader program
  // ------------------------------------
  std::string shader_location("../res/shaders/");
  std::string used_shaders("shader");
  Shader ourShader(shader_location + used_shaders + std::string(".vert"),
                   shader_location + used_shaders + std::string(".frag"));

    Shader textShader(shader_location + std::string("text.vert"),
                      shader_location + std::string("text.frag"));


    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    FT_Library ft;
    // All functions return a value different than 0 whenever an error occurred
    if (FT_Init_FreeType(&ft))
    {
        std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
        return -1;
    }

    FT_Face face;
    if (FT_New_Face(ft, "../res/fonts/arial.ttf", 0, &face)) {
        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
        return -1;
    }
    else {
        // set size to load glyphs as
        FT_Set_Pixel_Sizes(face, 0, 48);

        // disable byte-alignment restriction
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        // load first 128 characters of ASCII set
        for (unsigned char c = 0; c < 128; c++)
        {
            // Load character glyph
            if (FT_Load_Char(face, c, FT_LOAD_RENDER))
            {
                std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
                continue;
            }
            // generate texture
            unsigned int texture;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(
                    GL_TEXTURE_2D,
                    0,
                    GL_RED,
                    face->glyph->bitmap.width,
                    face->glyph->bitmap.rows,
                    0,
                    GL_RED,
                    GL_UNSIGNED_BYTE,
                    face->glyph->bitmap.buffer
            );
            // set texture options
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            // now store character for later use
            Character character = {
                    texture,
                    glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                    glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                    static_cast<unsigned int>(face->glyph->advance.x)
            };
            Characters.insert(std::pair<char, Character>(c, character));
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    FT_Done_Face(face);
    FT_Done_FreeType(ft);
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    float startZ = 3.0f;
    float length = 23.0f;
    int numSegments = 10;

    std::vector<float> pathVertices;
    std::vector<unsigned int> pathIndices;

    for(int i = 0; i < numSegments; i++){
        pathVertices.push_back(-0.7f);
        pathVertices.push_back(groundLevel);
        pathVertices.push_back(startZ - (i + 1) * length / numSegments);
        pathVertices.push_back(0.0f);
        pathVertices.push_back(1.0f);

        pathVertices.push_back(0.7f);
        pathVertices.push_back(groundLevel);
        pathVertices.push_back(startZ - (i + 1) * length / numSegments);
        pathVertices.push_back(1.0f);
        pathVertices.push_back(1.0f);

        pathVertices.push_back(-0.7f);
        pathVertices.push_back(groundLevel);
        pathVertices.push_back(startZ - i * length / numSegments);
        pathVertices.push_back(0.0f);
        pathVertices.push_back(0.0f);

        pathVertices.push_back(0.7f);
        pathVertices.push_back(groundLevel);
        pathVertices.push_back(startZ - i * length / numSegments);
        pathVertices.push_back(1.0f);
        pathVertices.push_back(0.0f);


        pathIndices.push_back(i * 4);//0
        pathIndices.push_back(i * 4 + 1);//1
        pathIndices.push_back(i * 4 + 2);//2
        pathIndices.push_back(i * 4 + 1);//1
        pathIndices.push_back(i * 4 + 2);//2
        pathIndices.push_back(i * 4 + 3);//3

    }

    unsigned int pathVAO, pathVBO, pathEBO;
    glGenVertexArrays(1, &pathVAO);
    glGenBuffers(1, &pathVBO);
    glGenBuffers(1, &pathEBO);

    glBindVertexArray(pathVAO);

    glBindBuffer(GL_ARRAY_BUFFER, pathVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * pathVertices.size(), &pathVertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pathEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * pathIndices.size(), &pathIndices[0], GL_STATIC_DRAW);

// Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

// Texture coordinate attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    std::vector<float> wallVerticess;
    std::vector<unsigned int> wallIndicess;

    for(int i = 0; i < numSegments; i++){
        //left wall
        wallVerticess.push_back(-0.7f);
        wallVerticess.push_back(5.0f);
        wallVerticess.push_back(startZ - (i+1) * length / numSegments);
        wallVerticess.push_back(1.0f);
        wallVerticess.push_back(5.0f);

        wallVerticess.push_back(-0.7f);
        wallVerticess.push_back(groundLevel);
        wallVerticess.push_back(startZ - (i+1) * length / numSegments);
        wallVerticess.push_back(1.0f);
        wallVerticess.push_back(0.0f);

        wallVerticess.push_back(-0.7f);
        wallVerticess.push_back(5.0f);
        wallVerticess.push_back(startZ - i * length / numSegments);
        wallVerticess.push_back(0.0f);
        wallVerticess.push_back(5.0f);

        wallVerticess.push_back(-0.7f);
        wallVerticess.push_back(groundLevel);
        wallVerticess.push_back(startZ - i * length / numSegments);
        wallVerticess.push_back(0.0f);
        wallVerticess.push_back(0.0f);

        //right wall
        wallVerticess.push_back(0.7f);
        wallVerticess.push_back(5.0f);
        wallVerticess.push_back(startZ - (i+1) * length / numSegments);
        wallVerticess.push_back(0.0f);
        wallVerticess.push_back(5.0f);

        wallVerticess.push_back(0.7f);
        wallVerticess.push_back(groundLevel);
        wallVerticess.push_back(startZ - (i+1) * length / numSegments);
        wallVerticess.push_back(0.0f);
        wallVerticess.push_back(0.0f);

        wallVerticess.push_back(0.7f);
        wallVerticess.push_back(5.0f);
        wallVerticess.push_back(startZ - i * length / numSegments);
        wallVerticess.push_back(1.0f);
        wallVerticess.push_back(5.0f);

        wallVerticess.push_back(0.7f);
        wallVerticess.push_back(groundLevel);
        wallVerticess.push_back(startZ - i * length / numSegments);
        wallVerticess.push_back(1.0f);
        wallVerticess.push_back(0.0f);

        //left wall
        wallIndicess.push_back(i*8);//0
        wallIndicess.push_back(i*8+1);//1
        wallIndicess.push_back(i*8+2);//2
        wallIndicess.push_back(i*8+1);//1
        wallIndicess.push_back(i*8+2);//2
        wallIndicess.push_back(i*8+3);//3

        //right wall
        wallIndicess.push_back(i*8+4);//0
        wallIndicess.push_back(i*8+5);//1
        wallIndicess.push_back(i*8+6);//2
        wallIndicess.push_back(i*8+5);//1
        wallIndicess.push_back(i*8+6);//2
        wallIndicess.push_back(i*8+7);//3
    }

    unsigned int wallVAO, wallVBO, wallEBO;
    glGenVertexArrays(1, &wallVAO);
    glGenBuffers(1, &wallVBO);
    glGenBuffers(1, &wallEBO);

    glBindVertexArray(wallVAO);

    glBindBuffer(GL_ARRAY_BUFFER, wallVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * wallVerticess.size(), &wallVerticess[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, wallEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * wallIndicess.size(), &wallIndicess[0], GL_STATIC_DRAW);

// Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

// Texture coordinate attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);


    const int sectorCount = 360; //horizontal slices
    const int stackCount = sectorCount / 2; //vertical slices
    const float radius = 0.1f;

    std::vector<float> playerVertices;
    for (int i = 0; i <= stackCount; ++i) {
        float stackAngle = M_PI / 2 - i * M_PI / stackCount;  //phi
        float xy = radius * cos(stackAngle);
        float z = radius * sin(stackAngle);

        for (int j = 0; j <= sectorCount; ++j) {
            float sectorAngle = j * 2 * M_PI / sectorCount; //theta
            float x = xy * cos(sectorAngle);
            float y = xy * sin(sectorAngle);

            playerVertices.push_back(x);
            playerVertices.push_back(y);
            playerVertices.push_back(z);

            float s = (float)j / sectorCount;
            float t = (float)i / stackCount;
            playerVertices.push_back(s);
            playerVertices.push_back(t);
        }
    }

    unsigned int playerVAO, playerVBO;
    glGenVertexArrays(1, &playerVAO);
    glGenBuffers(1, &playerVBO);

    glBindVertexArray(playerVAO);

    glBindBuffer(GL_ARRAY_BUFFER, playerVBO);
    glBufferData(GL_ARRAY_BUFFER, playerVertices.size() * sizeof(float ), &playerVertices[0], GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

// Texture coordinate attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    //obstacle preparation
    float playerStartPos = player.GetPosition().z;
    float segmentLength = length/numSegments;
    unsigned int pointer = 0;
    float endZ = startZ - length;

    //obstacle random preparation
    std::random_device rd;  // Obtain a random number from hardware
    std::mt19937 gen(rd()); // Seed the generator
    std::uniform_int_distribution<> disX(0, 2);
    std::uniform_int_distribution<> disObstacle(0, 3); //0-trunk 1-down trunk 2-up trunk 3-empty

    int numberOfObstacles = numSegments / 2;
    unsigned int pointerObstacle = 0;

    std::vector<float> zCoordinates;
    std::vector<int> lanesIndexes;
    std::vector<int> obstaclesTypes;

    float start = endZ/2;
    float end = endZ;

    for (int i = 0; i < numberOfObstacles; i++) {
        obstaclesTypes.push_back(disObstacle(gen));
    }

    float stepZ = (end - start) / numberOfObstacles;

    for (int i = 0; i < numberOfObstacles; i++) {
            if (obstaclesTypes[i] != 3) {
                std::uniform_real_distribution<> disZ(start + i * stepZ, start + (i + 1) * stepZ);
                zCoordinates.push_back(disZ(gen));
                if (obstaclesTypes[i] == 0) {
                    lanesIndexes.push_back(disX(gen));
                } else {
                    lanesIndexes.push_back(1);
                }
            } else {
                zCoordinates.push_back(0.0f);
                lanesIndexes.push_back(1);
            }
    }
    std::vector<float> obstacleVertices;

    //tree trunks
    int n = 130;
    float r = 0.1f;
    float xc=0.0f, yc=groundLevel, zc=0.0f;
    float angle = 0.0f;
    float delta_angle = 2*M_PI/n;
    float topyc = 0.2;

    obstacleVertices.push_back(xc);
    obstacleVertices.push_back(topyc);
    obstacleVertices.push_back(zc);

    obstacleVertices.push_back(0.5f); //middle 0.5,0.5 from texture
    obstacleVertices.push_back(0.5f);

    for (int i=0; i<n+1; i++) {
        obstacleVertices.push_back(xc+r*cos(angle));
        obstacleVertices.push_back(topyc);
        obstacleVertices.push_back(zc+r*sin(angle));
        angle+=delta_angle;

        obstacleVertices.push_back(0.5 + 0.5*cos(angle));
        obstacleVertices.push_back(0.5 + 0.5*sin(angle));
    }

    for (int i=0; i<n+1; i++) {
        obstacleVertices.push_back(xc+r*cos(angle));
        obstacleVertices.push_back(topyc);
        obstacleVertices.push_back(zc+r*sin(angle));
        angle+=delta_angle;

        obstacleVertices.push_back(i*1.0/n); //vertices from the top line
        obstacleVertices.push_back(1.0f);

        obstacleVertices.push_back(xc+r*cos(angle));
        obstacleVertices.push_back(yc);
        obstacleVertices.push_back(zc+r*sin(angle));
        angle+=delta_angle;

        obstacleVertices.push_back(i*1.0/n); //vertices from bottom line
        obstacleVertices.push_back(0.0f);
    }

//down fallen trunk
    float r_down = 0.1f;
    float leftXc=-0.6f, rightXc = 0.6f, yc_down=groundLevel+radius;
    angle = 0.0f;

    for (int i=0; i<n+1; i++) {
        obstacleVertices.push_back(leftXc);
        obstacleVertices.push_back(yc_down+r_down*sin(angle));
        obstacleVertices.push_back(zc+r_down*cos(angle));
        angle+=delta_angle;

        obstacleVertices.push_back(i*5.0/n);
        obstacleVertices.push_back(5.0f);

        obstacleVertices.push_back(rightXc);
        obstacleVertices.push_back(yc_down+r_down*sin(angle));
        obstacleVertices.push_back(zc+r_down*cos(angle));
        angle+=delta_angle;

        obstacleVertices.push_back(i*5.0/n);
        obstacleVertices.push_back(0.0f);
    }

//up fallen trunk

    float leftXcUp=-0.7f, rightXcUp = 0.7f, ycUp=0.3f;
    angle = 0.0f;
    delta_angle = 2*M_PI/n;

    for (int i=0; i<n+1; i++) {
        obstacleVertices.push_back(leftXcUp);
        obstacleVertices.push_back(ycUp+r_down*sin(angle));
        obstacleVertices.push_back(zc+r_down*cos(angle));
        angle+=delta_angle;

        obstacleVertices.push_back(i*5.0/n);
        obstacleVertices.push_back(5.0f);

        obstacleVertices.push_back(rightXcUp);
        obstacleVertices.push_back(ycUp+r_down*sin(angle));
        obstacleVertices.push_back(zc+r_down*cos(angle));
        angle+=delta_angle;

        obstacleVertices.push_back(i*5.0/n);
        obstacleVertices.push_back(0.0f);
    }

    unsigned int obstacleVAO, obstacleVBO;
    glGenVertexArrays(1, &obstacleVAO);
    glGenBuffers(1, &obstacleVBO);

    glBindVertexArray(obstacleVAO);

    glBindBuffer(GL_ARRAY_BUFFER, obstacleVBO);
    glBufferData(GL_ARRAY_BUFFER, obstacleVertices.size() * sizeof(float), &obstacleVertices[0], GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

// Texture coordinate attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    float endScreenVertices[] = {
            // positions     // texCoords
            -1.0f,  1.0f, 0.0f,  0.0f, 1.0f,  // Top-left
            -1.0f, -1.0f, 0.0f,  0.0f, 0.0f,  // Bottom-left
            1.0f, -1.0f, 0.0f,  1.0f, 0.0f,  // Bottom-right
            1.0f,  1.0f, 0.0f,  1.0f, 1.0f   // Top-right
    };

    unsigned int endScreenIndices[] = {
            0, 1, 2, // First triangle
            2, 3, 0  // Second triangle
    };

    unsigned int quadVAO, quadVBO, quadEBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glGenBuffers(1, &quadEBO);

    glBindVertexArray(quadVAO);

// Vertex Buffer Object
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(endScreenVertices), endScreenVertices, GL_STATIC_DRAW);

// Element Buffer Object
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(endScreenIndices), endScreenIndices, GL_STATIC_DRAW);

// Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float),static_cast<void *>(nullptr));
    glEnableVertexAttribArray(0);

// Texture coordinate attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void *>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    GLuint diffuseTexturePath = loadTexture("../res/textures/mud_forest_diff_4k_scaled.jpg");

    GLuint diffuseTextureWall = loadTexture("../res/textures/mossy_cobblestone_diff_4k_scaled.jpg");

    GLuint diffuseTextureTrunk = loadTexture("../res/textures/bark_willow_diff_4k.jpg");

    GLuint diffuseTextureCircleTrunk = loadTexture("../res/textures/round_tree.jpg");

    GLuint diffuseTextureFallenTrunk = loadTexture("../res/textures/tree_fallen.jpg");

    GLuint diffuseTextureBall = loadTexture("../res/textures/rock_ball_scaled.jpg");

    ourShader.use();

    glm::vec3 fogColor = glm::vec3(0.5f, 0.5f, 0.5f);
    float fogStart = 4.0f;
    float fogEnd = 13.0f;

    // Set fog uniforms
    ourShader.setVec3("fogColor", fogColor);
    ourShader.setFloat("fogStart", fogStart);
    ourShader.setFloat("fogEnd", fogEnd);

    bool gameOver = false;
    bool showEndScreen = false;
    float collisionTime = 0.0f;;
    const float endScreenDelay = 0.3f; // delay

    float playerRotationAngle = 0.0f;
    float rotationSpeed = 90.0f;

    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        processInput(window);
        player.ProcessInput(leftKeyPressed, rightKeyPressed, upKeyPressed, false, deltaTime);

        // endless running
        camera.ProcessKeyboard(FORWARD, deltaTime);
        player.MoveForward(2.5f, deltaTime);
        //2.5f is the speed of the camera moving forward

        playerRotationAngle += rotationSpeed * deltaTime;
        if (playerRotationAngle >= 360.0f) {
            playerRotationAngle -= 360.0f;
        }

        if (gameOver) {
            // Check if 2 seconds have passed since the collision
            if (currentFrame - collisionTime >= endScreenDelay) {
                showEndScreen = true;
            }
        }

        // collision detection
        if(gameOver) {
            //end screen
            if(showEndScreen) {
                glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                ourShader.use();
                ourShader.setVec3("MyColor", glm::vec3(0.0f, 0.0f, 0.0f));

                ourShader.setBool("useTexture", false);
                ourShader.setBool("endGame", true);

                // pass projection matrix to shader
                glm::mat4 projection = glm::perspective(
                        glm::radians(camera.Zoom), static_cast<float>(SCR_WIDTH) / SCR_HEIGHT,
                        0.2f, 100.0f);
                ourShader.setMat4("projection", projection);

                // camera/view transformation
                glm::mat4 view = camera.GetViewMatrix();
                ourShader.setMat4("view", view);
                ourShader.setVec3("cameraPos", camera.Position);

                glBindVertexArray(quadVAO);
                glm::mat4 modelEnd = glm::mat4(1.0f);
                glm::vec3 cameraFront = camera.Front;
                glm::vec3 quadPosition =
                        camera.Position + cameraFront * 1.0f;

                modelEnd = glm::translate(modelEnd, quadPosition);
                ourShader.setMat4("model", modelEnd);

                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                glEnable(GL_CULL_FACE);
                glEnable(GL_BLEND);
                textShader.use();

                // create transformations
                glm::mat4 projectionText = glm::ortho(0.0f, static_cast<float>(SCR_WIDTH), 0.0f, static_cast<float>(SCR_HEIGHT));
                glUniformMatrix4fv(glGetUniformLocation(textShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projectionText));

                RenderText(textShader, "GAME OVER", 120.0f, 400.0f, 2.0f, glm::vec3(1.0, 0.0f, 0.0f));
            }

            //break;
        } else {
            glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glDisable(GL_CULL_FACE);
            glDisable(GL_BLEND);
            CollisionDetector playerBox = CollisionDetector();
            playerBox.getPlayer(player, groundLevel);
            for (int i = 0; i < numberOfObstacles; i++) {
                if (obstaclesTypes[i] != 3) {
                    glm::vec3 obstacleVector = glm::vec3(lanes[lanesIndexes[i]], groundLevel, zCoordinates[i]);
                    CollisionDetector obstacleBox = CollisionDetector();
                    obstacleBox.getObstacle(obstacleVector, obstaclesTypes[i]);

                    if (playerBox.check(obstacleBox)) {
                        gameOver = true;
                        collisionTime = static_cast<float>(glfwGetTime());
                        break;
                    }
                }
            }

            // activate shader
            ourShader.use();
            ourShader.setVec3("MyColor", glm::vec3(1.0f, 0.0f, 0.0f));

            ourShader.setBool("useTexture", true);
            ourShader.setBool("endGame", false);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, diffuseTexturePath);
            ourShader.setInt("diffuseTexture", 0);

            // pass projection matrix to shader
            glm::mat4 projection = glm::perspective(
                    glm::radians(camera.Zoom), static_cast<float>(SCR_WIDTH) / SCR_HEIGHT,
                    0.2f, 100.0f);
            ourShader.setMat4("projection", projection);

            // camera/view transformation
            glm::mat4 view = camera.GetViewMatrix();
            ourShader.setMat4("view", view);
            ourShader.setVec3("cameraPos", camera.Position);

            glBindVertexArray(pathVAO);
            glm::mat4 modelPath = glm::mat4(1.0f);
            ourShader.setMat4("model", modelPath);
            for (int i = 0; i < numSegments; i++) {
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void *) (i * 6 * sizeof(unsigned int)));
            }

            glBindVertexArray(wallVAO);
            glm::mat4 modelWall = glm::mat4(1.0f);
            ourShader.setMat4("model", modelWall);
            ourShader.setVec3("MyColor", glm::vec3(0.0f, 0.0f, 1.0f));
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, diffuseTextureWall);
            ourShader.setInt("diffuseTexture", 0);
            for (int i = 0; i < numSegments; i++) {
                glDrawElements(GL_TRIANGLES, 12, GL_UNSIGNED_INT, (void *) (i * 12 * sizeof(unsigned int)));
            }

            if (abs(playerStartPos - player.GetPosition().z) >= segmentLength) {
                playerStartPos = player.GetPosition().z;

                //20 from drawing
                pathVertices[pointer * 20 + 2] = endZ - segmentLength;
                pathVertices[pointer * 20 + 5 + 2] = endZ - segmentLength;
                pathVertices[pointer * 20 + 10 + 2] = endZ;
                pathVertices[pointer * 20 + 15 + 2] = endZ;

                //left wall
                wallVerticess[pointer * 40 + 2] = endZ - segmentLength;
                wallVerticess[pointer * 40 + 5 + 2] = endZ - segmentLength;
                wallVerticess[pointer * 40 + 10 + 2] = endZ;
                wallVerticess[pointer * 40 + 15 + 2] = endZ;

                //right wall
                wallVerticess[pointer * 40 + 20 + 2] = endZ - segmentLength;
                wallVerticess[pointer * 40 + 25 + 2] = endZ - segmentLength;
                wallVerticess[pointer * 40 + 30 + 2] = endZ;
                wallVerticess[pointer * 40 + 35 + 2] = endZ;

                endZ -= segmentLength;

                if (pointer == numSegments - 1) {
                    pointer = 0;
                } else {
                    pointer++;
                }

                if (numberOfObstacles < numSegments) {
                    int obstacleType = disObstacle(gen);
                    obstaclesTypes.push_back(obstacleType);
                    if (obstacleType != 3) {
                        std::uniform_real_distribution<> disZ(endZ + segmentLength, endZ);
                        zCoordinates.push_back(disZ(gen));
                        if (obstacleType == 0) {
                            lanesIndexes.push_back(disX(gen));
                        } else {
                            lanesIndexes.push_back(1);
                        }
                    } else {
                        zCoordinates.push_back(0.0f);
                        lanesIndexes.push_back(1);
                    }

                    numberOfObstacles++;
                } else {
                    int obstacleType = disObstacle(gen);
                    obstaclesTypes[pointerObstacle] = obstacleType;

                    int previousObstacle, previousPointer;
                    if (pointerObstacle == 0) {
                        previousObstacle = obstaclesTypes[numberOfObstacles - 1];
                        previousPointer = numberOfObstacles - 1;
                    } else {
                        previousObstacle = obstaclesTypes[pointerObstacle - 1];
                        previousPointer = pointerObstacle - 1;
                    }

                    std::uniform_real_distribution<> disZ(endZ + segmentLength, endZ);
                    float newZ = disZ(gen);
                    float distance = 0.7f;
                    bool tooClose = false;
                    if ((previousObstacle == 1 || previousObstacle == 2) && (obstacleType == 1 || obstacleType == 2) && abs(newZ - zCoordinates[previousPointer]) <= distance) {
                        tooClose = true;
                    }


                    if (obstacleType != 3 && !tooClose) {
                        zCoordinates[pointerObstacle] = newZ;
                        if (obstacleType == 0) {
                            lanesIndexes[pointerObstacle] = disX(gen);
                        } else {
                            lanesIndexes[pointerObstacle] = 1;
                        }
                    } else {
                        zCoordinates[pointerObstacle] = 0.0f;
                        lanesIndexes[pointerObstacle] = 1;
                    }


                    if (pointerObstacle == numberOfObstacles - 1) {
                        pointerObstacle = 0;
                    } else {
                        pointerObstacle++;
                    }
                }

                // Re-upload the updated vertex data to the GPU
                glBindBuffer(GL_ARRAY_BUFFER, pathVBO);
                glBufferData(GL_ARRAY_BUFFER, sizeof(float) * pathVertices.size(), &pathVertices[0], GL_STATIC_DRAW);

                glBindBuffer(GL_ARRAY_BUFFER, wallVBO);
                glBufferData(GL_ARRAY_BUFFER, sizeof(float) * wallVerticess.size(), &wallVerticess[0], GL_STATIC_DRAW);
            }

            ourShader.setVec3("MyColor", glm::vec3(1.0f, 0.8f, 1.0f));


            glBindVertexArray(obstacleVAO);
            for (int i = 0; i < numberOfObstacles; i++) {
                if (obstaclesTypes[i] != 3) {
                    glm::mat4 modelObstacle = glm::mat4(1.0f);
                    if (obstaclesTypes[i] == 0) {
                        //tree trunk
                        modelObstacle = glm::translate(modelObstacle,
                                                       glm::vec3(lanes[lanesIndexes[i]], 0.0f, zCoordinates[i]));
                    } else {
                        //fallen tree trunk
                        modelObstacle = glm::translate(modelObstacle, glm::vec3(0.0f, 0.0f, zCoordinates[i]));
                    }
                    ourShader.setMat4("model", modelObstacle);
                    if (obstaclesTypes[i] == 0) {
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, diffuseTextureCircleTrunk);
                        ourShader.setInt("diffuseTexture", 0);

                        glDrawArrays(GL_TRIANGLE_FAN, 0, n + 2);


                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, diffuseTextureTrunk);
                        ourShader.setInt("diffuseTexture", 0);

                        glDrawArrays(GL_TRIANGLE_STRIP, n + 2, 2 * (n + 1));
                    } else if (obstaclesTypes[i] == 1) {
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, diffuseTextureFallenTrunk);
                        ourShader.setInt("diffuseTexture", 0);

                        glDrawArrays(GL_TRIANGLE_STRIP, n + 2 + 2 * (n + 1), 2 * (n + 1));
                    } else {
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, diffuseTextureFallenTrunk);
                        ourShader.setInt("diffuseTexture", 0);

                        glDrawArrays(GL_TRIANGLE_STRIP, n + 2 + 2 * 2 * (n + 1), 2 * (n + 1));
                    }
                }
            }

            ourShader.setVec3("MyColor", glm::vec3(0.82, 0.71, 0.55));

            glBindVertexArray(playerVAO);
            glm::mat4 modelPlayer = glm::mat4(1.0f);
            modelPlayer = glm::translate(modelPlayer, player.GetPosition());

            modelPlayer = glm::rotate(modelPlayer, glm::radians(playerRotationAngle), glm::vec3(-1.0f, 0.0f, 0.0f));

            ourShader.setMat4("model", modelPlayer);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, diffuseTextureBall);
            ourShader.setInt("diffuseTexture", 0);

            glDrawArrays(GL_LINE_STRIP, 0, sectorCount * stackCount);


            glEnable(GL_CULL_FACE);
            glEnable(GL_BLEND);
            textShader.use();

            // create transformations
            glm::mat4 projectionText = glm::ortho(0.0f, static_cast<float>(SCR_WIDTH), 0.0f, static_cast<float>(SCR_HEIGHT));
            glUniformMatrix4fv(glGetUniformLocation(textShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projectionText));

            std::ostringstream timeStream;
            timeStream << std::setfill('0') << std::setw(5) << static_cast<int>(abs(player.GetPosition().z));
            std::string timeText = timeStream.str();
            RenderText(textShader, timeText, 610.0f, 710.0f, 0.9f, glm::vec3(1.0f, 1.0f, 1.0f));
        }
            glfwSwapBuffers(window);
            glfwPollEvents();

    }

// optional: de-allocate all resources once they've outlived their purpose:
// ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &playerVAO);
    glDeleteBuffers(1, &playerVBO);
    glDeleteVertexArrays(1, &pathVAO);
    glDeleteBuffers(1, &pathVBO);
    glDeleteBuffers(1, &pathEBO);
    glDeleteVertexArrays(1, &wallVAO);
    glDeleteBuffers(1, &wallVBO);
    glDeleteBuffers(1, &wallEBO);

// glfw: terminate, clearing all previously allocated GLFW resources.
// ------------------------------------------------------------------
    glfwTerminate();

    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this
// frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    leftKeyPressed = glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS;
    rightKeyPressed = glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS;

    upKeyPressed = glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS;
    downKeyPressed = glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS;
}


// glfw: whenever the window size changed (by OS or user resize) this callback
// function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  // make sure the viewport matches the new window dimensions; note that width
  // and height will be significantly larger than specified on retina displays.
  glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------

void RenderText(Shader &shader, std::string text, float x, float y, float scale, glm::vec3 color) {
    // activate corresponding render state
    shader.use();
    glUniform3f(glGetUniformLocation(shader.ID, "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);

    // iterate through all characters
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++) {
        Character ch = Characters[*c];

        float xpos = x + ch.Bearing.x * scale;
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;
        // update VBO for each character
        float vertices[6][4] = {
                {xpos,     ypos + h, 0.0f, 0.0f},
                {xpos,     ypos,     0.0f, 1.0f},
                {xpos + w, ypos,     1.0f, 1.0f},

                {xpos,     ypos + h, 0.0f, 0.0f},
                {xpos + w, ypos,     1.0f, 1.0f},
                {xpos + w, ypos + h, 1.0f, 0.0f}
        };
        // render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        // update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices),
                        vertices); // be sure to use glBufferSubData and not glBufferData

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        x += (ch.Advance >> 6) *
             scale; // bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}