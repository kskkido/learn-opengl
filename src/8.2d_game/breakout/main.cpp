#include <tuple>
#include <vector>
#include <optional>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/gl.h>
#include <GLFW/glfw3.h>

struct Texture
{
  // holds the ID of the texture object, used for all texture operations to reference to this particular texture
  unsigned int id;
  // texture image dimensions
  int width;
  int height; // width and height of loaded image in pixels
  int channels;
};

struct RenderState
{
  float time;
  float deltaTime;
  float lastFrame;
  int bufferWidth;
  int bufferHeight;
};

struct WindowSettings
{
  int width;
  int height;
  std::string title;
};

struct SpriteVertex
{
  glm::vec2 position;
  glm::vec2 textureCoordinate;
};

struct Sprite
{
  Texture texture;
  std::vector<SpriteVertex> vertices;
  unsigned int vao;
  unsigned int vbo;
};

struct ParticleVertex
{
  glm::vec2 position;
  glm::vec2 textureCoordinate;
};

struct ParticleIntanceVertex
{
  glm::vec2 offset;
  glm::vec4 color;
};

struct Particle
{
  Texture texture;
  std::vector<ParticleVertex> vertices;
  unsigned int vao;
  unsigned int vbo;
  unsigned int instanceVbo;
};

struct PostProcessorVertex
{
  glm::vec2 position;
  glm::vec2 textureCoordinate;
};

struct PostProcessor
{
  std::vector<PostProcessorVertex> vertices;
  unsigned int vao;
  unsigned int vbo;
  unsigned int fbo;
  unsigned int rbo;
  unsigned int tid;
  bool confuse;
  bool chaos;
  bool shake;
  float offsets[9][2];
  int edgeKernel[9];
  float blurKernel[9];
};

struct ShakeEffectConfig
{
  float duration;
};

struct ShakeEffect
{
  float ttl;
};

struct EntityAttributes
{
  glm::vec2 position;
  glm::vec2 size;
  float rotation;
  glm::vec3 color;
};

enum TileType
{
  TILE_TYPE_SOLID,
  TILE_TYPE_EMPTY,
  TILE_TYPE_DESTROYABLE
};

struct Tile
{
  TileType type;
  glm::vec3 color;
};

struct TileMap
{
  std::vector<std::vector<Tile>> grid;
};

enum Direction
{
  DIRECTION_UP,
  DIRECTION_DOWN,
  DIRECTION_LEFT,
  DIRECTION_RIGHT,
};

struct Collision
{
  Direction direction;
  glm::vec2 difference;
};

struct AabbCollisionBox
{
  glm::vec2 topLeft;
  glm::vec2 bottomRight;
};

struct AabbCollisionCircle
{
  float radius;
  glm::vec2 center;
};

enum GameObjectBodyType
{
  GAME_OBJECT_BODY_SOLID,
  GAME_OBJECT_BODY_DESTROYABLE
};

enum GameObjectStatus
{
  GAME_OBJECT_ALIVE,
  GAME_OBJECT_DESTROYED,
};

struct GameObject
{
  glm::vec2 position;
  glm::vec2 size;
  float rotation;
  glm::vec3 color;
  GameObjectBodyType bodyType;
  GameObjectStatus status;
  Sprite sprite;
};

struct ParticleObject
{
  glm::vec2 position;
  glm::vec2 velocity;
  glm::vec4 color;
  float life;
};

enum BallObjectSurfaceType
{
  BALL_OBJECT_SURFACE_STICKY,
  BALL_OBJECT_SURFACE_REFLECT,
};

enum BallObjectCollisionType
{
  BALL_OBJECT_COLLISION_DEFAULT,
  BALL_OBJECT_COLLISION_PASS_THROUGH,
};

struct BallConfig
{
  float speed;
  float radius;
  glm::vec3 color;
  Sprite sprite;
  unsigned int particleCount;
  Particle particleModel;
};

struct BallObject : GameObject
{
  float speed;
  float radius;
  glm::vec2 velocity;
  BallObjectSurfaceType surfaceType;
  BallObjectCollisionType collisionType;
  std::vector<ParticleObject> particles;
  Particle particleModel;
};

struct PlayerConfig
{
  float velocity;
  glm::vec2 size;
  glm::vec3 color;
  Sprite sprite;
};

struct PlayerObject : GameObject
{
  float velocity;
};

enum PowerUpType
{
  POWER_UP_SPEED,
  POWER_UP_STICKY,
  POWER_UP_PASS_THROUGH,
  POWER_UP_PADDLE_SIZE_UP,
  POWER_UP_CONFUSION,
  POWER_UP_CHAOS,
};

struct PowerUpConfig
{
  PowerUpType type;
  int chance;
  float ttl;
  Sprite sprite;
  glm::vec2 velocity;
  glm::vec2 size;
  glm::vec3 color;
};

struct PowerUpObject : GameObject
{
  PowerUpType type;
  glm::vec2 velocity;
};

enum PowerUpEffectStatus
{
  POWER_UP_EFFECT_IDLE,
  POWER_UP_EFFECT_STATUS_ACTIVATE,
  POWER_UP_EFFECT_STATUS_ACTIVATED,
  POWER_UP_EFFECT_STATUS_DEACTIVATE,
  POWER_UP_EFFECT_STATUS_DEACTIVATED
};

struct PowerUpEffect
{
  PowerUpType type;
  float ttl;
  PowerUpEffectStatus status;
};

struct GameLevelConfig
{
  TileMap tileMap;
  unsigned int width;
  unsigned int height;
  PlayerConfig playerConfig;
  BallConfig ballConfig;
  ShakeEffectConfig shakeEffectConfig;
  std::vector<PowerUpConfig> powerUpConfigs;
  Sprite background;
  Sprite blockSolid;
  Sprite blockDestroyable;
  unsigned int spriteShader;
  unsigned int particleShader;
  unsigned int postProcessorShader;
};

struct GameLevelMap
{
  unsigned int width;
  unsigned int height;
  std::vector<GameObject> bricks;
  glm::mat4 projection;
  Sprite background;
};

struct GameLevel
{
  GameLevelConfig config;
  GameLevelMap map;
  PlayerObject player;
  BallObject ball;
  PostProcessor postProcessor;
  ShakeEffect shakeEffect;
  std::vector<PowerUpObject> powerUps;
  std::vector<PowerUpEffect> powerUpEffects;
};

enum GameStatus
{
  GAME_ACTIVE,
  GAME_MENU,
  GAME_WIN,
};

struct GameState
{
  int width;
  int height;
  GameStatus status;
  std::vector<GameLevel> levels;
  unsigned int level;
};

std::stringstream readFile(std::filesystem::path path)
{
  std::ifstream file;
  file.open(path);
  std::stringstream content;
  content << file.rdbuf();
  file.close();
  return content;
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

Texture loadTexture(std::filesystem::path path, bool flip)
{
  int width, height, channels;
  stbi_set_flip_vertically_on_load(flip);
  unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
  stbi_set_flip_vertically_on_load(false);
  unsigned int id;
  glGenTextures(1, &id);
  if (data)
  {
    GLenum format = textureFormatFromChannel(channels);
    glBindTexture(GL_TEXTURE_2D, id);
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
  Texture texture =
  {
    .id = id,
    .width = width,
    .height = height,
    .channels = channels
  };
  return texture;
}

Sprite createSprite(Texture texture)
{
  std::vector<SpriteVertex> vertices =
  {
    SpriteVertex { glm::vec2(0.0f, 1.0f), glm::vec2(0.0f, 1.0f) },
    SpriteVertex { glm::vec2(1.0f, 0.0f), glm::vec2(1.0f, 0.0f) },
    SpriteVertex { glm::vec2(0.0f, 0.0f), glm::vec2(0.0f, 0.0f) },
    SpriteVertex { glm::vec2(0.0f, 1.0f), glm::vec2(0.0f, 1.0f) },
    SpriteVertex { glm::vec2(1.0f, 1.0f), glm::vec2(1.0f, 1.0f) },
    SpriteVertex { glm::vec2(1.0f, 0.0f), glm::vec2(1.0f, 0.0f) },
  };
  Sprite sprite =
  {
    .texture = texture,
    .vertices = vertices
  };
  glGenVertexArrays(1, &sprite.vao);
  glBindVertexArray(sprite.vao);
  glGenBuffers(1, &sprite.vbo);
  glBindBuffer(GL_ARRAY_BUFFER, sprite.vbo);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(SpriteVertex), &vertices[0], GL_STATIC_DRAW);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(SpriteVertex), (void*)(offsetof(SpriteVertex, position)));
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(SpriteVertex), (void*)(offsetof(SpriteVertex, textureCoordinate)));
  glEnableVertexAttribArray(1);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
  return sprite;
}

Particle createParticle(Texture texture)
{
  std::vector<ParticleVertex> vertices =
  { ParticleVertex { glm::vec2(0.0f, 1.0f), glm::vec2(0.0f, 1.0f) },
    ParticleVertex { glm::vec2(1.0f, 0.0f), glm::vec2(1.0f, 0.0f) },
    ParticleVertex { glm::vec2(0.0f, 0.0f), glm::vec2(0.0f, 0.0f) },
    ParticleVertex { glm::vec2(0.0f, 1.0f), glm::vec2(0.0f, 1.0f) },
    ParticleVertex { glm::vec2(1.0f, 1.0f), glm::vec2(1.0f, 1.0f) },
    ParticleVertex { glm::vec2(1.0f, 0.0f), glm::vec2(1.0f, 0.0f) },
  };
  Particle particle = Particle
  { .vertices = vertices,
    .texture = texture
  };
  glGenVertexArrays(1, &particle.vao);
  glBindVertexArray(particle.vao);
  glGenBuffers(1, &particle.vbo);
  glGenBuffers(1, &particle.instanceVbo);
  glBindBuffer(GL_ARRAY_BUFFER, particle.vbo);
  glBufferData(GL_ARRAY_BUFFER, particle.vertices.size() * sizeof(ParticleVertex), &particle.vertices[0], GL_STATIC_DRAW);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex), (void*)(offsetof(ParticleVertex, position)));
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex), (void*)(offsetof(ParticleVertex, textureCoordinate)));
  glEnableVertexAttribArray(1);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
  return particle;
}

PostProcessor createPostProcessor(RenderState& renderState)
{
  std::vector<PostProcessorVertex> vertices =
  { PostProcessorVertex { glm::vec2(-1.0f, -1.0f), glm::vec2(0.0f, 0.0f) },
    PostProcessorVertex { glm::vec2(1.0f, 1.0f), glm::vec2(1.0f, 1.0f) },
    PostProcessorVertex { glm::vec2(-1.0f, 1.0f), glm::vec2(0.0f, 1.0f) },
    PostProcessorVertex { glm::vec2(-1.0f, -1.0f), glm::vec2(0.0f, 0.0f) },
    PostProcessorVertex { glm::vec2(1.0f, -1.0f), glm::vec2(1.0f, 0.0f) },
    PostProcessorVertex { glm::vec2(1.0f, 1.0f), glm::vec2(1.0f, 1.0f) },
  };
  float offset = 1.0f / 300.0f;
  PostProcessor postProcessor = PostProcessor
  { .vertices = vertices,
    .confuse = false,
    .chaos = false,
    .shake = false,
    .offsets = {
      { -offset,  offset  },  // top-left
      {  0.0f,    offset  },  // top-center
      {  offset,  offset  },  // top-right
      { -offset,  0.0f    },  // center-left
      {  0.0f,    0.0f    },  // center-center
      {  offset,  0.0f    },  // center - right
      { -offset, -offset  },  // bottom-left
      {  0.0f,   -offset  },  // bottom-center
      {  offset, -offset  }   // bottom-right    
    },
    .edgeKernel = {
      -1, -1, -1,
      -1,  8, -1,
      -1, -1, -1
    },
    .blurKernel = {
      1.0f / 16.0f, 2.0f / 16.0f, 1.0f / 16.0f,
      2.0f / 16.0f, 4.0f / 16.0f, 2.0f / 16.0f,
      1.0f / 16.0f, 2.0f / 16.0f, 1.0f / 16.0f
    }
  };
  glGenFramebuffers(1, &postProcessor.fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, postProcessor.fbo);
  glGenTextures(1, &postProcessor.tid);
  glBindTexture(GL_TEXTURE_2D, postProcessor.tid);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, renderState.bufferWidth, renderState.bufferHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, postProcessor.tid, 0);
  glGenRenderbuffers(1, &postProcessor.rbo);
  glBindRenderbuffer(GL_RENDERBUFFER, postProcessor.rbo);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, renderState.bufferWidth, renderState.bufferHeight);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, postProcessor.rbo);
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
  glBindRenderbuffer(GL_RENDERBUFFER, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glGenVertexArrays(1, &postProcessor.vao);
  glBindVertexArray(postProcessor.vao);
  glGenBuffers(1, &postProcessor.vbo);
  glBindBuffer(GL_ARRAY_BUFFER, postProcessor.vbo);
  glBufferData(GL_ARRAY_BUFFER, postProcessor.vertices.size() * sizeof(PostProcessorVertex), &postProcessor.vertices[0], GL_STATIC_DRAW);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(PostProcessorVertex), (void*)(offsetof(PostProcessorVertex, position)));
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(PostProcessorVertex), (void*)(offsetof(PostProcessorVertex, textureCoordinate)));
  glEnableVertexAttribArray(1);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
  return postProcessor;
}

PowerUpObject createPowerUp(PowerUpConfig& powerUpConfig, glm::vec2 position)
{
  PowerUpObject powerUp = PowerUpObject
  {
    .type = powerUpConfig.type,
    .velocity = powerUpConfig.velocity,
  };
  powerUp.position = position;
  powerUp.size = powerUpConfig.size;
  powerUp.rotation = 0.0f;
  powerUp.color = powerUpConfig.color;
  powerUp.bodyType = GAME_OBJECT_BODY_DESTROYABLE;
  powerUp.status = GAME_OBJECT_ALIVE;
  powerUp.sprite = powerUpConfig.sprite;
  return powerUp;
}

PowerUpEffect createPowerUpEffect(PowerUpConfig& powerUpConfig)
{
  PowerUpEffect powerUpEffect = PowerUpEffect
  {
    .type = powerUpConfig.type,
    .ttl = powerUpConfig.ttl,
    .status = POWER_UP_EFFECT_STATUS_ACTIVATE
  };
  return powerUpEffect;
}

void drawSprite(unsigned int shaderProgram, Sprite& sprite, EntityAttributes& attributes)
{
  glUseProgram(shaderProgram);
  glm::mat4 model = glm::mat4(1.0f);
  model = glm::translate(model, glm::vec3(attributes.position, 1.0f));
  glm::vec3 center = glm::vec3(attributes.size.x / 2.0, attributes.size.y / 2.0, 0.0f);
  model = glm::translate(model, center);
  model = glm::rotate(model, glm::radians(attributes.rotation), glm::vec3(0.0f, 0.0f, 1.0f));
  model = glm::translate(model, -center);
  model = glm::scale(model, glm::vec3(attributes.size, 1.0f));
  glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
  glUniform3fv(glGetUniformLocation(shaderProgram, "color"), 1, glm::value_ptr(attributes.color));
  glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, sprite.texture.id);
  glBindVertexArray(sprite.vao);
  glDrawArrays(GL_TRIANGLES, 0, sprite.vertices.size());
  glBindTexture(GL_TEXTURE_2D, 0);
  glBindVertexArray(0);
}

void drawParticles(unsigned int shaderProgram, Particle& particle, std::vector<ParticleObject>& particleObjects)
{
  glUseProgram(shaderProgram);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  glBindVertexArray(particle.vao);
  glBindBuffer(GL_ARRAY_BUFFER, particle.instanceVbo);
  std::vector<ParticleIntanceVertex> vertices = {};
  for (ParticleObject& particleObject : particleObjects)
  {
    if (particleObject.life > 0.0f)
    {
      ParticleIntanceVertex vertex = ParticleIntanceVertex
      { .offset = particleObject.position,
        .color = particleObject.color
      };
      vertices.push_back(vertex);
    }
  }
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ParticleIntanceVertex), &vertices[0], GL_STATIC_DRAW);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(ParticleIntanceVertex), (void*)(offsetof(ParticleIntanceVertex, offset)));
  glEnableVertexAttribArray(2);
  glVertexAttribDivisor(2, 1);
  glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(ParticleIntanceVertex), (void*)(offsetof(ParticleIntanceVertex, color)));
  glEnableVertexAttribArray(3);
  glVertexAttribDivisor(3, 1);
  glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, particle.texture.id);
  glBindVertexArray(particle.vao);
  glDrawArraysInstanced(GL_TRIANGLES, 0, particle.vertices.size(), vertices.size());
  glBindTexture(GL_TEXTURE_2D, 0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void drawBackground(unsigned int shaderProgram, int width, int height, Sprite& background)
{
  EntityAttributes spriteAttribute = 
  {
    .position = glm::vec2(0.0f, 0.0f),
    .size = glm::vec2(width, height),
    .rotation = 0.0f,
    .color = glm::vec3(1.0f),
  };
  drawSprite(shaderProgram, background, spriteAttribute);
}

void drawGameObject(unsigned int shaderProgram, GameObject& gameObject)
{
  if (gameObject.status == GAME_OBJECT_ALIVE)
  {
    EntityAttributes spriteAttribute = 
    {
      .position = gameObject.position,
      .size = gameObject.size,
      .rotation = gameObject.rotation,
      .color = gameObject.color,
    };
    drawSprite(shaderProgram, gameObject.sprite, spriteAttribute);
  }
}

void drawGameLevel(RenderState& renderState, GameLevel& gameLevel)
{
  glBindFramebuffer(GL_FRAMEBUFFER, gameLevel.postProcessor.fbo);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glClear(GL_COLOR_BUFFER_BIT);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glUseProgram(gameLevel.config.spriteShader);
  glUniformMatrix4fv(glGetUniformLocation(gameLevel.config.spriteShader, "projection"), 1, GL_FALSE, glm::value_ptr(gameLevel.map.projection));
  glUseProgram(gameLevel.config.particleShader);
  glUniformMatrix4fv(glGetUniformLocation(gameLevel.config.particleShader, "projection"), 1, GL_FALSE, glm::value_ptr(gameLevel.map.projection));
  drawBackground(gameLevel.config.spriteShader, gameLevel.map.width, gameLevel.map.height, gameLevel.map.background);
  for (GameObject& brick : gameLevel.map.bricks)
  {
    drawGameObject(gameLevel.config.spriteShader, brick);
  }
  for (PowerUpObject& powerUp : gameLevel.powerUps)
  {
    drawGameObject(gameLevel.config.spriteShader, powerUp);
  }
  drawGameObject(gameLevel.config.spriteShader, gameLevel.player);
  drawParticles(gameLevel.config.particleShader, gameLevel.ball.particleModel, gameLevel.ball.particles);
  drawGameObject(gameLevel.config.spriteShader, gameLevel.ball);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
  glDisable(GL_DEPTH_TEST);
  glClear(GL_COLOR_BUFFER_BIT);
  glUseProgram(gameLevel.config.postProcessorShader);
  glUniform1f(glGetUniformLocation(gameLevel.config.postProcessorShader, "time"), renderState.time);
  glUniform1i(glGetUniformLocation(gameLevel.config.postProcessorShader, "confuse"), gameLevel.postProcessor.confuse);
  glUniform1i(glGetUniformLocation(gameLevel.config.postProcessorShader, "chaos"), gameLevel.postProcessor.chaos);
  glUniform1i(glGetUniformLocation(gameLevel.config.postProcessorShader, "shake"), gameLevel.postProcessor.shake);
  glUniform2fv(glGetUniformLocation(gameLevel.config.postProcessorShader, "offsets"), 9, (float*)gameLevel.postProcessor.offsets);
  glUniform1iv(glGetUniformLocation(gameLevel.config.postProcessorShader, "edgeKernel"), 9, gameLevel.postProcessor.edgeKernel);
  glUniform1fv(glGetUniformLocation(gameLevel.config.postProcessorShader, "blurKernel"), 9, gameLevel.postProcessor.blurKernel);
  glBindVertexArray(gameLevel.postProcessor.vao);
  glUniform1i(glGetUniformLocation(gameLevel.config.postProcessorShader, "scene"), 0);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, gameLevel.postProcessor.tid);
  glDrawArrays(GL_TRIANGLES, 0, gameLevel.postProcessor.vertices.size());
}

void drawGameState(RenderState& renderState, GameState& gameState)
{
  GameLevel& gameLevel = gameState.levels[gameState.level];
  if (gameState.status == GAME_ACTIVE)
  {
    drawGameLevel(renderState, gameLevel);
  }
}

std::vector<std::vector<unsigned int>> loadTileData(std::filesystem::path path)
{
  std::vector<std::vector<unsigned int>> tileGrid;
  unsigned int tileCode;
  std::string line;
  std::ifstream file;
  file.open(path);
  while (std::getline(file, line))
  {
    std::istringstream content(line);
    std::vector<unsigned int> tileRow;
    while (content >> tileCode)
    {
      tileRow.push_back(tileCode);
    }
    tileGrid.push_back(tileRow);
  }
  return tileGrid;
}

Tile createTile(unsigned int tileData)
{
  Tile tile;
  if (tileData == 1)
  {
    tile.type = TILE_TYPE_SOLID;
    tile.color = glm::vec3(0.8f, 0.8f, 0.7f);
  }
  else if (tileData > 1)
  {
    tile.type = TILE_TYPE_DESTROYABLE;
    if (tileData == 2)
    {
      tile.color = glm::vec3(0.2f, 0.6f, 1.0f);
    }
    else if (tileData == 3)
    {
      tile.color = glm::vec3(0.0f, 0.7f, 0.0f);
    }
    else if (tileData == 4)
    {
      tile.color = glm::vec3(0.8f, 0.8f, 0.4f);
    }
    else if (tileData == 5)
    {
      tile.color = glm::vec3(1.0f, 0.5f, 0.0f);
    }
  }
  else
  {
    tile.type = TILE_TYPE_EMPTY;
    tile.color = glm::vec3(1.0f, 1.0f, 1.0f);
  }
  return tile;
}

TileMap createTileMap(std::vector<std::vector<unsigned int>> tileData)
{
  TileMap tileMap = { .grid = {} };
  for (std::vector<unsigned int>& tileRow : tileData)
  {
    std::vector<Tile> tileMapRow = {};
    for (unsigned int tileColumn : tileRow)
    {
      tileMapRow.push_back(createTile(tileColumn));
    }
    tileMap.grid.push_back(tileMapRow);
  }
  return tileMap;
}

GameLevelMap createGameLevelMap(GameLevelConfig gameLevelConfig)
{
  GameLevelMap gameLevelMap =
  {
    .width = gameLevelConfig.width,
    .height = gameLevelConfig.height,
    .background = gameLevelConfig.background,
    .projection = glm::ortho(0.0f, (float)gameLevelConfig.width, (float)gameLevelConfig.height, 0.0f),
    .bricks = {},
  };
  unsigned int width = gameLevelConfig.tileMap.grid[0].size();
  unsigned int height = gameLevelConfig.tileMap.grid.size();
  float blockWidth = gameLevelMap.width / (float)width;
  float blockHeight = (gameLevelMap.height / 2.0f) / (float)height;
  for (unsigned int y = 0; y < height; y++)
  {
    for (unsigned int x = 0; x < gameLevelConfig.tileMap.grid[y].size(); x++)
    {
      Tile tile = gameLevelConfig.tileMap.grid[y][x];
      glm::vec2 size(blockWidth, blockHeight);
      glm::vec2 position = size * glm::vec2(x, y);
      if (tile.type == TILE_TYPE_SOLID)
      {
        GameObject brick;
        brick.position = position;
        brick.size = size;
        brick.rotation = 0.0f;
        brick.color = tile.color;
        brick.bodyType = GAME_OBJECT_BODY_SOLID;
        brick.status = GAME_OBJECT_ALIVE;
        brick.sprite = gameLevelConfig.blockSolid;
        gameLevelMap.bricks.push_back(brick);
      }
      else if (tile.type == TILE_TYPE_DESTROYABLE)
      {
        GameObject brick;
        brick.position = position;
        brick.size = size;
        brick.rotation = 0.0f;
        brick.color = tile.color;
        brick.bodyType = GAME_OBJECT_BODY_DESTROYABLE;
        brick.status = GAME_OBJECT_ALIVE;
        brick.sprite = gameLevelConfig.blockDestroyable;
        gameLevelMap.bricks.push_back(brick);
      }
    }
  }
  return gameLevelMap;
}

BallObject createBallObject(GameLevelMap& gameLevelMap, BallConfig& ballConfig, PlayerObject& playerObject) 
{
  BallObject ball;
  ball.particles = {};
  for (unsigned int i = 0; i < ballConfig.particleCount; i++)
  {
    ParticleObject particle = ParticleObject
      { .position = glm::vec2(0.0f),
        .velocity = glm::vec2(0.0f),
        .color = glm::vec4(0.0f),
        .life = 0.0f
      };
    ball.particles.push_back(particle);
  }
  ball.particleModel = ballConfig.particleModel;
  ball.speed = ballConfig.speed;
  ball.surfaceType = BALL_OBJECT_SURFACE_STICKY;
  ball.radius = ballConfig.radius;
  ball.velocity = glm::vec2(0.0f);
  ball.size = glm::vec2(ballConfig.radius * 2.0f);
  ball.position = playerObject.position + glm::vec2(playerObject.size.x / 2.0f - ball.radius, -ball.radius * 2.0f);
  ball.rotation = 0.0f;
  ball.color = ballConfig.color;
  ball.bodyType = GAME_OBJECT_BODY_SOLID;
  ball.status = GAME_OBJECT_ALIVE;
  ball.sprite = ballConfig.sprite;
  return ball;
}

PlayerObject createPlayerObject(GameLevelMap& gameLevelMap, PlayerConfig& playerConfig)
{
  PlayerObject player;
  player.velocity = playerConfig.velocity;
  player.position = glm::vec2(std::max(gameLevelMap.width / 2.0f - playerConfig.size.x / 2.0f, 0.0f), gameLevelMap.height - playerConfig.size.y);
  player.size = playerConfig.size;
  player.rotation = 0.0f;
  player.color = playerConfig.color;
  player.bodyType = GAME_OBJECT_BODY_SOLID;
  player.status = GAME_OBJECT_ALIVE;
  player.sprite = playerConfig.sprite;
  return player;
}

GameLevel createGameLevel(RenderState renderState, GameLevelConfig config)
{
  GameLevelMap gameLevelMap = createGameLevelMap(config);
  PlayerObject playerObject = createPlayerObject(gameLevelMap, config.playerConfig);
  BallObject ballObject = createBallObject(gameLevelMap, config.ballConfig, playerObject);
  PostProcessor postProcessor = createPostProcessor(renderState);
  ShakeEffect shakeEffect = ShakeEffect { .ttl = 0.0f };
  GameLevel level =
  { .map = gameLevelMap,
    .player = playerObject,
    .ball = ballObject,
    .config = config,
    .postProcessor = postProcessor,
    .shakeEffect = shakeEffect,
    .powerUps = {},
  };
  return level;
}

AabbCollisionBox gameObjectToAabbCollisionBox(GameObject& gameObject)
{
  return AabbCollisionBox
  { .topLeft = gameObject.position,
    .bottomRight = gameObject.position + gameObject.size
  };
}

AabbCollisionCircle ballObjectToAabbCollisionCircle(BallObject& ballObject)
{
  return AabbCollisionCircle
  { .radius = ballObject.radius,
    .center = ballObject.position + glm::vec2(ballObject.radius)
  };
}

void handleBallObjectMovement(GLFWwindow* window, RenderState& renderState, GameLevel& gameLevel)
{
  gameLevel.ball.position += renderState.deltaTime * gameLevel.ball.velocity;
  if (gameLevel.ball.position.x <= 0.0f)
  {
    gameLevel.ball.velocity.x *= -1.0f;
    gameLevel.ball.position.x = 0;
  }
  else if (gameLevel.ball.position.x + gameLevel.ball.size.x >= gameLevel.map.width)
  {
    gameLevel.ball.velocity.x *= -1.0f;
    gameLevel.ball.position.x = gameLevel.map.width - gameLevel.ball.size.x;
  }
  else if (gameLevel.ball.position.y <= 0.0f)
  {
    gameLevel.ball.velocity.y *= -1.0f;
    gameLevel.ball.position.y = 0;
  }
  else if (gameLevel.ball.position.y + gameLevel.ball.size.y >= gameLevel.map.height)
  {
    // game over
    gameLevel = createGameLevel(renderState, gameLevel.config);
  }
}

void respawnBallObjectParticle(BallObject& ballObject, ParticleObject& particle)
{
  glm::vec2 offset = glm::vec2(ballObject.radius / 2.0f);
  float random = ((rand() % 100) - 50) / 10.0f;
  float rColor = 0.5f + ((rand() % 100) / 100.0f);
  particle.position = ballObject.position + random + offset;
  particle.color = glm::vec4(rColor, rColor, rColor, 1.0f);
  particle.life = 1.0f;
  particle.velocity = ballObject.velocity * 0.1f;
}

void handleBallObjectParticles(GLFWwindow* window, RenderState& renderState, GameLevel& gameLevel)
{
  for (unsigned int i = 0; i < 2; i++)
  {
    for (ParticleObject& particle : gameLevel.ball.particles)
    {
      if (particle.life <= 0.0f)
      {
        respawnBallObjectParticle(gameLevel.ball, particle);
        break;
      }
    }
  }
  for (ParticleObject& particle : gameLevel.ball.particles)
  {
    particle.life -= renderState.deltaTime;
    if (particle.life > 0.0f)
    {
      particle.position -= particle.velocity * renderState.deltaTime;
      particle.color.a -= renderState.deltaTime * 2.5f;
    }
  }
}

Direction directionFromTarget(glm::vec2 target)
{
  glm::vec2 normalized = glm::normalize(target);
  std::vector<std::tuple<Direction, glm::vec2>> directionByPositions = {
    {DIRECTION_UP, glm::vec2(0.0f,  1.0f)}, // up
    {DIRECTION_RIGHT, glm::vec2(1.0f,  0.0f)}, // right
    {DIRECTION_DOWN, glm::vec2(0.0f, -1.0f)}, // down
    {DIRECTION_LEFT, glm::vec2(-1.0f, 0.0f)}  // left
  };
  float max = 0.0f;
  Direction bestMatch = DIRECTION_UP;
  for (std::tuple<Direction, glm::vec2>& directionByPosition: directionByPositions)
  {
    float scale = glm::dot(normalized, std::get<1>(directionByPosition));
    if (scale > max)
    {
        max = scale;
        bestMatch = std::get<0>(directionByPosition);
    }
  }
  return bestMatch;
}

std::optional<Collision> checkCircleToBoxCollision(AabbCollisionCircle circle, AabbCollisionBox box)
{
  glm::vec2 halfExtent = (box.bottomRight - box.topLeft) / 2.0f;
  glm::vec2 center = box.topLeft + halfExtent;
  glm::vec2 closest = center + glm::clamp(circle.center - center, -halfExtent, halfExtent);
  glm::vec2 difference = closest - circle.center;
  if (glm::length(difference) > circle.radius)
  {
    return {};
  }
  return Collision
  { .direction = directionFromTarget(difference),
    .difference = difference
  };
}

std::optional<Collision> checkBoxToBoxCollision(AabbCollisionBox boxA, AabbCollisionBox boxB)
{
  bool collisionX = boxB.topLeft.x <= boxA.bottomRight.x && boxA.topLeft.x <= boxB.bottomRight.x;
  bool collisionY = boxB.topLeft.y <= boxA.bottomRight.y && boxA.topLeft.y <= boxB.bottomRight.y;
  if (collisionX && collisionY)
  {
    glm::vec2 difference = glm::vec2((boxA.topLeft + boxA.bottomRight) - (boxB.topLeft + boxB.bottomRight));
    return Collision
    { .direction = directionFromTarget(difference),
      .difference = difference
    };
  }
  return {};
}

void spawnPowerUp(PowerUpConfig& config, glm::vec2 position, GLFWwindow* window, RenderState& renderState, GameLevel& gameLevel)
{
  if ((rand() % config.chance) == 0)
  {
    gameLevel.powerUps.push_back(createPowerUp(config, position));
  }
}

void spawnPowerUps(glm::vec2 position, GLFWwindow* window, RenderState& renderState, GameLevel& gameLevel)
{
  for (PowerUpConfig& config : gameLevel.config.powerUpConfigs)
  {
    spawnPowerUp(config, position, window, renderState, gameLevel);
  }
}

void activatePowerUp(PowerUpEffect& powerUp, GLFWwindow* window, RenderState& renderState, GameLevel& gameLevel)
{
  switch (powerUp.type)
  {
    case POWER_UP_SPEED:
    {
      gameLevel.player.velocity *= 1.2f;
      break;
    }
    case POWER_UP_STICKY:
    {
      gameLevel.ball.surfaceType = BALL_OBJECT_SURFACE_STICKY;
      gameLevel.ball.color = glm::vec3(1.0f, 0.5f, 1.0f);
      break;
    }
    case POWER_UP_PASS_THROUGH:
    {
      gameLevel.ball.collisionType = BALL_OBJECT_COLLISION_PASS_THROUGH;
      gameLevel.ball.color = glm::vec3(1.0f, 0.5f, 0.5f);
      break;
    }
    case POWER_UP_PADDLE_SIZE_UP:
    {
      gameLevel.player.size.x += 50.0f;
      break;
    }
    case POWER_UP_CONFUSION:
    {
      gameLevel.postProcessor.confuse = true;
      break;
    }
    case POWER_UP_CHAOS:
    {
      gameLevel.postProcessor.chaos = true;
      break;
    }
  }
}

void deactivatePowerUp(PowerUpEffect& powerUp, GLFWwindow* window, RenderState& renderState, GameLevel& gameLevel)
{
  switch (powerUp.type)
  {
    case POWER_UP_SPEED:
    {
      gameLevel.player.velocity = gameLevel.config.playerConfig.velocity;
      break;
    }
    case POWER_UP_STICKY:
    {
      gameLevel.ball.surfaceType = BALL_OBJECT_SURFACE_REFLECT;
      gameLevel.ball.color = gameLevel.config.ballConfig.color;
      break;
    }
    case POWER_UP_PASS_THROUGH:
    {
      gameLevel.ball.collisionType = BALL_OBJECT_COLLISION_DEFAULT;
      gameLevel.ball.color = gameLevel.config.ballConfig.color;
      break;
    }
    case POWER_UP_PADDLE_SIZE_UP:
    {
      gameLevel.player.size = gameLevel.config.playerConfig.size;
      break;
    }
    case POWER_UP_CONFUSION:
    {
      gameLevel.postProcessor.confuse = false;
      break;
    }
    case POWER_UP_CHAOS:
    {
      gameLevel.postProcessor.chaos = false;
      break;
    }
  }
}

void handleGameLevelBrickCollision(GLFWwindow* window, RenderState& renderState, GameLevel& gameLevel)
{
  AabbCollisionCircle ballCollisionCircle = ballObjectToAabbCollisionCircle(gameLevel.ball);
  for (GameObject& brick : gameLevel.map.bricks)
  {
    if (brick.status != GAME_OBJECT_ALIVE)
    {
      continue;
    }
    std::optional<Collision> result = checkCircleToBoxCollision(
      ballCollisionCircle,
      gameObjectToAabbCollisionBox(brick)
    );
    if (!result.has_value())
    {
      continue;
    }
    Collision collision = result.value();
    if (brick.bodyType == GAME_OBJECT_BODY_DESTROYABLE)
    {
      brick.status = GAME_OBJECT_DESTROYED;
      spawnPowerUps(brick.position, window, renderState, gameLevel);
    } else if (brick.bodyType == GAME_OBJECT_BODY_SOLID)
    {
      if (gameLevel.ball.collisionType == BALL_OBJECT_COLLISION_PASS_THROUGH)
      {
        continue;
      }
      else
      {
        gameLevel.shakeEffect.ttl = gameLevel.config.shakeEffectConfig.duration;
      }
    }
    if (collision.direction == DIRECTION_LEFT || collision.direction == DIRECTION_RIGHT)
    {
      gameLevel.ball.velocity.x *= -1.0f;
      float penetration = gameLevel.ball.radius - std::abs(collision.difference.x);
      if (collision.direction == DIRECTION_LEFT)
      {
        gameLevel.ball.position.x += penetration;
      }
      else
      {
        gameLevel.ball.position.x -= penetration;
      }
    }
    else if (collision.direction == DIRECTION_UP || collision.direction == DIRECTION_DOWN)
    {
      gameLevel.ball.velocity.y *= -1.0f;
      float penetration = gameLevel.ball.radius - std::abs(collision.difference.y);
      if (collision.direction == DIRECTION_UP)
      {
        gameLevel.ball.position.y -= penetration;
      }
      else
      {
        gameLevel.ball.position.y += penetration;
      }
    }
  }
}

void handleGameLevelPlayerCollision(GLFWwindow* window, RenderState& renderState, GameLevel& gameLevel)
{
  std::optional<Collision> result = checkCircleToBoxCollision(
    ballObjectToAabbCollisionCircle(gameLevel.ball),
    gameObjectToAabbCollisionBox(gameLevel.player)
  );
  if (!result.has_value())
  {
    return;
  }
  Collision collision = result.value();
  if (collision.direction == DIRECTION_UP)
  {
    gameLevel.ball.position.y = gameLevel.player.position.y - gameLevel.ball.size.y;
  }
  else if (collision.direction == DIRECTION_DOWN)
  {
    gameLevel.ball.position.y = gameLevel.player.position.y + gameLevel.player.size.y;
  }
  else if (collision.direction == DIRECTION_LEFT)
  {
    gameLevel.ball.position.x = gameLevel.player.position.x - gameLevel.ball.size.x;
  }
  else if (collision.direction == DIRECTION_RIGHT)
  {
    gameLevel.ball.position.x = gameLevel.player.position.x + gameLevel.player.size.x;
  }
  if (gameLevel.ball.surfaceType == BALL_OBJECT_SURFACE_STICKY)
  {
    gameLevel.ball.velocity = glm::vec2(0.0f);
  }
  else
  {
    glm::vec2 half = gameLevel.player.size / 2.0f;
    glm::vec2 center = gameLevel.player.position + half;
    float distance = (gameLevel.ball.position.x + gameLevel.ball.radius) - center.x;
    float percentage = distance / half.x;
    float strength = 2.0f;
    glm::vec2 velocity = gameLevel.ball.velocity;
    gameLevel.ball.velocity.x = gameLevel.ball.speed * percentage * strength;
    gameLevel.ball.velocity = glm::normalize(gameLevel.ball.velocity) * glm::length(velocity);
    gameLevel.ball.velocity.y = -1.0f * abs(gameLevel.ball.velocity.y);
  }
}

void handleGameLevelCollision(GLFWwindow* window, RenderState& renderState, GameLevel& gameLevel)
{
  handleGameLevelBrickCollision(window, renderState, gameLevel);
  handleGameLevelPlayerCollision(window, renderState, gameLevel);
}

void movePlayerObject(GLFWwindow* window, RenderState& renderState, GameLevel& gameLevel, float travel)
{
  float x = gameLevel.player.position.x;
  float maxX = std::max(0.0f, (float)gameLevel.map.width - gameLevel.player.size.x);
  gameLevel.player.position.x += travel;
  gameLevel.player.position.x = std::clamp(gameLevel.player.position.x, 0.0f, maxX);
  if (gameLevel.ball.surfaceType != BALL_OBJECT_SURFACE_STICKY)
  {
    return;
  }
  std::optional<Collision> result = checkCircleToBoxCollision(
    ballObjectToAabbCollisionCircle(gameLevel.ball),
    gameObjectToAabbCollisionBox(gameLevel.player)
  );
  if (!result.has_value())
  {
    return;
  }
  float xdiff = gameLevel.player.position.x - x;
  Collision collision = result.value();
  gameLevel.ball.position.x += xdiff;
}

void handlePlayerInput(GLFWwindow* window, RenderState& renderState, GameLevel& gameLevel)
{
  float travel = gameLevel.player.velocity * renderState.deltaTime;
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
  {
    movePlayerObject(window, renderState, gameLevel, travel * -1);
  }
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
  {
    movePlayerObject(window, renderState, gameLevel, travel);
  }
  if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
  {
    if (gameLevel.ball.surfaceType == BALL_OBJECT_SURFACE_STICKY) {
      std::optional<Collision> result = checkCircleToBoxCollision(
        ballObjectToAabbCollisionCircle(gameLevel.ball),
        gameObjectToAabbCollisionBox(gameLevel.player)
      );
      if (result.has_value())
      {
        gameLevel.ball.surfaceType = BALL_OBJECT_SURFACE_REFLECT;
        gameLevel.ball.velocity = gameLevel.ball.speed * glm::normalize(glm::vec2(1.0f));
      }
    }
  }
}

void handleUpdatePowerUpObject(GLFWwindow* window, RenderState& renderState, GameLevel& gameLevel)
{
  AabbCollisionBox playerCollisionBox = gameObjectToAabbCollisionBox(gameLevel.player);
  for (auto i = gameLevel.powerUps.begin(); i != gameLevel.powerUps.end();)
  {
    PowerUpObject& powerUp = *i;
    powerUp.position += renderState.deltaTime * powerUp.velocity;
    if (powerUp.position.y + powerUp.size.y >= gameLevel.map.height)
    {
      i = gameLevel.powerUps.erase(i);
      continue;
    }
    std::optional<Collision> result = checkBoxToBoxCollision(
      gameObjectToAabbCollisionBox(powerUp),
      playerCollisionBox
    );
    if (result.has_value())
    {
      for (PowerUpConfig& config : gameLevel.config.powerUpConfigs)
      {
        if (config.type != powerUp.type)
        {
          continue;
        }
        gameLevel.powerUpEffects.push_back(createPowerUpEffect(config));
      }
      i = gameLevel.powerUps.erase(i);
      continue;
    }
    else
    {
      ++i;
    }
  }
}

void handleUpdatePowerUpEffect(GLFWwindow* window, RenderState& renderState, GameLevel& gameLevel)
{
  for (auto i = gameLevel.powerUpEffects.begin(); i != gameLevel.powerUpEffects.end();)
  {
    PowerUpEffect& powerUpEffect = *i;
    if (powerUpEffect.status == POWER_UP_EFFECT_STATUS_DEACTIVATED)
    {
      i = gameLevel.powerUpEffects.erase(i);
    }
    else
    {
      powerUpEffect.ttl = std::max(powerUpEffect.ttl - renderState.deltaTime, 0.0f);
      if (powerUpEffect.status == POWER_UP_EFFECT_STATUS_ACTIVATED && powerUpEffect.ttl <= 0)
      {
        powerUpEffect.status = POWER_UP_EFFECT_STATUS_DEACTIVATE;
      }
      else if (powerUpEffect.status == POWER_UP_EFFECT_STATUS_ACTIVATE)
      {
        activatePowerUp(powerUpEffect, window, renderState, gameLevel);
        powerUpEffect.status = POWER_UP_EFFECT_STATUS_ACTIVATED;
      }
      else if (powerUpEffect.status == POWER_UP_EFFECT_STATUS_DEACTIVATE)
      {
        deactivatePowerUp(powerUpEffect, window, renderState, gameLevel);
        powerUpEffect.status = POWER_UP_EFFECT_STATUS_DEACTIVATED;
      }
      ++i;
    }
  }
}

void handleShakeEffect(GLFWwindow* window, RenderState& renderState, GameLevel& gameLevel)
{
  gameLevel.postProcessor.shake = gameLevel.shakeEffect.ttl > 0.0f;
  gameLevel.shakeEffect.ttl = std::max(gameLevel.shakeEffect.ttl - renderState.deltaTime, 0.0f);
}

void updateGameLevel(GLFWwindow* window, RenderState& renderState, GameLevel& gameLevel)
{
  handlePlayerInput(window, renderState, gameLevel);
  handleBallObjectMovement(window, renderState, gameLevel);
  handleGameLevelCollision(window, renderState, gameLevel);
  handleBallObjectParticles(window, renderState, gameLevel);
  handleUpdatePowerUpObject(window, renderState, gameLevel);
  handleUpdatePowerUpEffect(window, renderState, gameLevel);
  handleShakeEffect(window, renderState, gameLevel);
}

void updateGameState(GLFWwindow* window, RenderState& renderState, GameState& gameState)
{
  if (gameState.status == GAME_ACTIVE)
  {
    GameLevel& gameLevel = gameState.levels[gameState.level];
    updateGameLevel(window, renderState, gameLevel);
  }
};

void updateRenderState(GLFWwindow* window, RenderState& state)
{
  state.time = glfwGetTime();
  state.deltaTime = state.time - state.lastFrame;
  state.lastFrame = state.time;
  glfwGetFramebufferSize(window, &state.bufferWidth, &state.bufferHeight);
  if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
  {
    glfwSetWindowShouldClose(window, GL_TRUE);
  }
}

void handleFrameBufferUpdate(GLFWwindow* window, int width, int height)
{
  glViewport(0, 0, width, height);
}

int main()
{
  std::tuple<int, int> glVersion = {3, 3};
  WindowSettings windowSettings =
  {
    .width = 800,
    .height = 600,
    .title = {WINDOW_TITLE}
  };
  std::filesystem::path staticFilePath = {STATIC_FILE_PATH};
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, std::get<0>(glVersion));
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, std::get<1>(glVersion));
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  GLFWwindow* window = glfwCreateWindow(windowSettings.width, windowSettings.height, windowSettings.title.c_str(), NULL, NULL);
  if(window == NULL)
  {
    glfwTerminate();
    std::cout << "Unable to create window" << std::endl;
    return EXIT_FAILURE;
  }
  glfwMakeContextCurrent(window);
  int version = gladLoadGL(glfwGetProcAddress);
  if(version == -1)
  {
    glfwTerminate();
    std::cout << "Unable to link OpenGL" << std::endl;
    return EXIT_FAILURE;
  }
  std::vector<unsigned int> spriteShaders =
  {
    loadShader(staticFilePath / "sprite.vert", GL_VERTEX_SHADER),
    loadShader(staticFilePath / "sprite.frag", GL_FRAGMENT_SHADER)
  };
  unsigned int spriteShaderProgram = createShaderProgram(spriteShaders);
  std::vector<unsigned int> particleShaders =
  {
    loadShader(staticFilePath / "particle.vert", GL_VERTEX_SHADER),
    loadShader(staticFilePath / "particle.frag", GL_FRAGMENT_SHADER)
  };
  unsigned int particleShaderProgram = createShaderProgram(particleShaders);
  std::vector<unsigned int> postProcessorShaders =
  {
    loadShader(staticFilePath / "post_processor.vert", GL_VERTEX_SHADER),
    loadShader(staticFilePath / "post_processor.frag", GL_FRAGMENT_SHADER)
  };
  unsigned int postProcessorShaderProgram = createShaderProgram(postProcessorShaders);
  Sprite awesomeFaceSprite = createSprite(loadTexture(staticFilePath / "resources/awesomeface.png", false));
  Sprite blockSolidSprite = createSprite(loadTexture(staticFilePath / "resources/block.png", false));
  Sprite blockDestroyableSprite = createSprite(loadTexture(staticFilePath / "resources/block_solid.png", false));
  Sprite backgroundSprite = createSprite(loadTexture(staticFilePath / "resources/background.jpg", false));
  Sprite paddleSprite = createSprite(loadTexture(staticFilePath / "resources/paddle.png", false));
  Sprite powerUpChaos = createSprite(loadTexture(staticFilePath / "resources/powerup_chaos.png", false));
  Sprite powerUpConfusion = createSprite(loadTexture(staticFilePath / "resources/powerup_confuse.png", false));
  Sprite powerUpPaddleSizeUp = createSprite(loadTexture(staticFilePath / "resources/powerup_increase.png", false));
  Sprite powerUpPassThrough = createSprite(loadTexture(staticFilePath / "resources/powerup_passthrough.png", false));
  Sprite powerUpSpeed = createSprite(loadTexture(staticFilePath / "resources/powerup_speed.png", false));
  Sprite powerUpSticky = createSprite(loadTexture(staticFilePath / "resources/powerup_sticky.png", false));
  Particle ballParticle = createParticle(loadTexture(staticFilePath / "resources/particle.png", false));
  RenderState renderState =
  {
    .bufferWidth = 0,
    .bufferHeight = 0,
  };
  updateRenderState(window, renderState);
  PlayerConfig playerConfig =
  {
    .velocity = 500.0f,
    .size = glm::vec2(100.0f, 20.0f),
    .color = glm::vec3(1.0f),
    .sprite = paddleSprite
  };
  BallConfig ballConfig =
  {
    .speed = 400.0f,
    .radius = 12.5f,
    .color = glm::vec3(1.0f),
    .sprite = awesomeFaceSprite,
    .particleCount = 500,
    .particleModel = ballParticle
  };
  ShakeEffectConfig shakeEffectConfig =
  {
    .duration = 0.05f,
  };
  std::vector<PowerUpConfig> powerUpConfigs =
  {
    PowerUpConfig
    {
      .type = POWER_UP_SPEED,
      .ttl = 10.0f,
      .chance = 2,
      .sprite = powerUpSpeed,
      .velocity = glm::vec2(0.0f, 120.0f),
      .size = glm::vec2(20.f),
      .color = glm::vec3(1.0f),
    },
    PowerUpConfig
    {
      .type = POWER_UP_STICKY,
      .ttl = 10.0f,
      .chance = 2,
      .sprite = powerUpSticky,
      .velocity = glm::vec2(0.0f, 60.0f),
      .size = glm::vec2(20.f),
      .color = glm::vec3(1.0f),
    },
    PowerUpConfig
    {
      .type = POWER_UP_PASS_THROUGH,
      .ttl = 10.0f,
      .chance = 2,
      .sprite = powerUpPassThrough,
      .velocity = glm::vec2(0.0f, 200.0f),
      .size = glm::vec2(20.f),
      .color = glm::vec3(1.0f),
    },
    PowerUpConfig
    {
      .type = POWER_UP_PADDLE_SIZE_UP,
      .ttl = 10.0f,
      .chance = 2,
      .sprite = powerUpPaddleSizeUp,
      .velocity = glm::vec2(0.0f, 140.0f),
      .size = glm::vec2(20.f),
      .color = glm::vec3(1.0f),
    },
    PowerUpConfig
    {
      .type = POWER_UP_CHAOS,
      .ttl = 5.0f,
      .chance = 8,
      .sprite = powerUpChaos,
      .velocity = glm::vec2(0.0f, 50.0f),
      .size = glm::vec2(20.f),
      .color = glm::vec3(1.0f),
    },
    PowerUpConfig
    {
      .type = POWER_UP_CONFUSION,
      .ttl = 5.0f,
      .chance = 8,
      .sprite = powerUpConfusion,
      .velocity = glm::vec2(0.0f, 100.0f),
      .size = glm::vec2(20.f),
      .color = glm::vec3(1.0f),
    },
  };
  std::vector<GameLevelConfig> gameLevelConfigs =
  {
    GameLevelConfig
    {
      .tileMap = createTileMap(loadTileData(staticFilePath / "resources/levels/1.txt")),
      .width = (unsigned int)windowSettings.width,
      .height = (unsigned int)windowSettings.height,
      .background = backgroundSprite,
      .blockSolid = blockSolidSprite,
      .blockDestroyable = blockDestroyableSprite,
      .playerConfig = playerConfig,
      .ballConfig = ballConfig,
      .shakeEffectConfig = shakeEffectConfig,
      .powerUpConfigs = powerUpConfigs,
      .spriteShader = spriteShaderProgram,
      .particleShader = particleShaderProgram,
      .postProcessorShader = postProcessorShaderProgram
    },
    GameLevelConfig
    {
      .tileMap = createTileMap(loadTileData(staticFilePath / "resources/levels/2.txt")),
      .width = (unsigned int)windowSettings.width,
      .height = (unsigned int)windowSettings.height,
      .background = backgroundSprite,
      .blockSolid = blockSolidSprite,
      .blockDestroyable = blockDestroyableSprite,
      .playerConfig = playerConfig,
      .ballConfig = ballConfig,
      .shakeEffectConfig = shakeEffectConfig,
      .powerUpConfigs = powerUpConfigs,
      .spriteShader = spriteShaderProgram,
      .particleShader = particleShaderProgram,
      .postProcessorShader = postProcessorShaderProgram
    },
    GameLevelConfig
    {
      .tileMap = createTileMap(loadTileData(staticFilePath / "resources/levels/3.txt")),
      .width = (unsigned int)windowSettings.width,
      .height = (unsigned int)windowSettings.height,
      .background = backgroundSprite,
      .blockSolid = blockSolidSprite,
      .blockDestroyable = blockDestroyableSprite,
      .playerConfig = playerConfig,
      .ballConfig = ballConfig,
      .shakeEffectConfig = shakeEffectConfig,
      .powerUpConfigs = powerUpConfigs,
      .spriteShader = spriteShaderProgram,
      .particleShader = particleShaderProgram,
      .postProcessorShader = postProcessorShaderProgram
    },
    GameLevelConfig
    {
      .tileMap = createTileMap(loadTileData(staticFilePath / "resources/levels/4.txt")),
      .width = (unsigned int)windowSettings.width,
      .height = (unsigned int)windowSettings.height,
      .background = backgroundSprite,
      .blockSolid = blockSolidSprite,
      .blockDestroyable = blockDestroyableSprite,
      .playerConfig = playerConfig,
      .ballConfig = ballConfig,
      .shakeEffectConfig = shakeEffectConfig,
      .powerUpConfigs = powerUpConfigs,
      .spriteShader = spriteShaderProgram,
      .particleShader = particleShaderProgram,
      .postProcessorShader = postProcessorShaderProgram
    },
  };
  std::vector<GameLevel> gameLevels = {};
  for (GameLevelConfig& gameLevelConfig: gameLevelConfigs)
  {
    gameLevels.push_back(createGameLevel(renderState, gameLevelConfig));
  }
  GameState gameState =
  {
    .status = GAME_ACTIVE,
    .levels = gameLevels,
    .level = 0
  };
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glfwSetFramebufferSizeCallback(window, handleFrameBufferUpdate);
  while(!glfwWindowShouldClose(window))
  {
    updateRenderState(window, renderState);
    updateGameState(window, renderState, gameState);
    drawGameState(renderState, gameState);
    glfwSwapBuffers(window);
    glfwPollEvents();
  }
  return EXIT_SUCCESS;
};
