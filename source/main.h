#pragma once

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <gtx/string_cast.hpp>
#include <windows.h>
#include <iostream>
#include <psapi.h>
#include <unordered_map>

#include "shader.h"
#include "Camera.h"
#include "FastNoiseLite.h"

struct Chunk {
	int cx, cz;
	GLuint VAO, VBO, EBO;
	GLsizei indexCount;
};

class main {
public:
	static void processRendering(GLFWwindow* window, shader& mainShader);

	static void initializeGLFW(GLFWwindow*& window);
	static void initializeGLAD();
	static void framebuffer_size_callback(GLFWwindow* window, GLint width, GLint height);
	static void processInput(GLFWwindow* window);

	static void mouse_callback(GLFWwindow* window, GLdouble xposIn, GLdouble yposIn);

	static void setupRenderingState();

	static void initializeImGui(GLFWwindow* window);
	static void renderImGui(GLFWwindow* window, const glm::vec3& playerPosition);
	static void cleanupImGui();

	static void updateFPS();

	static void cleanup(shader& mainShader);

	static Chunk generateChunk(int cx, int cz);
	static void deleteChunk(Chunk& chunk);
	static void unloadFarChunks(const glm::vec3& cameraPosition, int8_t camChunkX, int8_t camChunkZ, int8_t renderDistance);
	static void cleanupAllChunks();
};