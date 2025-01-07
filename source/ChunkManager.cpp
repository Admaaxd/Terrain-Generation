#include "ChunkManager.h"

ChunkManager::ChunkManager() {
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFrequency(0.023f);
}

ChunkManager::~ChunkManager() {
    cleanupAllChunks();
}

Chunk ChunkManager::generateChunk(int16_t cx, int16_t cz) {
    Chunk chunk;
    chunk.cx = cx;
    chunk.cz = cz;

    GLfloat originX = cx * CHUNK_SIZE * CELL_SIZE;
    GLfloat originZ = cz * CHUNK_SIZE * CELL_SIZE;

    std::vector<GLfloat> vertices;
    std::vector<GLuint> indices;

    for (int16_t z = 0; z <= CHUNK_SIZE; ++z) {
        for (int16_t x = 0; x <= CHUNK_SIZE; ++x) {
            GLfloat wx = originX + x * CELL_SIZE;
            GLfloat wz = originZ + z * CELL_SIZE;

            GLfloat n = noise.GetNoise(wx, wz);
            GLfloat hy = n * HEIGHT_SCALE;

            vertices.push_back(wx);
            vertices.push_back(hy);
            vertices.push_back(wz);
        }
    }

    int16_t vertsPerSide = CHUNK_SIZE + 1;
    for (int16_t z = 0; z < CHUNK_SIZE; ++z) {
        for (int16_t x = 0; x < CHUNK_SIZE; ++x) {
            int16_t topLeft = z * vertsPerSide + x;
            int16_t topRight = topLeft + 1;
            int16_t bottomLeft = (z + 1) * vertsPerSide + x;
            int16_t bottomRight = bottomLeft + 1;

            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }
    chunk.indexCount = (GLsizei)indices.size();

    glGenVertexArrays(1, &chunk.VAO);
    glGenBuffers(1, &chunk.VBO);
    glGenBuffers(1, &chunk.EBO);

    glBindVertexArray(chunk.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, chunk.VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, chunk.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return chunk;
}

void ChunkManager::deleteChunk(Chunk& chunk) {
    glDeleteVertexArrays(1, &chunk.VAO);
    glDeleteBuffers(1, &chunk.VBO);
    glDeleteBuffers(1, &chunk.EBO);
}

void ChunkManager::unloadFarChunks(const glm::vec3& cameraPosition, int16_t camChunkX, int16_t camChunkZ, uint8_t renderDistance) {
    for (auto it = chunks.begin(); it != chunks.end(); ) {
        int16_t chunkX = it->second.cx;
        int16_t chunkZ = it->second.cz;

        if (abs(chunkX - camChunkX) > renderDistance || abs(chunkZ - camChunkZ) > renderDistance) {
            deleteChunk(it->second);
            it = chunks.erase(it);
        }
        else {
            ++it;
        }
    }
}

void ChunkManager::cleanupAllChunks() {
    for (auto& entry : chunks) {
        deleteChunk(entry.second);
    }
    chunks.clear();
}

void ChunkManager::renderChunks(const glm::mat4& gVP, const glm::vec3& cameraPosition, uint8_t renderDistance) {
    int16_t camChunkX = (int16_t)floor(cameraPosition.x / (CHUNK_SIZE * CELL_SIZE));
    int16_t camChunkZ = (int16_t)floor(cameraPosition.z / (CHUNK_SIZE * CELL_SIZE));

    for (int16_t cz = camChunkZ - renderDistance; cz <= camChunkZ + renderDistance; ++cz) {
        for (int16_t cx = camChunkX - renderDistance; cx <= camChunkX + renderDistance; ++cx) {
            long long key = chunkKey(cx, cz);

            if (chunks.find(key) == chunks.end()) {
                chunks[key] = generateChunk(cx, cz);
            }

            Chunk& chunk = chunks[key];
            glBindVertexArray(chunk.VAO);
            glDrawElements(GL_TRIANGLES, chunk.indexCount, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }
    }
}