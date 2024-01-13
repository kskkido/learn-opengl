#include <optional>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stb_image.h>
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

struct CubeVertex
{
  glm::vec3 position;
  glm::vec2 textureCoordinate;
};

struct SkyBoxVertex
{
  glm::vec3 position;
};

std::stringstream readFile(std::filesystem::path path)
{
  std::ifstream file;
  std::stringstream content;
  file.open(path);
  content << file.rdbuf();
  file.close();
  return content;
}

unsigned int createShader(const char* shaderSource, GLenum shaderType)
{
  unsigned int shader = glCreateShader(shaderType);
  glShaderSource(shader, 1, &shaderSource, NULL);
  glCompileShader(shader);
  int success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success)
  {
    char infoLog[512];
    glGetShaderInfoLog(shader, 512, NULL, infoLog);
    std::cout << "ERROR::SHADER::" << shaderType << "COMPILATION_FAILED\n" << infoLog << std::endl;
  }
  return shader;
}

unsigned int readShader(std::filesystem::path path, GLenum shaderType)
{
  std::string source = readFile(path).str();
  return createShader(source.c_str(), shaderType);
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

GLenum textureFormatFromChannel(int channels)
{
  GLenum format = GL_RED;
  if (channels == 1)
      format = GL_RED;
  else if (channels == 3)
      format = GL_RGB;
  else if (channels == 4)
      format = GL_RGBA;
  return format;
}

unsigned int readTexture(std::filesystem::path path, bool flip)
{
  int width, height, channels;
  stbi_set_flip_vertically_on_load(flip);
  unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
  stbi_set_flip_vertically_on_load(false);
  unsigned int texture;
  glGenTextures(1, &texture);
  if (data)
  {
    GLenum format = textureFormatFromChannel(channels);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
  }
  else
  {
    std::cout << "Texture failed to load at path: " << path << std::endl;
  }
  stbi_image_free(data);
  return texture;
}

unsigned int readCubeMap(std::vector<std::filesystem::path>& paths, bool flip)
{
  unsigned int cubeMap;
  glGenTextures(1, &cubeMap);
  glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);
  for(unsigned int i = 0; i < paths.size(); i++)
  {
    std::filesystem::path path = paths[i];
    int width, height, channels;
    stbi_set_flip_vertically_on_load(flip);
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
    stbi_set_flip_vertically_on_load(false);
    if (data)
    {
      GLenum format = textureFormatFromChannel(channels);
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    }
    else
    {
      std::cout << "Texture failed to load at path: " << path << std::endl;
    }
    stbi_image_free(data);
  }
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
  return cubeMap;
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

void updateState(GLFWwindow* window, State* state)
{
  float time = glfwGetTime();
  state->time = time;
  state->deltaTime = state->time - state->lastFrame;
  state->lastFrame = state->time;
  glfwGetFramebufferSize(window, &state->bufferWidth, &state->bufferHeight);
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

int main()
{
  int glMajorVersion = 3, glMinorVersion = 3;
  int windowWidth = 800, windowHeight = 600;
  std::string windowTitle = {WINDOW_TITLE};
  std::filesystem::path staticFilePath = (std::filesystem::path){STATIC_FILE_PATH};
  glfwInit();
  // https://www.glfw.org/docs/latest/window.html#window_hints
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, glMajorVersion); // Target OpenGL 3.3
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, glMinorVersion); // Target OpenGL 3.3
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, windowTitle.c_str(), NULL, NULL);
  if (window == NULL)
  {
    std::cout << "Failed to initialize window" << std::endl;
    glfwTerminate();
    return EXIT_FAILURE;
  }
  glfwMakeContextCurrent(window);
  int version = gladLoadGL(glfwGetProcAddress);
  if (version == -1)
  {
    std::cout << "Failed to link gl function pointers" << std::endl;
    return EXIT_FAILURE;
  }
  std::vector<CubeVertex> cubeVertices = {
    CubeVertex {glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec2(0.0f, 0.0f)},
    CubeVertex {glm::vec3( 0.5f,  0.5f, -0.5f), glm::vec2(1.0f, 1.0f)},
    CubeVertex {glm::vec3( 0.5f, -0.5f, -0.5f), glm::vec2(1.0f, 0.0f)},
    CubeVertex {glm::vec3( 0.5f,  0.5f, -0.5f), glm::vec2(1.0f, 1.0f)},
    CubeVertex {glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec2(0.0f, 0.0f)},
    CubeVertex {glm::vec3(-0.5f,  0.5f, -0.5f), glm::vec2(0.0f, 1.0f)},

    CubeVertex {glm::vec3(-0.5f, -0.5f,  0.5f), glm::vec2(0.0f, 0.0f)},
    CubeVertex {glm::vec3( 0.5f, -0.5f,  0.5f), glm::vec2(1.0f, 0.0f)},
    CubeVertex {glm::vec3( 0.5f,  0.5f,  0.5f), glm::vec2(1.0f, 1.0f)},
    CubeVertex {glm::vec3( 0.5f,  0.5f,  0.5f), glm::vec2(1.0f, 1.0f)},
    CubeVertex {glm::vec3(-0.5f,  0.5f,  0.5f), glm::vec2(0.0f, 1.0f)},
    CubeVertex {glm::vec3(-0.5f, -0.5f,  0.5f), glm::vec2(0.0f, 0.0f)},

    CubeVertex {glm::vec3(-0.5f,  0.5f,  0.5f), glm::vec2(1.0f, 0.0f)},
    CubeVertex {glm::vec3(-0.5f,  0.5f, -0.5f), glm::vec2(1.0f, 1.0f)},
    CubeVertex {glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec2(0.0f, 1.0f)},
    CubeVertex {glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec2(0.0f, 1.0f)},
    CubeVertex {glm::vec3(-0.5f, -0.5f,  0.5f), glm::vec2(0.0f, 0.0f)},
    CubeVertex {glm::vec3(-0.5f,  0.5f,  0.5f), glm::vec2(1.0f, 0.0f)},
    
    CubeVertex {glm::vec3(0.5f,  0.5f,  0.5f), glm::vec2(1.0f, 0.0f)},
    CubeVertex {glm::vec3(0.5f, -0.5f, -0.5f), glm::vec2(0.0f, 1.0f)},
    CubeVertex {glm::vec3(0.5f,  0.5f, -0.5f), glm::vec2(1.0f, 1.0f)},
    CubeVertex {glm::vec3(0.5f, -0.5f, -0.5f), glm::vec2(0.0f, 1.0f)},
    CubeVertex {glm::vec3(0.5f,  0.5f,  0.5f), glm::vec2(1.0f, 0.0f)},
    CubeVertex {glm::vec3(0.5f, -0.5f,  0.5f), glm::vec2(0.0f, 0.0f)},

    CubeVertex {glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec2(0.0f, 1.0f)},
    CubeVertex {glm::vec3( 0.5f, -0.5f, -0.5f), glm::vec2(1.0f, 1.0f)},
    CubeVertex {glm::vec3( 0.5f, -0.5f,  0.5f), glm::vec2(1.0f, 0.0f)},
    CubeVertex {glm::vec3( 0.5f, -0.5f,  0.5f), glm::vec2(1.0f, 0.0f)},
    CubeVertex {glm::vec3(-0.5f, -0.5f,  0.5f), glm::vec2(0.0f, 0.0f)},
    CubeVertex {glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec2(0.0f, 1.0f)},

    CubeVertex {glm::vec3(-0.5f,  0.5f, -0.5f), glm::vec2(0.0f, 1.0f)},
    CubeVertex {glm::vec3( 0.5f,  0.5f,  0.5f), glm::vec2(1.0f, 0.0f)},
    CubeVertex {glm::vec3( 0.5f,  0.5f, -0.5f), glm::vec2(1.0f, 1.0f)},
    CubeVertex {glm::vec3( 0.5f,  0.5f,  0.5f), glm::vec2(1.0f, 0.0f)},
    CubeVertex {glm::vec3(-0.5f,  0.5f, -0.5f), glm::vec2(0.0f, 1.0f)},
    CubeVertex {glm::vec3(-0.5f,  0.5f,  0.5f), glm::vec2(0.0f, 0.0f)},
  };
  std::vector<glm::vec3> cubePositions = {
    glm::vec3(0.0f, 0.0f, 0.0f)
  };
  std::vector<SkyBoxVertex> skyBoxVertices = {
    SkyBoxVertex {glm::vec3(-1.0f,  1.0f, -1.0f)},
    SkyBoxVertex {glm::vec3(-1.0f, -1.0f, -1.0f)},
    SkyBoxVertex {glm::vec3( 1.0f, -1.0f, -1.0f)},
    SkyBoxVertex {glm::vec3( 1.0f, -1.0f, -1.0f)},
    SkyBoxVertex {glm::vec3( 1.0f,  1.0f, -1.0f)},
    SkyBoxVertex {glm::vec3(-1.0f,  1.0f, -1.0f)},

    SkyBoxVertex {glm::vec3(-1.0f, -1.0f,  1.0f)},
    SkyBoxVertex {glm::vec3(-1.0f, -1.0f, -1.0f)},
    SkyBoxVertex {glm::vec3(-1.0f,  1.0f, -1.0f)},
    SkyBoxVertex {glm::vec3(-1.0f,  1.0f, -1.0f)},
    SkyBoxVertex {glm::vec3(-1.0f,  1.0f,  1.0f)},
    SkyBoxVertex {glm::vec3(-1.0f, -1.0f,  1.0f)},

    SkyBoxVertex {glm::vec3(1.0f, -1.0f, -1.0f)},
    SkyBoxVertex {glm::vec3(1.0f, -1.0f,  1.0f)},
    SkyBoxVertex {glm::vec3(1.0f,  1.0f,  1.0f)},
    SkyBoxVertex {glm::vec3(1.0f,  1.0f,  1.0f)},
    SkyBoxVertex {glm::vec3(1.0f,  1.0f, -1.0f)},
    SkyBoxVertex {glm::vec3(1.0f, -1.0f, -1.0f)},

    SkyBoxVertex {glm::vec3(-1.0f, -1.0f,  1.0f)},
    SkyBoxVertex {glm::vec3(-1.0f,  1.0f,  1.0f)},
    SkyBoxVertex {glm::vec3( 1.0f,  1.0f,  1.0f)},
    SkyBoxVertex {glm::vec3( 1.0f,  1.0f,  1.0f)},
    SkyBoxVertex {glm::vec3( 1.0f, -1.0f,  1.0f)},
    SkyBoxVertex {glm::vec3(-1.0f, -1.0f,  1.0f)},

    SkyBoxVertex {glm::vec3(-1.0f,  1.0f, -1.0f)},
    SkyBoxVertex {glm::vec3( 1.0f,  1.0f, -1.0f)},
    SkyBoxVertex {glm::vec3( 1.0f,  1.0f,  1.0f)},
    SkyBoxVertex {glm::vec3( 1.0f,  1.0f,  1.0f)},
    SkyBoxVertex {glm::vec3(-1.0f,  1.0f,  1.0f)},
    SkyBoxVertex {glm::vec3(-1.0f,  1.0f, -1.0f)},

    SkyBoxVertex {glm::vec3(-1.0f, -1.0f, -1.0f)},
    SkyBoxVertex {glm::vec3(-1.0f, -1.0f,  1.0f)},
    SkyBoxVertex {glm::vec3( 1.0f, -1.0f, -1.0f)},
    SkyBoxVertex {glm::vec3( 1.0f, -1.0f, -1.0f)},
    SkyBoxVertex {glm::vec3(-1.0f, -1.0f,  1.0f)},
    SkyBoxVertex {glm::vec3( 1.0f, -1.0f,  1.0f)},
  };
  std::vector<unsigned int> cubeShaders = {
    readShader(staticFilePath / "cubemaps.vert", GL_VERTEX_SHADER),
    readShader(staticFilePath / "cubemaps.frag", GL_FRAGMENT_SHADER),
  };
  unsigned int cubeShaderProgram = createShaderProgram(cubeShaders);
  std::vector<unsigned int> skyBoxShaders = {
    readShader(staticFilePath / "skybox.vert", GL_VERTEX_SHADER),
    readShader(staticFilePath / "skybox.frag", GL_FRAGMENT_SHADER),
  };
  unsigned int skyBoxShaderProgram = createShaderProgram(skyBoxShaders);
  unsigned int cubeVbo, cubeVao;
  glGenBuffers(1, &cubeVbo);
  glBindBuffer(GL_ARRAY_BUFFER, cubeVbo);
  glBufferData(GL_ARRAY_BUFFER, cubeVertices.size() * sizeof(CubeVertex), &cubeVertices[0], GL_STATIC_DRAW);
  glGenVertexArrays(1, &cubeVao);
  glBindVertexArray(cubeVao);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(CubeVertex), (void*)(offsetof(CubeVertex, position)));
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(CubeVertex), (void*)(offsetof(CubeVertex, textureCoordinate)));
  glEnableVertexAttribArray(1);
  glBindVertexArray(0);
  unsigned int skyBoxVbo, skyBoxVao;
  glGenBuffers(1, &skyBoxVbo);
  glBindBuffer(GL_ARRAY_BUFFER, skyBoxVbo);
  glBufferData(GL_ARRAY_BUFFER, skyBoxVertices.size() * sizeof(SkyBoxVertex), &skyBoxVertices[0], GL_STATIC_DRAW);
  glGenVertexArrays(1, &skyBoxVao);
  glBindVertexArray(skyBoxVao);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SkyBoxVertex), (void*)(offsetof(SkyBoxVertex, position)));
  glEnableVertexAttribArray(0);
  glBindVertexArray(0);
  // textures
  unsigned int cubeTexture = readTexture(staticFilePath / "marble.jpg", false);
  std::vector<std::filesystem::path> skyBoxFilePaths = {
    staticFilePath / "resources/skybox/right.jpg",
    staticFilePath / "resources/skybox/left.jpg",
    staticFilePath / "resources/skybox/top.jpg",
    staticFilePath / "resources/skybox/bottom.jpg",
    staticFilePath / "resources/skybox/front.jpg",
    staticFilePath / "resources/skybox/back.jpg",
  };
  unsigned int skyBoxTexture = readCubeMap(skyBoxFilePaths, false);
  glUseProgram(cubeShaderProgram);
  glUniform1i(glGetUniformLocation(cubeShaderProgram, "texture1"), 0);
  glUseProgram(skyBoxShaderProgram);
  glUniform1i(glGetUniformLocation(skyBoxShaderProgram, "texture1"), 0);
  glUseProgram(0);
  State state = State {
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
    .bufferHeight = 0
  };
  glfwGetFramebufferSize(window, &state.bufferWidth, &state.bufferHeight);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glfwSetWindowUserPointer(window, &state);
  glfwSetFramebufferSizeCallback(window, handleBufferSizeChange);
  glfwSetCursorPosCallback(window, handleMouseChange);
  while (!glfwWindowShouldClose(window))
  {
    updateState(window, &state);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    // compute cube
    glBindVertexArray(cubeVao);
    glUseProgram(cubeShaderProgram);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, cubeTexture);
    glm::mat4 view = glm::lookAt(state.cameraPosition,  state.cameraPosition + state.cameraFront, state.cameraUp);
    glm::mat4 projection = glm::perspective(glm::radians(state.fov), (float)state.bufferWidth / (float)state.bufferHeight, 0.1f, 100.f);;
    glUniformMatrix4fv(glGetUniformLocation(cubeShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(cubeShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    for (glm::vec3& position : cubePositions)
    {
      glm::mat4 model = glm::mat4(1.0f);
      model = glm::translate(model, position);
      glUniformMatrix4fv(glGetUniformLocation(cubeShaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
      glDrawArrays(GL_TRIANGLES, 0, cubeVertices.size());
    }
    glBindVertexArray(0);
    // compute sky box
    glBindVertexArray(skyBoxVao);
    glDepthFunc(GL_LEQUAL);
    glUseProgram(skyBoxShaderProgram);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyBoxTexture);
    glUniformMatrix4fv(glGetUniformLocation(skyBoxShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(glm::mat4(glm::mat3(view))));
    glUniformMatrix4fv(glGetUniformLocation(skyBoxShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glDrawArrays(GL_TRIANGLES, 0, skyBoxVertices.size());
    glDepthFunc(GL_LESS);
    glBindVertexArray(0);
    // draw to screen
    glfwSwapBuffers(window);
    glfwPollEvents();
  }
  glfwTerminate();
  return EXIT_SUCCESS;
}
