#include <optional>
#include <tuple>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
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
  float time;
  float deltaTime;
  float lastFrame;
  std::optional<float> lastX;
  std::optional<float> lastY;
  int bufferWidth;
  int bufferHeight;
};

struct AsteroidInstanceVertex
{
  glm::mat4 model;
};

struct MeshVertex
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
  std::vector<MeshVertex> vertices;
  std::vector<unsigned int> indices;
  std::vector<Texture> textures;
  unsigned int vao;
  unsigned int vbo;
  unsigned int ebo;
};

struct Model
{
  std::vector<Mesh> meshes;
};

struct ModelLoadContext
{
  std::filesystem::path filename;
  std::filesystem::path directory;
  std::vector<Texture> textures;
};

std::stringstream readFile(std::filesystem::path path)
{
  std::ifstream file;
  file.open(path);
  std::stringstream stream;
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
  if(!success)
  {
    char infoLog[512];
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

unsigned int createShaderProgram(std::vector<unsigned int> shaders)
{
  unsigned int shaderProgram = glCreateProgram();
  for (unsigned int shader : shaders)
  {
    glAttachShader(shaderProgram, shader);
  }
  glLinkProgram(shaderProgram);
  int success;
  glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
  if(!success)
  {
    char infoLog[512];
    glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
    std::cout << "ERROR::SHADER_PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
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

unsigned int loadTexture(std::filesystem::path path, bool flip)
{
  int width, height, channels;
  stbi_set_flip_vertically_on_load(flip);
  unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
  stbi_set_flip_vertically_on_load(false);
  unsigned int textureId;
  glGenTextures(1, &textureId);
  if (data)
  {
    GLenum format = textureFormatFromChannel(channels);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  }
  else
  {
    std::cout << "Texture failed to load at path: " << path << std::endl;
  }
  stbi_image_free(data);
  return textureId;
}

std::vector<Texture> loadModelTextures(aiMaterial* material, aiTextureType textureType, std::string textureTypeName, ModelLoadContext& context)
{
  std::vector<Texture> textures = {};
  for (unsigned int i = 0; i < material->GetTextureCount(textureType); i++)
  {
    aiString path;
    material->GetTexture(textureType, i, &path);
    std::filesystem::path filename = context.directory / (std::filesystem::path)path.C_Str();
    bool skip = false;
    for (Texture& texture : context.textures)
    {
      if (texture.filename == filename)
      {
        textures.push_back(texture);
        skip = true;
        break;
      }
    }
    if (!skip)
    {
      Texture texture;
      texture.id = loadTexture(filename, true);
      texture.type = textureTypeName;
      texture.filename = filename;
      textures.push_back(texture);
      context.textures.push_back(texture);
    }
  }
  return textures;
}

Mesh loadModelMesh(aiMesh *aiMesh, const aiScene *scene, ModelLoadContext& context)
{
  Mesh mesh = {
    .vertices = {},
    .indices = {},
    .textures = {},
  };
  for (unsigned int i = 0; i < aiMesh->mNumVertices; i++)
  {
    MeshVertex vertex;
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
  std::vector<Texture> diffuseMap = loadModelTextures(material, aiTextureType_DIFFUSE, "texture_diffuse", context);
  context.textures.insert(context.textures.end(), diffuseMap.begin(), diffuseMap.end());
  std::vector<Texture> specularMap = loadModelTextures(material, aiTextureType_SPECULAR, "texture_specular", context);
  context.textures.insert(context.textures.end(), specularMap.begin(), specularMap.end());
  std::vector<Texture> normalMap = loadModelTextures(material, aiTextureType_HEIGHT, "texture_normal", context);
  context.textures.insert(context.textures.end(), normalMap.begin(), normalMap.end());
  std::vector<Texture> heightMap = loadModelTextures(material, aiTextureType_AMBIENT, "texture_height", context);
  context.textures.insert(context.textures.end(), heightMap.begin(), heightMap.end());
  mesh.textures.insert(mesh.textures.end(), context.textures.begin(), context.textures.end());
  return mesh;
}

std::vector<Mesh> loadModelMeshes(aiNode *node, const aiScene *scene, ModelLoadContext& context)
{
  std::vector<Mesh> meshes = {};
  for (unsigned int i = 0; i < node->mNumMeshes; i++)
  {
    aiMesh* aiMesh = scene->mMeshes[node->mMeshes[i]];
    meshes.push_back(loadModelMesh(aiMesh, scene, context));
  }
  for (unsigned int i = 0; i < node->mNumChildren; i++)
  {
    std::vector<Mesh> meshes_ = loadModelMeshes(node->mChildren[i], scene, context);
    meshes.insert(meshes.end(), meshes_.begin(), meshes_.end());
  }
  return meshes;
}

void setupMesh(Mesh& mesh)
{
  glGenVertexArrays(1, &mesh.vao);
  glGenBuffers(1, &mesh.vbo);
  glGenBuffers(1, &mesh.ebo);
  glBindVertexArray(mesh.vao);
  glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
  glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(MeshVertex), &mesh.vertices[0], GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)(offsetof(MeshVertex, position)));
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)(offsetof(MeshVertex, normal)));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)(offsetof(MeshVertex, textureCoordinate)));
  glEnableVertexAttribArray(2);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(unsigned int), &mesh.indices[0], GL_STATIC_DRAW);
  glBindVertexArray(0);
}

Model loadModel(ModelLoadContext& context)
{
  Model model = { .meshes = {} };
  Assimp::Importer importer;
  const aiScene* aiScene = importer.ReadFile(context.filename.c_str(), aiProcess_Triangulate | aiProcess_FlipUVs);
  if(!aiScene || aiScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !aiScene->mRootNode)
  {
    std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
    return model;
  }
  model.meshes = loadModelMeshes(aiScene->mRootNode, aiScene, context);
  for (Mesh& mesh : model.meshes)
  {
    setupMesh(mesh);
  }
  return model;
}

void drawMesh(Mesh& mesh, unsigned int shaderProgram, unsigned int amount)
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
  glDrawElementsInstanced(GL_TRIANGLES, mesh.indices.size(), GL_UNSIGNED_INT, 0, amount);
  glBindVertexArray(0);
}

void drawModel(Model& model, unsigned int shaderProgram, unsigned int amount)
{
  for (Mesh& mesh : model.meshes)
  {
    drawMesh(mesh, shaderProgram, amount);
  }
}

void updateState(GLFWwindow* window, State& state)
{
  state.time = glfwGetTime();
  state.deltaTime = state.time - state.lastFrame;
  state.lastFrame = state.time;
  if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
  {
    glfwSetWindowShouldClose(window, GL_TRUE);
  }
  float cameraTravel = state.deltaTime * state.cameraSpeed;
  if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
  {
    state.cameraPosition += glm::normalize(state.cameraFront) * cameraTravel;
  }
  if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
  {
    state.cameraPosition -= glm::normalize(state.cameraFront) * cameraTravel;
  }
  if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
  {
    state.cameraPosition += glm::normalize(glm::cross(state.cameraFront, state.cameraUp)) * cameraTravel;
  }
  if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
  {
    state.cameraPosition -= glm::normalize(glm::cross(state.cameraFront, state.cameraUp)) * cameraTravel;
  }
}

void handleMousePositionUpdate(GLFWwindow* window, double xposin, double yposin)
{
  State* state = (State*)glfwGetWindowUserPointer(window);
  float xpos = (float)xposin;
  float ypos = (float)yposin;
  float xoffset = 0;
  float yoffset = 0;

  try
  {
    float lastX = state->lastX.value();
    xoffset = (xpos - lastX) * state->cameraSensitivity;
  }
  catch(const std::bad_optional_access& error)
  {
    xoffset = 0;
  }

  try
  {
    float lastY = state->lastY.value();
    yoffset = (lastY - ypos) * state->cameraSensitivity;
  }
  catch(const std::bad_optional_access& error)
  {
    xoffset = 0;
  }

  state->lastX = xpos;
  state->lastY = ypos;
  state->yaw += xoffset;
  state->pitch += yoffset;
  state->pitch = std::clamp(state->pitch, -89.0f, 89.0f);

  glm::vec3 direction;
  direction.x = cos(glm::radians(state->yaw)) * cos(glm::radians(state->pitch)); // scale x by xs displacement of pitch, imagine adjusting pitch with an angled yaw
  direction.y = sin(glm::radians(state->pitch));
  direction.z = sin(glm::radians(state->yaw)) * cos(glm::radians(state->pitch));
  state->cameraFront = glm::normalize(direction);
}

void handleScrollUpdate(GLFWwindow* window, double xoffset, double yoffset)
{
  State* state = (State*)glfwGetWindowUserPointer(window);
  state->fov -= (float)yoffset;
  state->fov = std::clamp(state->fov, 1.0f, 45.0f);
}

void handleFrameBufferSizeUpdate(GLFWwindow* window, int width, int height)
{
  glViewport(0, 0, width, height);
}

int main()
{
  std::tuple<int,int> glVersion = {3, 3};
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
    std::cout << "Unable to create window" << std::endl;
    glfwTerminate();
    return EXIT_FAILURE;
  }
  glfwMakeContextCurrent(window);
  int version = gladLoadGL(glfwGetProcAddress);
  if (version == -1)
  {
    std::cout << "Unable to link OpenGL" << std::endl;
    glfwTerminate();
    return EXIT_FAILURE;
  }
  std::vector<unsigned int> asteroidShaders =
  {
    loadShader(staticFilePath / "asteroid.vert", GL_VERTEX_SHADER),
    loadShader(staticFilePath / "asteroid.frag", GL_FRAGMENT_SHADER),
  };
  unsigned int asteroidShaderProgram = createShaderProgram(asteroidShaders);
  std::vector<unsigned int> planetShaders =
  {
    loadShader(staticFilePath / "planet.vert", GL_VERTEX_SHADER),
    loadShader(staticFilePath / "planet.frag", GL_FRAGMENT_SHADER),
  };
  unsigned int planetShaderProgram = createShaderProgram(planetShaders);
  ModelLoadContext asteroidLoadContext =
  {
    .directory = staticFilePath / "resources/rock",
    .filename = staticFilePath / "resources/rock/rock.obj",
    .textures = {}
  };
  Model asteroid = loadModel(asteroidLoadContext);
  ModelLoadContext planetLoadContext =
  {
    .directory = staticFilePath / "resources/planet",
    .filename = staticFilePath / "resources/planet/planet.obj",
    .textures = {}
  };
  Model planet = loadModel(planetLoadContext);
  unsigned int amount = 10000;
  std::vector<AsteroidInstanceVertex> asteroidInstanceVertices = {};
  srand((unsigned int)glfwGetTime()); // initialize random seed
  float radius = 150.0;
  float offset = 25.0f;
  for (unsigned int i = 0; i < amount; i++)
  {
    glm::mat4 model = glm::mat4(1.0f);
    // 1. translation: displace along circle with 'radius' in range [-offset, offset]
    float angle = (float)i / (float)amount * 360.0f;
    float displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
    float x = sin(angle) * radius + displacement;
    displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
    float y = displacement * 0.4f; // keep height of asteroid field smaller compared to width of x and z
    displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
    float z = cos(angle) * radius + displacement;
    model = glm::translate(model, glm::vec3(x, y, z));

    // 2. scale: Scale between 0.05 and 0.25f
    float scale = (float)(rand() % 20) / 100.0 + 0.05;
    model = glm::scale(model, glm::vec3(scale));

    // 3. rotation: add random rotation around a (semi)randomly picked rotation axis vector
    float rotAngle = (float)(rand() % 360);
    model = glm::rotate(model, rotAngle, glm::vec3(0.4f, 0.6f, 0.8f));

    // 4. now add to list of matrices
    asteroidInstanceVertices.push_back(AsteroidInstanceVertex {model});
  }
  unsigned int asteroidInstanceVbo;
  glGenBuffers(1, &asteroidInstanceVbo);
  glBindBuffer(GL_ARRAY_BUFFER, asteroidInstanceVbo);
  glBufferData(GL_ARRAY_BUFFER, asteroidInstanceVertices.size() * sizeof(AsteroidInstanceVertex), &asteroidInstanceVertices[0], GL_STATIC_DRAW);
  for (Mesh& mesh : asteroid.meshes)
  {
    for (unsigned int i = 0; i < 4; i++)
    {
      unsigned int index = i + 3;
      glBindVertexArray(mesh.vao);
      glVertexAttribPointer(index, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(i * sizeof(glm::vec4)));
      glEnableVertexAttribArray(index);
      glVertexAttribDivisor(index, 1);
      glBindVertexArray(0);
    }
  }
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  State state =
  {
    .cameraPosition = glm::vec3(0.0f, 0.0f, 155.0f),
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
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_CAPTURED);
  glfwSetWindowUserPointer(window, &state);
  glfwGetFramebufferSize(window, &state.bufferWidth, &state.bufferHeight);
  glfwSetFramebufferSizeCallback(window, handleFrameBufferSizeUpdate);
  glfwSetCursorPosCallback(window, handleMousePositionUpdate);
  glfwSetScrollCallback(window, handleScrollUpdate);
  while (!glfwWindowShouldClose(window))
  {
    updateState(window, state);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glm::mat4 projection = glm::perspective(glm::radians(state.fov), (float)state.bufferWidth / (float)state.bufferHeight, 0.1f, 1000.0f);
    glm::mat4 view = glm::lookAt(state.cameraPosition, state.cameraPosition + state.cameraFront, state.cameraUp);
    glUseProgram(planetShaderProgram);
    glm::mat4 planetModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -3.0f, 0.0f));
    planetModel = glm::scale(planetModel, glm::vec3(4.0f, 4.0f, 4.0f));
    glUniformMatrix4fv(glGetUniformLocation(planetShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(planetShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(planetShaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(planetModel));
    drawModel(planet, planetShaderProgram, 1);
    glUseProgram(asteroidShaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(asteroidShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(asteroidShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    drawModel(asteroid, asteroidShaderProgram, amount);
    glfwSwapBuffers(window);
    glfwPollEvents();
  }
  return EXIT_SUCCESS;
}
