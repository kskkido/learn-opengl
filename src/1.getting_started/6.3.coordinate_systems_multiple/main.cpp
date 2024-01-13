#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <filesystem>
#include <cmath>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

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

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, GL_TRUE);
}

void frameBufferSizeCallback(GLFWwindow* window, int width, int height)
{
  glViewport(0, 0, width, height);
}

int main()
{
  const GLuint width = 800, height = 600;
  const std::string staticFilePath = {STATIC_FILE_PATH};
  std::filesystem::path vertexShaderFilePath = staticFilePath;
  vertexShaderFilePath /= "texture.vert";
  std::filesystem::path fragmentShaderFilePath = staticFilePath;
  fragmentShaderFilePath /= "texture.frag";
  std::filesystem::path containerTextureFilePath = staticFilePath;
  containerTextureFilePath /= "container.jpg";
  std::filesystem::path awesomeFaceTextureFilePath = staticFilePath;
  awesomeFaceTextureFilePath /= "awesomeface.png";
  float vertices[] = {
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
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
  ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

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
  int containerTextureWidth, containerTextureHeight;
  unsigned char *containerTextureData = readImage(containerTextureFilePath, containerTextureWidth, containerTextureHeight);
  if (!containerTextureData)
  {
    std::cout << "Failed to load containerTexture" << std::endl;
    return EXIT_FAILURE;
  }
  int awesomeFaceTextureWidth, awesomeFaceTextureHeight;
  stbi_set_flip_vertically_on_load(true);
  unsigned char *awesomeFaceTextureData = readImage(awesomeFaceTextureFilePath, awesomeFaceTextureWidth, awesomeFaceTextureHeight);
  stbi_set_flip_vertically_on_load(false);
  if (!awesomeFaceTextureData)
  {
    std::cout << "Failed to load awesomeFaceTexture" << std::endl;
    return EXIT_FAILURE;
  }
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO(); (void)io;
  ImGui::StyleColorsDark();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330");
  std::string vertexShaderSource = readFile(vertexShaderFilePath);
  std::string fragmentShaderSource = readFile(fragmentShaderFilePath);
  unsigned int vertexShader = { createShader(GL_VERTEX_SHADER, vertexShaderSource.c_str()) }; // generated and assign unique shader ID
  unsigned int fragmentShader = { createShader(GL_FRAGMENT_SHADER, fragmentShaderSource.c_str()) }; // generated and assign unique shader ID
  std::vector<unsigned int> shaders = {vertexShader, fragmentShader};
  unsigned int shaderProgram = { createShaderProgram(shaders) };                                                                                           
  unsigned int vbo;
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  unsigned int vao;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);
  unsigned int containerTexture;
  glGenTextures(1, &containerTexture);
  glBindTexture(GL_TEXTURE_2D, containerTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, containerTextureWidth, containerTextureHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, containerTextureData);
  glGenerateMipmap(GL_TEXTURE_2D);
  stbi_image_free(containerTextureData);
  unsigned int awesomeFaceTexture;
  glGenTextures(1, &awesomeFaceTexture);
  glBindTexture(GL_TEXTURE_2D, awesomeFaceTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, awesomeFaceTextureWidth, awesomeFaceTextureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, awesomeFaceTextureData);
  glGenerateMipmap(GL_TEXTURE_2D);
  stbi_image_free(awesomeFaceTextureData);
  glUseProgram(shaderProgram);
  glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);
  glUniform1i(glGetUniformLocation(shaderProgram, "texture2"), 1);
  glEnable(GL_DEPTH_TEST);
  glViewport(0, 0, width, height);
  glfwSetKeyCallback(window, keyCallback);
  glfwSetFramebufferSizeCallback(window, frameBufferSizeCallback);
  while (!glfwWindowShouldClose(window))
  {
    float time = (float)glfwGetTime();
    glfwPollEvents();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::Begin("Adjust clear color");
    ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color
    ImGui::End();
    ImGui::Render();
    glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shaderProgram);
    glBindVertexArray(vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, containerTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, awesomeFaceTexture);
    glm::mat4 view = glm::mat4(1.0f);
    view = glm::translate(view, glm::vec3(0.0f, 0.0f, -3.0f));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glm::mat4 projection;
    projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 100.f);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    for (glm::vec3& cubePosition: cubePositions)
    {
      glm::mat4 model = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first
      model = glm::translate(model, cubePosition);
      model = glm::rotate(model, time * glm::radians(50.0f), glm::vec3(0.5f, 0.5f, 0.0f));
      glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
      glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
  }
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  for (unsigned int shader : shaders)
  {
    glDeleteShader(shader);
  }
  glDeleteVertexArrays(1, &vao);
  glDeleteBuffers(1, &vbo);
  glfwTerminate();
  return EXIT_SUCCESS;
}
