#include <optional>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
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
  float fov;
  float yaw;
  float pitch;
  std::optional<float> lastX;
  std::optional<float> lastY;
  float time;
  float deltaTime;
  float lastFrame;
  int bufferWidth;
  int bufferHeight;
};

struct CubeVertex
{
  glm::vec3 position;
};

struct Cube
{
  glm::vec3 position;
  unsigned int shaderProgram;
};

struct Matrices
{
  glm::mat4 projection;
  glm::mat4 view;
};

std::stringstream readFile(std::filesystem::path path)
{
  std::ifstream file;
  std::stringstream stream;
  file.open(path);
  stream << file.rdbuf();
  file.close();
  return stream;
}

unsigned int createShader(const char* source, GLenum type)
{
  unsigned int shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, NULL);
  glCompileShader(shader);
  int success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success)
  {
    char infoLog[512];
    glGetShaderInfoLog(shader, 512, NULL, infoLog);
    std::cout << "ERROR::SHADER::" << type << "COMPILATION_FAILED\n" << infoLog << std::endl;
  }
  return shader;
}

unsigned int readShader(std::filesystem::path path, GLenum type)
{
  std::string source = readFile(path).str();
  return createShader(source.c_str(), type);
}

unsigned int createShaderProgram(std::vector<unsigned int>& shaders)
{
  unsigned int shaderProgram = glCreateProgram();
  for (unsigned int shader : shaders)
  {
    glAttachShader(shaderProgram, shader);
  }
  glLinkProgram(shaderProgram);
  int success;
  glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
  if (!success)
  {
    char infoLog[512];
    glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
    std::cout << "ERROR::SHADER_PROGRAM::" << "LINK_FAILED\n" << infoLog << std::endl;
  }
  return shaderProgram;
}

void handleBufferSizeChange(GLFWwindow* window, int width, int height)
{
  glViewport(0, 0, width, height);
}

void handleMouseChange(GLFWwindow* window, double xposIn, double yposIn)
{
  State* state = (State*)glfwGetWindowUserPointer(window);
  float xpos = (float)(xposIn);
  float ypos = (float)(yposIn);
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

void handleScrollChange(GLFWwindow* window, double xoffset, double yoffset)
{
  State* state = (State*)glfwGetWindowUserPointer(window);
  state->fov -= (float)yoffset;
  state->fov = std::clamp(state->fov, 1.0f, 45.0f);
}

void updateState(GLFWwindow* window, State& state)
{
  float time = glfwGetTime();
  state.time = time;
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
    state.cameraPosition += state.cameraFront * cameraTravel;
  }
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
  {
    state.cameraPosition -= state.cameraFront * cameraTravel;
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

int main()
{
  int glMajorVersion = 3, glMinorVersion = 3;
  int windowWidth = 800, windowHeight = 600;
  std::string windowTitle = {WINDOW_TITLE};
  std::filesystem::path staticFilePath = {STATIC_FILE_PATH};
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, glMajorVersion);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, glMinorVersion);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, windowTitle.c_str(), NULL, NULL);;
  if (window == NULL)
  {
    std::cout << "Failed to create window" << std::endl;
    glfwTerminate();
    return EXIT_FAILURE;
  }
  glfwMakeContextCurrent(window);
  int version = gladLoadGL(glfwGetProcAddress);
  if (version == -1)
  {
    std::cout << "Failed to link gl function pointers" << std::endl;
    glfwTerminate();
    return EXIT_FAILURE;
  }
  unsigned int cubeVertexShader = readShader(staticFilePath / "cube.vert", GL_VERTEX_SHADER);
  std::vector<unsigned int> redShaders = {
    cubeVertexShader,
    readShader(staticFilePath / "red.frag", GL_FRAGMENT_SHADER)
  };
  unsigned int redShaderProgram = createShaderProgram(redShaders);
  std::vector<unsigned int> greenShaders = {
    cubeVertexShader,
    readShader(staticFilePath / "green.frag", GL_FRAGMENT_SHADER)
  };
  unsigned int greenShaderProgram = createShaderProgram(greenShaders);
  std::vector<unsigned int> blueShaders = {
    cubeVertexShader,
    readShader(staticFilePath / "blue.frag", GL_FRAGMENT_SHADER)
  };
  unsigned int blueShaderProgram = createShaderProgram(blueShaders);
  std::vector<unsigned int> yellowShaders = {
    cubeVertexShader,
    readShader(staticFilePath / "yellow.frag", GL_FRAGMENT_SHADER)
  };
  unsigned int yellowShaderProgram = createShaderProgram(yellowShaders);
  std::vector<CubeVertex> cubeVertices = {
    CubeVertex {glm::vec3(-0.5f, -0.5f, -0.5f)},
    CubeVertex {glm::vec3( 0.5f,  0.5f, -0.5f)},
    CubeVertex {glm::vec3( 0.5f, -0.5f, -0.5f)},
    CubeVertex {glm::vec3( 0.5f,  0.5f, -0.5f)},
    CubeVertex {glm::vec3(-0.5f, -0.5f, -0.5f)},
    CubeVertex {glm::vec3(-0.5f,  0.5f, -0.5f)},

    CubeVertex {glm::vec3(-0.5f, -0.5f,  0.5f)},
    CubeVertex {glm::vec3( 0.5f, -0.5f,  0.5f)},
    CubeVertex {glm::vec3( 0.5f,  0.5f,  0.5f)},
    CubeVertex {glm::vec3( 0.5f,  0.5f,  0.5f)},
    CubeVertex {glm::vec3(-0.5f,  0.5f,  0.5f)},
    CubeVertex {glm::vec3(-0.5f, -0.5f,  0.5f)},

    CubeVertex {glm::vec3(-0.5f,  0.5f,  0.5f)},
    CubeVertex {glm::vec3(-0.5f,  0.5f, -0.5f)},
    CubeVertex {glm::vec3(-0.5f, -0.5f, -0.5f)},
    CubeVertex {glm::vec3(-0.5f, -0.5f, -0.5f)},
    CubeVertex {glm::vec3(-0.5f, -0.5f,  0.5f)},
    CubeVertex {glm::vec3(-0.5f,  0.5f,  0.5f)},
    
    CubeVertex {glm::vec3(0.5f,  0.5f,  0.5f)},
    CubeVertex {glm::vec3(0.5f, -0.5f, -0.5f)},
    CubeVertex {glm::vec3(0.5f,  0.5f, -0.5f)},
    CubeVertex {glm::vec3(0.5f, -0.5f, -0.5f)},
    CubeVertex {glm::vec3(0.5f,  0.5f,  0.5f)},
    CubeVertex {glm::vec3(0.5f, -0.5f,  0.5f)},

    CubeVertex {glm::vec3(-0.5f, -0.5f, -0.5f)},
    CubeVertex {glm::vec3( 0.5f, -0.5f, -0.5f)},
    CubeVertex {glm::vec3( 0.5f, -0.5f,  0.5f)},
    CubeVertex {glm::vec3( 0.5f, -0.5f,  0.5f)},
    CubeVertex {glm::vec3(-0.5f, -0.5f,  0.5f)},
    CubeVertex {glm::vec3(-0.5f, -0.5f, -0.5f)},

    CubeVertex {glm::vec3(-0.5f,  0.5f, -0.5f)},
    CubeVertex {glm::vec3( 0.5f,  0.5f,  0.5f)},
    CubeVertex {glm::vec3( 0.5f,  0.5f, -0.5f)},
    CubeVertex {glm::vec3( 0.5f,  0.5f,  0.5f)},
    CubeVertex {glm::vec3(-0.5f,  0.5f, -0.5f)},
    CubeVertex {glm::vec3(-0.5f,  0.5f,  0.5f)},
  };
  std::vector<Cube> cubes = {
    Cube {glm::vec3(-0.75f, 0.75f, 0.0f), redShaderProgram},
    Cube {glm::vec3(0.75f, 0.75f, 0.0f), greenShaderProgram},
    Cube {glm::vec3(-0.75f, -0.75f, 0.0f), blueShaderProgram},
    Cube {glm::vec3(0.75f, -0.75f, 0.0f), yellowShaderProgram},
  };
  unsigned int cubeVbo, cubeVao;
  glGenBuffers(1, &cubeVbo);
  glBindBuffer(GL_ARRAY_BUFFER, cubeVbo);
  glBufferData(GL_ARRAY_BUFFER, cubeVertices.size() * sizeof(CubeVertex), &cubeVertices[0], GL_STATIC_DRAW);
  glGenVertexArrays(1, &cubeVao);
  glBindVertexArray(cubeVao);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(CubeVertex), (void*)(offsetof(CubeVertex, position)));
  glEnableVertexAttribArray(0);
  glBindVertexArray(0);
  unsigned int matricesUbo;
  glGenBuffers(1, &matricesUbo);
  glBindBuffer(GL_UNIFORM_BUFFER, matricesUbo);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(Matrices), NULL, GL_STATIC_DRAW);
  glBindBuffer(GL_UNIFORM_BUFFER, 0);
  for (Cube& cube : cubes)
  {
    glUniformBlockBinding(cube.shaderProgram, glGetUniformBlockIndex(cube.shaderProgram, "Matrices"), 0);
  }
  glBindBufferBase(GL_UNIFORM_BUFFER, 0, matricesUbo);
  State state = {
    .cameraPosition = glm::vec3(0.0f, 0.0f, 3.0f),
    .cameraFront = glm::vec3(0.0f, 0.0f, -1.0f),
    .cameraUp = glm::vec3(0.0f, 1.0f,  0.0f),
    .cameraSpeed = 2.5,
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
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glfwSetWindowUserPointer(window, &state);
  glfwSetFramebufferSizeCallback(window, handleBufferSizeChange);
  glfwSetCursorPosCallback(window, handleMouseChange);
  glfwSetScrollCallback(window, handleScrollChange);
  while (!glfwWindowShouldClose(window))
  {
    updateState(window, state);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glm::mat4 view = glm::lookAt(state.cameraPosition, state.cameraPosition + state.cameraFront, state.cameraUp);
    glm::mat4 projection = glm::perspective(glm::radians(state.fov), (float)state.bufferWidth / (float)state.bufferHeight, 0.1f, 100.f);;
    glBindBuffer(GL_UNIFORM_BUFFER, matricesUbo);
    Matrices matrices = {.view = view, .projection = projection};
    glBufferData(GL_UNIFORM_BUFFER, sizeof(Matrices), &matrices, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindVertexArray(cubeVao);
    for (Cube& cube : cubes)
    {
      glUseProgram(cube.shaderProgram);
      glm::mat4 model = glm::translate(glm::mat4(1.0f), cube.position);
      glUniformMatrix4fv(glGetUniformLocation(cube.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
      glUniformMatrix4fv(glGetUniformLocation(cube.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
      glUniformMatrix4fv(glGetUniformLocation(cube.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
      glDrawArrays(GL_TRIANGLES, 0, cubeVertices.size());
    }
    glBindVertexArray(0);
    glfwSwapBuffers(window);
    glfwPollEvents();
  }
  glfwTerminate();
  return EXIT_SUCCESS;
}
