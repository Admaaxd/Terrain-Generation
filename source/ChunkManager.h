#ifndef CHUNK_H
#define CHUNK_H

#include <unordered_map>
#include <vector>
#include <glm.hpp>
#include "FastNoiseLite.h"
#include <glad/glad.h>

struct Chunk {
    int16_t cx, cz;
    GLuint VAO, VBO, EBO;
    GLsizei indexCount;
};

constexpr int8_t CHUNK_SIZE = 90;
const float CELL_SIZE = 1.0f;
const float HEIGHT_SCALE = 10.0f;

class ChunkManager {
public:
    ChunkManager();
    ~ChunkManager();

    Chunk generateChunk(int16_t cx, int16_t cz);
    void deleteChunk(Chunk& chunk);
    void unloadFarChunks(const glm::vec3& cameraPosition, int16_t camChunkX, int16_t camChunkZ, uint8_t renderDistance);
    void cleanupAllChunks();
    void renderChunks(const glm::mat4& gVP, const glm::vec3& cameraPosition, uint8_t renderDistance);

private:
    std::unordered_map<long long, Chunk> chunks;
    FastNoiseLite noise;

    inline long long chunkKey(int16_t cx, int16_t cz) const {
        return (static_cast<long long>(cx) << 32) ^ (cz & 0xffffffff);
    }
};

#endif // CHUNK_H