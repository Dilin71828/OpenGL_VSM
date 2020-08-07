#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>

#include <iostream>
#include <vector>
#include <fstream>

#include "myOpenGL/camera.h"
#include "myOpenGL/shader.h"

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xPos, double yPos);
void scroll_callback(GLFWwindow *window, double xOffset, double yOffset);
void processInput(GLFWwindow *window);
void renderScene(Shader& shader);
void renderQuad();

// basic window setting
const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;

// global time
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// camera control
Camera mainCamera(glm::vec3(0.0f, 2.0f, 8.0f));
float nearPlane = 0.1f;
float farPlane = 50.0f;
float lastX = (float)SCREEN_WIDTH / 2.0f;
float lastY = (float)SCREEN_HEIGHT / 2.0f;
bool firstMouse = true;

// render setting
const int DEPTH_MAP_WIDTH = 1024;
const int DEPTH_MAP_HEIGHT = 1024;

// light setting
glm::vec3 lightPosition = glm::vec3(8.0f, 4.0f, 5.0f);
glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
float lightNearPlane = 0.1f;
float lightFarPlane = 20.0f;

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow *window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "OpenGL_VSM", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    Shader depthShader("depthShader.vert", "depthShader.frag");
    Shader averageShader("screenQuad.vert", "varianceCalculate.frag");
    Shader mainShader("mainShader.vert", "mainShader.frag");
    Shader debugShader("screenQuad.vert", "debugShader.frag");

    // frame buffer for the first pass, view from the light and get the depth and squared depth
    unsigned int depthFBO;
    unsigned int depthRBO;
    glGenFramebuffers(1, &depthFBO);
    glGenRenderbuffers(1, &depthRBO);
    glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, DEPTH_MAP_WIDTH, DEPTH_MAP_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRBO);
    glEnable(GL_DEPTH_TEST);

    unsigned int depthTexture;
    glGenTextures(1, &depthTexture);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, DEPTH_MAP_WIDTH, DEPTH_MAP_HEIGHT, 0, GL_RG, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, depthTexture, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    unsigned int varianceFBO[2];
    unsigned int varianceTexture[2];
    glGenFramebuffers(2, varianceFBO);
    glGenTextures(2, varianceTexture);

    GLfloat borderColor[] = {1.0, 1.0, 1.0, 1.0};
    for (int i = 0; i < 2; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, varianceFBO[i]);
        glBindTexture(GL_TEXTURE_2D, varianceTexture[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, DEPTH_MAP_WIDTH, DEPTH_MAP_HEIGHT, 0, GL_RG, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, varianceTexture[i], 0);
    }

    glm::mat4 lightView = glm::lookAt(lightPosition, glm::vec3(6.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 lightProjection = glm::perspective(glm::radians(90.0f), (float)DEPTH_MAP_WIDTH / (float)DEPTH_MAP_HEIGHT, lightNearPlane, lightFarPlane);

    // static parameter of shader
    depthShader.use();
    depthShader.setFloat("nearPlane", lightNearPlane);
    depthShader.setFloat("farPlane", lightFarPlane);
    mainShader.setInt("varianceShadowMap", 0);

    mainShader.use();
    mainShader.setFloat("nearPlane", lightNearPlane);
    mainShader.setFloat("farPlane", lightFarPlane);
    mainShader.setInt("varianceShadowMap", 0);
    mainShader.setMat4("worldToLight", lightProjection*lightView);
    mainShader.setVec3("mainLight.position", lightPosition);
    mainShader.setVec3("mainLight.intensity", glm::vec3(3,3,3));
    mainShader.setFloat("mainLight.constant", 1.0);
    mainShader.setFloat("mainLight.linear", 0.05);
    mainShader.setFloat("mainLight.quadratic", 0.002);
    mainShader.setVec3("material.albedo", glm::vec3(0.6, 0.6, 0.6));

    debugShader.use();
    debugShader.setInt("debugTexture", 0);

    averageShader.use();
    averageShader.setInt("depthTexture", 0);

    while (!glfwWindowShouldClose(window))
    {
        // calculate the passed time from last frame
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // get camera parameters
        glm::mat4 view = mainCamera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(mainCamera.Zoom), (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, nearPlane, farPlane);

        // shadow pass
        glViewport(0, 0, DEPTH_MAP_WIDTH, DEPTH_MAP_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        depthShader.use();
        depthShader.setMat4("view", lightView);
        depthShader.setMat4("projection", lightProjection);
        renderScene(depthShader);

        // calculate the average value
        glBindFramebuffer(GL_FRAMEBUFFER, varianceFBO[0]);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, depthTexture);
        averageShader.use();
        averageShader.setBool("horizontal", true);
        renderQuad();
        glBindFramebuffer(GL_FRAMEBUFFER, varianceFBO[1]);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, varianceTexture[0]);
        averageShader.setBool("horizontal", false);
        renderQuad();

        // render from camera view
        glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, varianceTexture[1]);
        mainShader.use();
        mainShader.setMat4("view", view);
        mainShader.setMat4("projection", projection);
        mainShader.setVec3("cameraPosition", mainCamera.Position);
        renderScene(mainShader);

        // debug
        /*glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        debugShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, varianceTexture[1]);
        renderQuad();*/

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();

    return 0;
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}
void mouse_callback(GLFWwindow *window, double xPos, double yPos)
{
    if (firstMouse)
    {
        lastX = xPos;
        lastY = yPos;
        firstMouse = false;
    }

    float xoffset = xPos - lastX;
    float yoffset = lastY - yPos;

    lastX = xPos;
    lastY = yPos;

    mainCamera.ProcessMouseMovement(xoffset, yoffset);
}
void scroll_callback(GLFWwindow *window, double xOffset, double yOffset)
{
    mainCamera.ProcessMouseScroll(yOffset);
}
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    {
        mainCamera.ProcessKeyboard(FORWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    {
        mainCamera.ProcessKeyboard(BACKWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    {
        mainCamera.ProcessKeyboard(LEFT, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    {
        mainCamera.ProcessKeyboard(RIGHT, deltaTime);
    }
}

unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
            // position        // uv
            -1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
            1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
            1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

unsigned int frameVAO = 0;
unsigned int frameVBO;
unsigned int planeVAO = 0;
unsigned int planeVBO;
void renderScene(Shader& shader)
{
    if (frameVAO == 0)
    {
        float frameVertices[] = {
            // position         // uv       // normal
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f,
            0.0f, 0.0f, 0.25f, 0.0f, 1.0f, 0.0f, -1.0f, 0.0f,
            0.25f, 0.0f, 0.25f, 1.0f, 1.0f, 0.0f, -1.0f, 0.0f,
            0.25f, 0.0f, 0.25f, 1.0f, 1.0f, 0.0f, -1.0f, 0.0f,
            0.25f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, -1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f,

            0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
            0.0f, 2.0f, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f,
            0.0f, 2.0f, 0.25f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f,
            0.0f, 2.0f, 0.25f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.25f, 1.0f, 0.0f, -1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,

            0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f,
            0.25f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, -1.0f,
            0.25f, 2.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, -1.0f,
            0.25f, 2.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, -1.0f,
            0.0f, 2.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, -1.0f,
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f,

            0.0f, 0.0f, 0.25f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
            0.25f, 0.0f, 0.25f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
            0.25f, 2.0f, 0.25f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
            0.25f, 2.0f, 0.25f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
            0.0f, 2.0f, 0.25f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
            0.0f, 0.0f, 0.25f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,

            0.25f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
            0.25f, 2.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f,
            0.25f, 2.0f, 0.25f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f,
            0.25f, 2.0f, 0.25f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f,
            0.25f, 0.0f, 0.25f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
            0.25f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,

            0.0f, 2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 2.0f, 0.25f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
            0.25f, 2.0f, 0.25f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f,
            0.25f, 2.0f, 0.25f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f,
            0.25f, 2.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        };
        float planeVertices[] = {
            -100.0f, 0.0f, -100.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            100.0f, 0.0f, 100.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f,
            -100.0f, 0.0f, 100.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
            -100.0f, 0.0f, -100.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            100.0f, 0.0f, -100.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            100.0f, 0.0f, 100.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f,
            
        };
        glGenVertexArrays(1, &frameVAO);
        glGenBuffers(1, &frameVBO);
        glBindVertexArray(frameVAO);
        glBindBuffer(GL_ARRAY_BUFFER, frameVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(frameVertices), &frameVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(3*sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(5*sizeof(float)));

        glGenVertexArrays(1, &planeVAO);
        glGenBuffers(1, &planeVBO);
        glBindVertexArray(planeVAO);
        glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), &planeVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(3*sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(5*sizeof(float)));
    }
    glBindVertexArray(frameVAO);
    glm::mat4 model = glm::mat4(1.0);
    for (int i=0; i<5; i++){
        shader.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        model = glm::translate(model, glm::vec3(2.0, 0.0, 0.0));
    }
    glBindVertexArray(planeVAO);
    model = glm::mat4(1.0);
    model = glm::translate(model, glm::vec3(0.0, 0.001, 0.0));
    shader.setMat4("model", model);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}