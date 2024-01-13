#include <tuple>
#include <optional>
#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/gl.h>
#include <GLFW/glfw3.h>

struct State
{
  glm::vec3 cameraPosition;
  glm::vec3 cameraFront;
  glm::vec3 cameraUp;
  float cameraSpeed;
  float cameraSensitivity;
  float time;
  float deltaTime;
  float lastFrame;
  float fov;
  float yaw;
  float pitch;
  std::optional<float> lastX;
  std::optional<float> lastY;
  int bufferWidth;
  int bufferHeight;
};

struct QuadVertex
{
  glm::vec3 position;
  glm::vec3 color;
};

struct QuadInstanceVertex
{
  glm::mat4 model;
};

std::stringstream readFile(std::filesystem::path path)
{
  std::ifstream file;
  file.open(path);
  std::stringstream stream;
  stream << file.rdbuf();
  return stream;
}

unsigned int createShader(const char* source, GLenum type)
{
  unsigned int shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, NULL);
  glCompileShader(shader);
  int success;
  char infoLog[512];
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if(!success)
  {
    glGetShaderInfoLog(shader, 512, NULL, infoLog);
    std::cout << "ERROR::SHADER::" << type << "COMPILATION_FAILED\n" << infoLog << std::endl;
  }
  return shader;
}

unsigned int loadShader(std::filesystem::path path, GLenum type)
{
  std::string source = readFile(path).str();
  return createShader(source.c_str(), type);
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

void updateState(GLFWwindow* window, State& state)
{
  state.time = glfwGetTime();
  state.deltaTime = state.time - state.lastFrame;
  state.lastFrame = state.time;
  glfwGetFramebufferSize(window, &state.bufferWidth, &state.bufferHeight);
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
  {
    glfwSetWindowShouldClose(window, GL_TRUE);
  }
  float cameraTravel = state.deltaTime * state.cameraSpeed;
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
  {
    state.cameraPosition += cameraTravel * glm::normalize(state.cameraFront);
  }
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
  {
    state.cameraPosition -= cameraTravel * glm::normalize(state.cameraFront);
  }
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
  {
    state.cameraPosition -= glm::normalize(glm::cross(state.cameraFront, state.cameraUp)) * cameraTravel;
  }
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
  {
    state.cameraPosition += glm::normalize(glm::cross(state.cameraFront, state.cameraUp)) * cameraTravel;
  }
}

void handleMouseUpdate(GLFWwindow* window, double xposIn, double yposIn)
{
  State* state = static_cast<State*>(glfwGetWindowUserPointer(window));
  float xPosition = (float)xposIn;
  float yPosition = (float)yposIn;
  float xOffset;
  float yOffset;

  try
  {
    float lastX = state->lastX.value();
    xOffset = (xPosition - lastX) * state->cameraSensitivity;
  }
  catch(const std::bad_optional_access& e)
  {
    xOffset = 0;
  }

  try
  {
    float lastY = state->lastY.value();
    yOffset = (lastY - yPosition) * state->cameraSensitivity;
  }
  catch(const std::bad_optional_access& e)
  {
    yOffset = 0;
  }

  state->lastX = xPosition;
  state->lastY = yPosition;
  state->yaw += xOffset;
  state->pitch += yOffset;
  state->pitch = std::clamp(state->pitch, -89.0f, 89.0f);

  glm::vec3 direction;
  direction.x = cos(glm::radians(state->yaw)) * cos(glm::radians(state->pitch)); // scale x by xz displacement of pitch
  direction.y = sin(glm::radians(state->pitch));
  direction.z = sin(glm::radians(state->yaw)) * cos(glm::radians(state->pitch)); // scale z by xz displacement of pitch
  state->cameraFront = glm::normalize(direction);
}

void handleScrollUpdate(GLFWwindow* window, double xoffset, double yoffset)
{
  State* state = (State*)glfwGetWindowUserPointer(window);
  state->fov -= (float)yoffset;
  state->fov = std::clamp(state->fov, 1.0f, 45.0f);
}

void handleFrameBufferUpdate(GLFWwindow* window, int width, int height)
{
  glViewport(0, 0, width, height);
}

int main()
{
  std::tuple<int, int> glVersion = {3,3};
  int windowWidth = 800, windowHeight = 600;
  std::string windowTitle = {WINDOW_TITLE};
  std::filesystem::path staticFilePath = {STATIC_FILE_PATH};
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, std::get<0>(glVersion));
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, std::get<1>(glVersion));
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, windowTitle.c_str(), NULL, NULL);
  if (window == NULL)
  {
    glfwTerminate();
    std::cout << "Failed to create window" << std::endl;
    return EXIT_FAILURE;
  }
  glfwMakeContextCurrent(window);
  int version = gladLoadGL(glfwGetProcAddress);
  if (version == -1)
  {
    glfwTerminate();
    std::cout << "Failed to link OpenGL" << std::endl;
    return EXIT_FAILURE;
  }
  std::vector<unsigned int> quadShaders = {
    loadShader(staticFilePath / "quad.vert", GL_VERTEX_SHADER),
    loadShader(staticFilePath / "quad.frag", GL_FRAGMENT_SHADER),
  };
  unsigned int quadShaderProgram = createShaderProgram(quadShaders);
  std::vector<QuadVertex> quadVertices = {
    QuadVertex { .position = glm::vec3(-0.05f, 0.05f, 0.0f), .color = glm::vec3(1.0f, 0.0f, 0.0f) },
    QuadVertex { .position = glm::vec3(0.05f, -0.05f, 0.0f), .color = glm::vec3(0.0f, 1.0f, 0.0f) },
    QuadVertex { .position = glm::vec3(-0.05f, -0.05f, 0.0f), .color = glm::vec3(0.0f, 0.0f, 1.0f) },
    QuadVertex { .position = glm::vec3(-0.05f, 0.05f, 0.0f), .color = glm::vec3(1.0f, 0.0f, 0.0f) },
    QuadVertex { .position = glm::vec3(0.05f, -0.05f, 0.0f), .color = glm::vec3(0.0f, 1.0f, 0.0f) },
    QuadVertex { .position = glm::vec3(0.05f, 0.05f, 0.0f), .color = glm::vec3(0.0f, 1.0f, 1.0f) },
  };
  std::vector<QuadInstanceVertex> quadInstanceVertices = {};
  for (int x = -10; x < 10; x += 2)
  {
    for (int y = -10; y < 10; y += 2)
    {
      float offset = 0.1f;
      glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3((float)x / 10.0f + offset, (float)y / 10.0f + offset, 0.0f));
      quadInstanceVertices.push_back(QuadInstanceVertex { .model = model });
    }
  }
  unsigned int quadVao;
  glGenVertexArrays(1, &quadVao);
  glBindVertexArray(quadVao);
  unsigned int quadVbo;
  glGenBuffers(1, &quadVbo);
  glBindBuffer(GL_ARRAY_BUFFER, quadVbo);
  glBufferData(GL_ARRAY_BUFFER, quadVertices.size() * sizeof(QuadVertex), &quadVertices[0], GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), (void*)(offsetof(QuadVertex, position)));
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), (void*)(offsetof(QuadVertex, color)));
  glEnableVertexAttribArray(1);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  unsigned int quadInstanceVbo;
  glGenBuffers(1, &quadInstanceVbo);
  glBindBuffer(GL_ARRAY_BUFFER, quadInstanceVbo);
  glBufferData(GL_ARRAY_BUFFER, quadInstanceVertices.size() * sizeof(QuadInstanceVertex), &quadInstanceVertices[0], GL_STATIC_DRAW);
  unsigned int offset = 2;
  for (unsigned int i = 0; i < 4; i++)
  {
    unsigned int index = i + offset;
    glVertexAttribPointer(index, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(i * sizeof(glm::vec4)));
    glEnableVertexAttribArray(index);
    glVertexAttribDivisor(index, 1);
  }
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
  State state = {
    .cameraPosition = glm::vec3(0.0f, 0.0f, 3.0f),
    .cameraFront = glm::vec3(0.0f, 0.0f, -1.0f),
    .cameraUp = glm::vec3(0.0f, 1.0f, 0.0f),
    .cameraSpeed = 2.5f,
    .cameraSensitivity = 0.1f,
    .fov = 45.0f,
    .yaw = -90.0f,
    .pitch = 0.0f,
    .time = 0.0f,
    .deltaTime = 0.0f,
    .lastFrame = 0.0f,
    .bufferWidth = 0,
    .bufferHeight = 0,
  };
  glfwGetFramebufferSize(window, &state.bufferWidth, &state.bufferHeight);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_CAPTURED);
  glfwSetWindowUserPointer(window, &state);
  glfwSetFramebufferSizeCallback(window, handleFrameBufferUpdate);
  glfwSetCursorPosCallback(window, handleMouseUpdate);
  glfwSetScrollCallback(window, handleScrollUpdate);
  while (!glfwWindowShouldClose(window))
  {
    updateState(window, state);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glBindVertexArray(quadVao);
    glUseProgram(quadShaderProgram);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)state.bufferWidth / (float)state.bufferHeight, 0.1f, 100.0f);
    glm::mat4 view = glm::lookAt(state.cameraPosition, state.cameraPosition + state.cameraFront, state.cameraUp);
    glUniformMatrix4fv(glGetUniformLocation(quadShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(quadShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glDrawArraysInstanced(GL_TRIANGLES, 0, quadVertices.size(), quadInstanceVertices.size());
    glfwSwapBuffers(window);
    glfwPollEvents();
  }
  return EXIT_SUCCESS;
}
