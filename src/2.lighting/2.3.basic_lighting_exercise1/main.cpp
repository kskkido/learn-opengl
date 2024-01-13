#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <filesystem>
#include <cmath>
#include <optional>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

struct State
{
  glm::vec3 cameraPosition;
  glm::vec3 cameraFront;
  glm::vec3 cameraUp;
  float cameraSpeed;
  float cameraSensitivity;
  glm::vec3 lightPosition;
  glm::vec3 lightColor;
  glm::vec3 cubeColor;
  float deltaTime;
  float lastFrame;
  float fov;
  float yaw;
  float pitch;
  std::optional<float> lastX;
  std::optional<float> lastY;
};

std::string readFile(std::filesystem::path& path)
{
  std::ifstream handle;
  std::string content;
  handle.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  try
  {
    handle.open(path);
    std::stringstream stream;
    stream << handle.rdbuf();
    handle.close();
    content = stream.str();
  }
  catch (std::ifstream::failure& error)
  {
      std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << error.what() << std::endl;
  }
  return content;
}

unsigned char* readImage(std::filesystem::path& path, int& width, int& height)
{
  int nrChannels;
  return stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
}

unsigned int createShader(GLenum shaderType, const char* shaderSource)
{
  unsigned int shader = glCreateShader(shaderType);
  glShaderSource(shader, 1, &shaderSource, NULL);
  glCompileShader(shader);
  int success;
  char infoLog[512];
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if(!success)
  {
    glGetShaderInfoLog(shader, 512, NULL, infoLog);
    std::cout << "ERROR::SHADER::" << shaderType << "COMPILATION_FAILED\n" << infoLog << std::endl;
  }
  return shader;
}

unsigned int createShaderProgram(std::vector<unsigned int>& shaders)
{
  unsigned int shaderProgram = glCreateProgram();
  for (unsigned int& shader: shaders)
  {
    glAttachShader(shaderProgram, shader);
  }
  glLinkProgram(shaderProgram);
  int success;
  char infoLog[512];
  glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
  if(!success) {
    glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
    std::cout << "ERROR::SHADER_PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
  }
  return shaderProgram;
}

void handleInput(GLFWwindow* window, State* state)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
      glfwSetWindowShouldClose(window, true);
    float cameraTravel = static_cast<float>(state->cameraSpeed * state->deltaTime);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
      state->cameraPosition += cameraTravel * state->cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
      state->cameraPosition -= cameraTravel * state->cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
      state->cameraPosition -= glm::normalize(glm::cross(state->cameraFront, state->cameraUp)) * cameraTravel;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
      state->cameraPosition += glm::normalize(glm::cross(state->cameraFront, state->cameraUp)) * cameraTravel;
}

void handleFrameBufferUpdate(GLFWwindow* window, int width, int height)
{
  glViewport(0, 0, width, height);
}

void handleMouseUpdate(GLFWwindow* window, double xposIn, double yposIn)
{
  State* state = static_cast<State*>(glfwGetWindowUserPointer(window));
  float xpos = static_cast<float>(xposIn);
  float ypos = static_cast<float>(yposIn);
  float xoffset;
  float yoffset;
  
  try {
    float lastX = state->lastX.value();
    xoffset = xpos - lastX;
  } catch(const std::bad_optional_access& e) {
    xoffset = 0;
  }
  try {
    float lastY = state->lastY.value();
    yoffset = lastY - ypos;
  } catch(const std::bad_optional_access& e) {
    yoffset = 0;
  }

  state->lastX = xpos;
  state->lastY = ypos;
  xoffset *= state->cameraSensitivity;
  yoffset *= state->cameraSensitivity;
  state->yaw += xoffset;
  state->pitch += yoffset;
  state->pitch = std::clamp(state->pitch, -89.0f, 89.0f);

  glm::vec3 front;
  front.x = cos(glm::radians(state->yaw)) * cos(glm::radians(state->pitch));
  front.y = sin(glm::radians(state->pitch));
  front.z = sin(glm::radians(state->yaw)) * cos(glm::radians(state->pitch));
  state->cameraFront = glm::normalize(front);
}

void handleScrollUpdate(GLFWwindow* window, double xoffset, double yoffset)
{
  State* state = static_cast<State*>(glfwGetWindowUserPointer(window));
  state->fov -= (float)yoffset;
  state->fov = std::clamp(state->fov, 1.0f, 45.0f);
}

int main()
{
  const GLuint width = 800, height = 600;
  const std::string staticFilePath = {STATIC_FILE_PATH};
  std::filesystem::path cubeVertexShaderFilePath = staticFilePath;
  cubeVertexShaderFilePath /= "cube.vert";
  std::filesystem::path cubeFragmentShaderFilePath = staticFilePath;
  cubeFragmentShaderFilePath /= "cube.frag";
  std::filesystem::path lightSourceVertexShaderFilePath = staticFilePath;
  lightSourceVertexShaderFilePath /= "light-source.vert";
  std::filesystem::path lightSourceFragmentShaderFilePath = staticFilePath;
  lightSourceFragmentShaderFilePath /= "light-source.frag";
  std::filesystem::path containerTextureFilePath = staticFilePath;
  containerTextureFilePath /= "container.jpg";
  std::filesystem::path awesomeFaceTextureFilePath = staticFilePath;
  awesomeFaceTextureFilePath /= "awesomeface.png";
  State state = {
    .cameraPosition = glm::vec3(0.0f, 0.0f, 3.0f),
    .cameraFront = glm::vec3(0.0f, 0.0f, -1.0f),
    .cameraUp = glm::vec3(0.0f, 1.0f,  0.0f),
    .cameraSpeed = 2.5,
    .cameraSensitivity = 0.1f,
    .lightPosition = glm::vec3(1.2f, 1.0f, 2.0f),
    .lightColor = glm::vec3(1.0f, 1.0f, 1.0f),
    .cubeColor = glm::vec3(1.0f, 0.5f, 0.31f),
    .fov = 45.0f,
    .yaw = -90.0f,
    .pitch = 0.0f,
    .deltaTime = 0.0f,
    .lastFrame = 0.0f,
  };
  float vertices[] = {
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
     0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,

    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
     0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
  };
  std::vector<glm::vec3> cubePositions = {
    glm::vec3( 0.0f,  0.0f,  0.0f),
    glm::vec3( 2.0f,  5.0f, -15.0f),
    glm::vec3(-1.5f, -2.2f, -2.5f),
    glm::vec3(-3.8f, -2.0f, -12.3f),
    glm::vec3( 2.4f, -0.4f, -3.5f),
    glm::vec3(-1.7f,  3.0f, -7.5f),
    glm::vec3( 1.3f, -2.0f, -2.5f),
    glm::vec3( 1.5f,  2.0f, -2.5f),
    glm::vec3( 1.5f,  0.2f, -1.5f),
    glm::vec3(-1.3f,  1.0f, -1.5f)
  };
  ImVec4 clearColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

  std::cout << "Starting GLFW context" << std::endl;
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
  GLFWwindow* window = glfwCreateWindow(width, height, "LearnOpenGL", NULL, NULL);
  glfwMakeContextCurrent(window);
  if (window == NULL)
  {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return EXIT_FAILURE;
  }
  int version = gladLoadGL(glfwGetProcAddress);
  if (version == -1)
  {
    std::cout << "Failed to initialize OpenGL context" << std::endl;
    return EXIT_FAILURE;
  }
  std::cout << "Loaded OpenGL " << GLAD_VERSION_MAJOR(version) << "." << GLAD_VERSION_MINOR(version) << std::endl;
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO(); (void)io;
  ImGui::StyleColorsDark();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330");
  std::string cubeVertexShaderSource = readFile(cubeVertexShaderFilePath);
  std::string cubeFragmentShaderSource = readFile(cubeFragmentShaderFilePath);
  unsigned int cubeVertexShader = { createShader(GL_VERTEX_SHADER, cubeVertexShaderSource.c_str()) }; // generated and assign unique shader ID
  unsigned int cubeFragmentShader = { createShader(GL_FRAGMENT_SHADER, cubeFragmentShaderSource.c_str()) }; // generated and assign unique shader ID
  std::vector<unsigned int> cubeShaders = {cubeVertexShader, cubeFragmentShader};
  unsigned int cubeShaderProgram = { createShaderProgram(cubeShaders) };                                                                                           
  std::string lightSourceVertexShaderSource = readFile(lightSourceVertexShaderFilePath);
  std::string lightSourceFragmentShaderSource = readFile(lightSourceFragmentShaderFilePath);
  unsigned int lightSourceVertexShader = { createShader(GL_VERTEX_SHADER, lightSourceVertexShaderSource.c_str()) }; // generated and assign unique shader ID
  unsigned int lightSourceFragmentShader = { createShader(GL_FRAGMENT_SHADER, lightSourceFragmentShaderSource.c_str()) }; // generated and assign unique shader ID
  std::vector<unsigned int> lightSourceShaders = {lightSourceVertexShader, lightSourceFragmentShader};
  unsigned int lightSourceShaderProgram = { createShaderProgram(lightSourceShaders) };                                                                                           
  unsigned int vbo;
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  unsigned int cubeVao;
  glGenVertexArrays(1, &cubeVao);
  glBindVertexArray(cubeVao);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);
  unsigned int lightSourceVao;
  glGenVertexArrays(1, &lightSourceVao);
  glBindVertexArray(lightSourceVao);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);
  glUseProgram(cubeShaderProgram);
  glUniform3fv(glGetUniformLocation(cubeShaderProgram, "lightColor"), 1, glm::value_ptr(state.lightColor));
  glUniform3fv(glGetUniformLocation(cubeShaderProgram, "objectColor"), 1, glm::value_ptr(state.cubeColor));
  glEnable(GL_DEPTH_TEST);
  glViewport(0, 0, width, height);
  glfwSetWindowUserPointer(window, &state);
  glfwSetFramebufferSizeCallback(window, handleFrameBufferUpdate);
  glfwSetCursorPosCallback(window, handleMouseUpdate);
  glfwSetScrollCallback(window, handleScrollUpdate);
  while (!glfwWindowShouldClose(window))
  {
    float time = (float)glfwGetTime();
    state.deltaTime = time - state.lastFrame;
    state.lastFrame = time;
    handleInput(window, &state);
    float radius = 10.0f;
    glm::mat4 view = glm::lookAt(state.cameraPosition, state.cameraPosition + state.cameraFront, state.cameraUp);
    glm::mat4 projection;
    projection = glm::perspective(glm::radians(state.fov), (float)width / (float)height, 0.1f, 100.f);
    glfwPollEvents();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::Begin("Adjust clear color");
    ImGui::ColorEdit3("clear color", (float*)&clearColor); // Edit 3 floats representing a color
    ImGui::End();
    ImGui::Render();
    glClearColor(clearColor.x * clearColor.w, clearColor.y * clearColor.w, clearColor.z * clearColor.w, clearColor.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(cubeShaderProgram);
    glBindVertexArray(cubeVao);
    state.lightPosition = glm::vec3(sin(time) * 5.0f, 1.0f, cos(time) * 5.0f);
    glUniform3fv(glGetUniformLocation(cubeShaderProgram, "lightPosition"), 1, glm::value_ptr(state.lightPosition));
    glUniform3fv(glGetUniformLocation(cubeShaderProgram, "viewPosition"), 1, glm::value_ptr(state.cameraPosition));
    glUniformMatrix4fv(glGetUniformLocation(cubeShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(cubeShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    for (int i = 0; i < cubePositions.size(); ++i)
    {
      glm::vec3 cubePosition = cubePositions[i];
      glm::mat4 model = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first
      model = glm::translate(model, cubePosition);
      model = glm::rotate(model, (float)i * glm::radians(50.0f), glm::vec3(0.5f, 0.5f, 0.0f));
      glUniformMatrix4fv(glGetUniformLocation(cubeShaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
      glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    glUseProgram(lightSourceShaderProgram);
    glBindVertexArray(cubeVao);
    glUniformMatrix4fv(glGetUniformLocation(lightSourceShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(lightSourceShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glm::mat4 model = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first
    model = glm::translate(model, state.lightPosition);
    model = glm::scale(model, glm::vec3(0.2f));
    glUniformMatrix4fv(glGetUniformLocation(lightSourceShaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glDrawArrays(GL_TRIANGLES, 0, 36);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
  }
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  for (unsigned int shader : cubeShaders)
  {
    glDeleteShader(shader);
  }
  glDeleteVertexArrays(1, &cubeVao);
  glDeleteBuffers(1, &vbo);
  glfwTerminate();
  return EXIT_SUCCESS;
}
