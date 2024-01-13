#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <filesystem>
#include <cmath>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, GL_TRUE);
}

void frameBufferSizeCallback(GLFWwindow* window, int width, int height)
{
  glViewport(0, 0, width, height);
}

std::string readFile(const char* filePath)
{
  std::ifstream fileHandle;
  std::string file;
  fileHandle.exceptions (std::ifstream::failbit | std::ifstream::badbit);
  try 
  {
    // open files
    fileHandle.open(filePath);
    std::stringstream fileStream;
    // read file's buffer contents into streams
    fileStream << fileHandle.rdbuf();
    // close file handlers
    fileHandle.close();
    // convert stream into string
    file = fileStream.str();
  }
  catch (std::ifstream::failure& e)
  {
      std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << std::endl;
  }
  return file;
}

unsigned int createShader(GLenum shaderType, const char* shaderSource)
{
  unsigned int shader = { glCreateShader(shaderType) }; // generated and assign unique shader ID
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
  for(unsigned int& shader : shaders)
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

int main()
{
  const GLuint WIDTH = 800, HEIGHT = 600;

  std::cout << "Starting GLFW context, OpenGL 3.3" << std::endl;
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

  GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "LearnOpenGL", NULL, NULL);
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

  float verticesA[] = {
    -0.9f, -0.5f, 0.0f,  // left 
    -0.0f, -0.5f, 0.0f,  // right
    -0.45f, 0.5f, 0.0f,  // top 
  };
  float verticesB[] = {
    0.0f, -0.5f, 0.0f,  // left
    0.9f, -0.5f, 0.0f,  // right
    0.45f, 0.5f, 0.0f   // top 
  };
  std::string staticFilePath = {STATIC_FILE_PATH};
  std::filesystem::path vertexShaderFilePath = staticFilePath;
  vertexShaderFilePath /= "texture.vert";
  std::filesystem::path fragmentShaderFilePath = staticFilePath;
  fragmentShaderFilePath /= "texture.frag";
  std::string vertexShaderSource = readFile(vertexShaderFilePath.c_str());
  std::string fragmentShaderSource = readFile(fragmentShaderFilePath.c_str());
  unsigned int vertexShader = { createShader(GL_VERTEX_SHADER, vertexShaderSource.c_str()) }; // generated and assign unique shader ID
  unsigned int fragmentShader = { createShader(GL_FRAGMENT_SHADER, fragmentShaderSource.c_str()) }; // generated and assign unique shader ID
  std::vector<unsigned int> shaders = {vertexShader, fragmentShader};
  unsigned int shaderProgram = { createShaderProgram(shaders) };                                                                                           
  unsigned int vboA;
  glGenBuffers(1, &vboA); // generate and assign a unique buffer ID
  glBindBuffer(GL_ARRAY_BUFFER, vboA); // assign VBO buffer to GL_ARRAY_BUFFER, subsequent changes to GL_ARRAY_BUFFER configures VBO
  glBufferData(GL_ARRAY_BUFFER, sizeof(verticesA), verticesA, GL_STATIC_DRAW);
  unsigned int vaoA;
  glGenVertexArrays(1, &vaoA);
  glBindVertexArray(vaoA);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);
  unsigned int vboB;
  glGenBuffers(1, &vboB); // generate and assign a unique buffer ID
  glBindBuffer(GL_ARRAY_BUFFER, vboB); // assign VBO buffer to GL_ARRAY_BUFFER, subsequent changes to GL_ARRAY_BUFFER configures VBO
  glBufferData(GL_ARRAY_BUFFER, sizeof(verticesB), verticesB, GL_STATIC_DRAW);
  unsigned int vaoB;
  glGenVertexArrays(1, &vaoB);
  glBindVertexArray(vaoB);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO(); (void)io;
  ImGui::StyleColorsDark();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330");
  glViewport(0, 0, WIDTH, HEIGHT);
  glfwSetKeyCallback(window, keyCallback);
  glfwSetFramebufferSizeCallback(window, frameBufferSizeCallback);
  ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
  while (!glfwWindowShouldClose(window))
  {
    glfwPollEvents();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::Begin("Adjust clear color");
    ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color
    ImGui::End();
    ImGui::Render();
    glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(shaderProgram);
    float timeValue = glfwGetTime();
    int vertexColorLocation = glGetUniformLocation(shaderProgram, "ourColor");
    glUniform4f(vertexColorLocation, 0.0f, sin(timeValue) / 2.0f + 0.5f, 0.0f, 1.0f);
    glBindVertexArray(vaoA);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(vaoB);
    glDrawArrays(GL_TRIANGLES, 0, 3);
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
  glfwTerminate();
  return EXIT_SUCCESS;
}
