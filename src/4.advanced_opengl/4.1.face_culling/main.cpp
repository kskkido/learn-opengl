#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <format>
#include <filesystem>
#include <cmath>
#include <map>
#include <optional>
#include <fmt/core.h>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

struct State
{
  glm::vec3 cameraPosition;
  glm::vec3 cameraFront;
  glm::vec3 cameraUp;
  float cameraSpeed;
  float cameraSensitivity;
  float deltaTime;
  float lastFrame;
  float fov;
  float yaw;
  float pitch;
  std::optional<float> lastX;
  std::optional<float> lastY;
};

struct Vertex
{
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 textureCoordinate;
};

struct Texture
{
  unsigned int id;
  std::string type;
  std::filesystem::path filename;
};

struct Mesh
{
  std::vector<Vertex> vertices;
  std::vector<unsigned int> indices;
  std::vector<Texture> textures;
  unsigned int vbo;
  unsigned int vao;
  unsigned int ebo;
};

struct Model
{
  std::vector<Mesh> meshes;
};

struct ModelContext
{
  std::filesystem::path filename;
  std::filesystem::path directory;
  std::vector<Texture> textures;
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

unsigned char* readImage(std::filesystem::path& path, int& width, int& height, int& nrChannels)
{
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

GLenum textureFormatFromChannel(int nrChannels)
{
  GLenum format;
  if (nrChannels == 1)
      format = GL_RED;
  else if (nrChannels == 3)
      format = GL_RGB;
  else if (nrChannels == 4)
      format = GL_RGBA;
  return format;
}

void setupMesh(Mesh& mesh)
{
  glGenVertexArrays(1, &mesh.vao);
  glGenBuffers(1, &mesh.vbo);
  glGenBuffers(1, &mesh.ebo);
  glBindVertexArray(mesh.vao);
  glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
  glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(Vertex), &mesh.vertices[0], GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, position)));
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, normal)));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, textureCoordinate)));
  glEnableVertexAttribArray(2);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(unsigned int), &mesh.indices[0], GL_STATIC_DRAW);
  glBindVertexArray(0);
}

void drawMesh(Mesh& mesh, unsigned int shaderProgram)
{
  unsigned int diffuseNr = 1;
  unsigned int specularNr = 1;
  unsigned int normalNr = 1;
  unsigned int heightNr = 1;
  for (unsigned int i = 0; i < mesh.textures.size(); i++)
  {
    Texture texture = mesh.textures[i];
    glActiveTexture(GL_TEXTURE0 + i);
    std::string number;
    std::string name = texture.type;
    if (name == "texture_diffuse")
      number = std::to_string(diffuseNr++);
    else if (name == "texture_specular")
      number = std::to_string(specularNr++);
    else if (name == "texture_normal")
      number = std::to_string(normalNr++);
    else if (name == "texture_height")
      number = std::to_string(heightNr++);
    std::string location = name + number;
    glUniform1i(glGetUniformLocation(shaderProgram, &location[0]), i);
    glBindTexture(GL_TEXTURE_2D, texture.id);
  }
  glActiveTexture(GL_TEXTURE0);
  glBindVertexArray(mesh.vao);
  glDrawElements(GL_TRIANGLES, mesh.indices.size(), GL_UNSIGNED_INT, 0);
  glBindVertexArray(0);
}

unsigned int readTexture(std::filesystem::path& filename, bool flip)
{
  unsigned int textureId;
  glGenTextures(1, &textureId);
  int width, height, nrComponents;
  stbi_set_flip_vertically_on_load(flip);
  unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
  if (data)
  {
    GLenum format = textureFormatFromChannel(nrComponents);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(data);
  }
  else
  {
    std::cout << "Texture failed to load at path: " << filename << std::endl;
    stbi_image_free(data);
  }
  stbi_set_flip_vertically_on_load(false);
  return textureId;
}

std::vector<Texture> readMaterialTextures(aiMaterial* material, aiTextureType textureType, std::string textureTypeName, ModelContext& context)
{
  std::vector<Texture> textures;
  for (unsigned int i = 0; i < material->GetTextureCount(textureType); i++)
  {
    aiString path;
    material->GetTexture(textureType, i, &path);
    std::filesystem::path filename = context.directory / (std::filesystem::path)path.C_Str();
    bool skip = false;
    for (Texture& texture : context.textures)
    {
      if(texture.filename == filename)
      {
        textures.push_back(texture);
        skip = true;
        break;
      }
    }
    if (!skip)
    {
      Texture texture;
      texture.id = readTexture(filename, true);
      texture.type = textureTypeName;
      texture.filename = filename;
      textures.push_back(texture);
      context.textures.push_back(texture);
    }
  }
  return textures;
}

Mesh meshFromAiMesh(aiMesh *aiMesh, const aiScene *scene, ModelContext& context)
{
  Mesh mesh = Mesh
    { .vertices = {},
      .indices = {},
      .textures = {},
    };
  for (unsigned int i = 0; i < aiMesh->mNumVertices; i++)
  {
    Vertex vertex;
    glm::vec3 position;
    position.x = aiMesh->mVertices[i].x;
    position.y = aiMesh->mVertices[i].y;
    position.z = aiMesh->mVertices[i].z;
    vertex.position = position;
    glm::vec3 normal;
    normal.x = aiMesh->mNormals[i].x;
    normal.y = aiMesh->mNormals[i].y;
    normal.z = aiMesh->mNormals[i].z;
    vertex.normal = normal;
    glm::vec2 textureCoordinate;
    textureCoordinate.x = aiMesh->mTextureCoords[0][i].x;
    textureCoordinate.y = aiMesh->mTextureCoords[0][i].y;
    vertex.textureCoordinate = textureCoordinate;
    mesh.vertices.push_back(vertex);
  }
  for (unsigned int i = 0; i < aiMesh->mNumFaces; i++)
  {
    aiFace face = aiMesh->mFaces[i];
    for (unsigned int j = 0; j < face.mNumIndices; j++)
    {
      unsigned int indice = face.mIndices[j];
      mesh.indices.push_back(indice);
    }
  }
  aiMaterial* material = scene->mMaterials[aiMesh->mMaterialIndex];
  std::vector<Texture> diffuseMap = readMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse", context);
  context.textures.insert(context.textures.end(), diffuseMap.begin(), diffuseMap.end());
  std::vector<Texture> specularMap = readMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular", context);
  context.textures.insert(context.textures.end(), specularMap.begin(), specularMap.end());
  std::vector<Texture> normalMap = readMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal", context);
  context.textures.insert(context.textures.end(), normalMap.begin(), normalMap.end());
  std::vector<Texture> heightMap = readMaterialTextures(material, aiTextureType_AMBIENT, "texture_height", context);
  context.textures.insert(context.textures.end(), heightMap.begin(), heightMap.end());
  mesh.textures.insert(mesh.textures.end(), context.textures.begin(), context.textures.end());
  return mesh;
}

std::vector<Mesh> meshesFromAiNode(aiNode *node, const aiScene *scene, ModelContext& context)
{
  std::vector<Mesh> meshes = {};
  for(unsigned int i = 0; i < node->mNumMeshes; i++)
  {
    aiMesh *aiMesh = scene->mMeshes[node->mMeshes[i]];
    meshes.push_back(meshFromAiMesh(aiMesh, scene, context));
  }
  // then do the same for each of its children
  for(unsigned int i = 0; i < node->mNumChildren; i++)
  {
    std::vector<Mesh> meshes_ = meshesFromAiNode(node->mChildren[i], scene, context);
    meshes.insert(meshes.end(), meshes_.begin(), meshes_.end());
  }
  return meshes;
}

Model readModel(ModelContext& context)
{
  Model model = Model
    { .meshes = {}
    };
  Assimp::Importer importer;
  const aiScene *scene = importer.ReadFile(context.filename.c_str(), aiProcess_Triangulate | aiProcess_FlipUVs);
  if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
  {
    std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
    return model;
  }
  std::vector<Mesh> meshes = meshesFromAiNode(scene->mRootNode, scene, context);
  for (Mesh& mesh : meshes)
  {
    setupMesh(mesh);
  }
  model.meshes = meshes;
  return model;
}

void drawModel(Model& model, unsigned int shaderProgram)
{
  for (Mesh& mesh: model.meshes)
  {
    drawMesh(mesh, shaderProgram);
  }
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
  ImVec4 clearColor = ImVec4(0.1f, 0.1f, 0.1f, 1.00f);
  std::filesystem::path staticFilePath = (std::filesystem::path){STATIC_FILE_PATH};
  std::filesystem::path textureVertexShaderFilePath = staticFilePath / "texture.vert";
  std::filesystem::path textureFragmentShaderFilePath = staticFilePath / "texture.frag";
  std::filesystem::path outlineFragmentShaderFilePath = staticFilePath / "outline.frag";
  std::filesystem::path marbleTextureFilePath = staticFilePath / "marble.jpg";
  std::filesystem::path metalTextureFilePath = staticFilePath / "metal.png";
  std::filesystem::path windowTextureFilePath = staticFilePath / "window.png";
  std::vector<float> cubeVertices = {
    // positions          // texture Coords
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, // bottom-left
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // top-right
     0.5f, -0.5f, -0.5f,  1.0f, 0.0f, // bottom-right
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // top-right
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, // bottom-left
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, // top-left
    // front face
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // bottom-left
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f, // bottom-right
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f, // top-right
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f, // top-right
    -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, // top-left
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // bottom-left
    // left face
    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // top-right
    -0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // top-left
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // bottom-left
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // bottom-left
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // bottom-right
    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // top-right
    // right face
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // top-left
     0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // bottom-right
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // top-right
     0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // bottom-right
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // top-left
     0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // bottom-left
    // bottom face
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // top-right
     0.5f, -0.5f, -0.5f,  1.0f, 1.0f, // top-left
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f, // bottom-left
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f, // bottom-left
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // bottom-right
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // top-right
    // top face
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, // top-left
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // bottom-right
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // top-right
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // bottom-right
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, // top-left
    -0.5f,  0.5f,  0.5f,  0.0f, 0.0f  // bottom-left
  };
  std::vector<glm::vec3> cubePositions = {
    glm::vec3(-1.0f, 0.0f, -1.0f),
    glm::vec3(1.0f, 0.0f, 1.0f),
  };
  std::vector<float> windowVertices = {
    // positions         // texture Coords (swapped y coordinates because texture is flipped upside down)
    0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
    0.0f, -0.5f,  0.0f,  0.0f,  1.0f,
    1.0f, -0.5f,  0.0f,  1.0f,  1.0f,
    0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
    1.0f, -0.5f,  0.0f,  1.0f,  1.0f,
    1.0f,  0.5f,  0.0f,  1.0f,  0.0f
  };
  std::vector<glm::vec3> windowPositions = {
    glm::vec3(-1.5f, 0.0f, -0.48f),
    glm::vec3( 1.5f, 0.0f, 0.51f),
    glm::vec3( 0.0f, 0.0f, 0.7f),
    glm::vec3(-0.3f, 0.0f, -2.3f),
    glm::vec3 (0.5f, 0.0f, -0.6f)
  };
  std::vector<float> planeVertices = {
       5.0f, -0.5f,  5.0f,  2.0f, 0.0f,
      -5.0f, -0.5f,  5.0f,  0.0f, 0.0f,
      -5.0f, -0.5f, -5.0f,  0.0f, 2.0f,

       5.0f, -0.5f,  5.0f,  2.0f, 0.0f,
      -5.0f, -0.5f, -5.0f,  0.0f, 2.0f,
       5.0f, -0.5f, -5.0f,  2.0f, 2.0f								
  };
  State state = State
    { .cameraPosition = glm::vec3(0.0f, 0.0f, 3.0f),
      .cameraFront = glm::vec3(0.0f, 0.0f, -1.0f),
      .cameraUp = glm::vec3(0.0f, 1.0f,  0.0f),
      .cameraSpeed = 2.5,
      .cameraSensitivity = 0.1f,
      .fov = 45.0f,
      .yaw = -90.0f,
      .pitch = 0.0f,
      .deltaTime = 0.0f,
      .lastFrame = 0.0f,
    };

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
  std::string textureVertexShaderSource = readFile(textureVertexShaderFilePath);
  unsigned int textureVertexShader = createShader(GL_VERTEX_SHADER, textureVertexShaderSource.c_str());
  std::string textureFragmentShaderSource = readFile(textureFragmentShaderFilePath);
  unsigned int textureFragmentShader = createShader(GL_FRAGMENT_SHADER, textureFragmentShaderSource.c_str());
  std::string outlineFragmentShaderSource = readFile(outlineFragmentShaderFilePath);
  unsigned int outlineFragmentShader = createShader(GL_FRAGMENT_SHADER, outlineFragmentShaderSource.c_str());
  std::vector<unsigned int> textureShaders = {textureVertexShader, textureFragmentShader};
  std::vector<unsigned int> outlineShaders = {textureVertexShader, outlineFragmentShader};
  unsigned int textureShaderProgram = createShaderProgram(textureShaders);
  unsigned int outlineShaderProgram = createShaderProgram(outlineShaders);
  unsigned int cubeVbo, cubeVao;
  glGenBuffers(1, &cubeVbo);
  glBindBuffer(GL_ARRAY_BUFFER, cubeVbo);
  glBufferData(GL_ARRAY_BUFFER, cubeVertices.size() * sizeof(float), &cubeVertices[0], GL_STATIC_DRAW);
  glGenVertexArrays(1, &cubeVao);
  glBindVertexArray(cubeVao);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(0));
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);
  unsigned int windowVbo, windowVao;
  glGenBuffers(1, &windowVbo);
  glBindBuffer(GL_ARRAY_BUFFER, windowVbo);
  glBufferData(GL_ARRAY_BUFFER, windowVertices.size() * sizeof(float), &windowVertices[0], GL_STATIC_DRAW);
  glGenVertexArrays(1, &windowVao);
  glBindVertexArray(windowVao);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(0));
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);
  unsigned int planeVbo, planeVao;
  glGenBuffers(1, &planeVbo);
  glBindBuffer(GL_ARRAY_BUFFER, planeVbo);
  glBufferData(GL_ARRAY_BUFFER, planeVertices.size() * sizeof(float), &planeVertices[0], GL_STATIC_DRAW);
  glGenVertexArrays(1, &planeVao);
  glBindVertexArray(planeVao);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(0));
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);
  unsigned int cubeTexture = readTexture(marbleTextureFilePath, true);
  unsigned int planeTexture = readTexture(metalTextureFilePath, true);
  unsigned int windowTexture = readTexture(windowTextureFilePath, false);
  glUseProgram(textureShaderProgram);
  glUniform1i(glGetUniformLocation(textureShaderProgram, "texture1"), 0);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS); // always pass the depth test (same effect as glDisable(GL_DEPTH_TEST))
  glEnable(GL_BLEND);
  glDepthFunc(GL_LESS);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glViewport(0, 0, width, height);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glfwSetWindowUserPointer(window, &state);
  glfwSetFramebufferSizeCallback(window, handleFrameBufferUpdate);
  glfwSetCursorPosCallback(window, handleMouseUpdate);
  glfwSetScrollCallback(window, handleScrollUpdate);
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO(); (void)io;
  ImGui::StyleColorsDark();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330");
  while (!glfwWindowShouldClose(window))
  {
    float time = (float)glfwGetTime();
    state.deltaTime = time - state.lastFrame;
    state.lastFrame = time;
    handleInput(window, &state);
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
    glUseProgram(textureShaderProgram);
    glm::mat4 view = glm::lookAt(state.cameraPosition, state.cameraPosition + state.cameraFront, state.cameraUp);
    glm::mat4 projection = glm::perspective(glm::radians(state.fov), (float)width / (float)height, 0.1f, 100.f);
    glUniformMatrix4fv(glGetUniformLocation(textureShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(textureShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUseProgram(outlineShaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(outlineShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(outlineShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUseProgram(textureShaderProgram);
    // plane
    glBindVertexArray(planeVao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, planeTexture);
    glm::mat4 planeModel = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(textureShaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(planeModel));
    glDrawArrays(GL_TRIANGLES, 0, 6);
    // cube
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glBindVertexArray(cubeVao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, cubeTexture);
    for (glm::vec3& position : cubePositions)
    {
      glm::mat4 cubeModel = glm::mat4(1.0f);
      cubeModel = glm::translate(cubeModel, position);
      glUniformMatrix4fv(glGetUniformLocation(textureShaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(cubeModel));
      glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    glDisable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    // window
    glBindVertexArray(windowVao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, windowTexture);
    std::map<float, glm::vec3> sortedWindowPositions;
    for (glm::vec3& position : windowPositions)
    {
      float distance = glm::length(state.cameraPosition - position);
      sortedWindowPositions[distance] = position;
    }
    for (std::map<float,glm::vec3>::reverse_iterator it = sortedWindowPositions.rbegin(); it != sortedWindowPositions.rend(); ++it)
    {
      glm::mat4 windowModel = glm::mat4(1.0f);
      windowModel = glm::translate(windowModel, it->second);
      glUniformMatrix4fv(glGetUniformLocation(textureShaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(windowModel));
      glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
  }
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwTerminate();
  return EXIT_SUCCESS;
}
