#include "main.h"

#pragma region Global Variables
constexpr GLuint SCR_WIDTH = 1280;
constexpr GLuint SCR_HEIGHT = 720;

GLFWmonitor* primaryMonitor;
GLint windowedPosX, windowedPosY;
GLint windowedWidth = SCR_WIDTH, windowedHeight = SCR_HEIGHT;

GLfloat lastX = SCR_WIDTH / 2.0f;
GLfloat lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

Camera camera;

bool isGUIEnabled = false;

GLdouble lastTime = glfwGetTime();
uint8_t nbFrames = 0;
GLfloat fps = 0;

std::unordered_map<long long, Chunk> g_Chunks;

const int8_t CHUNK_SIZE = 100;
const float CELL_SIZE = 1.0f;
#pragma endregion

inline long long chunkKey(int cx, int cz) {
	return (static_cast<long long>(cx) << 32) ^ (cz & 0xffffffff);
}

int main()
{
	GLFWwindow* window;
	main::initializeGLFW(window);
	main::initializeGLAD();

	glfwSetFramebufferSizeCallback(window, main::framebuffer_size_callback);
	glfwSetCursorPosCallback(window, main::mouse_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	shader mainShader("main.vs", "main.fs");

	main::initializeImGui(window);
	main::setupRenderingState();

	while (!glfwWindowShouldClose(window))
	{
		main::updateFPS();

		main::processInput(window);

		main::processRendering(window, mainShader);

		main::renderImGui(window, camera.getPosition());

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	main::cleanup(mainShader);
	main::cleanupImGui();

	main::cleanupAllChunks();
	glfwTerminate();
	return 0;

}

void main::processRendering(GLFWwindow* window, shader& mainShader)
{
	glm::mat4 view = camera.getViewMatrix();
	glm::mat4 projection = glm::perspective(glm::radians(75.0f), (GLfloat)(SCR_WIDTH / (GLfloat)SCR_HEIGHT), 0.1f, 1000.0f);
	glm::mat4 gVP = projection * view;

	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	mainShader.use();
	
	mainShader.setMat4("gVP", gVP);

	glm::vec3 cameraWorldPos = camera.getPosition();

	const float CELL_SIZE = 1.0f;
	int8_t camChunkX = (int8_t)floor(cameraWorldPos.x / (CHUNK_SIZE * CELL_SIZE));
	int8_t camChunkZ = (int8_t)floor(cameraWorldPos.z / (CHUNK_SIZE * CELL_SIZE));

	int8_t R = 2;
	for (int8_t cz = camChunkZ - R; cz <= camChunkZ + R; cz++) {
		for (int8_t cx = camChunkX - R; cx <= camChunkX + R; cx++) {
			long long key = chunkKey(cx, cz);

			if (g_Chunks.find(key) == g_Chunks.end()) {
				Chunk newChunk = generateChunk(cx, cz);
				g_Chunks[key] = newChunk;
			}
		}
	}

	for (int8_t cz = camChunkZ - R; cz <= camChunkZ + R; cz++) {
		for (int8_t cx = camChunkX - R; cx <= camChunkX + R; cx++) {
			long long key = chunkKey(cx, cz);
			auto it = g_Chunks.find(key);
			if (it != g_Chunks.end()) {
				Chunk& chunk = it->second;

				glBindVertexArray(chunk.VAO);
				glDrawElements(GL_TRIANGLES, chunk.indexCount, GL_UNSIGNED_INT, 0);
				glBindVertexArray(0);
			}
		}
	}
	main::unloadFarChunks(camera.getPosition(), camChunkX, camChunkZ, 2);
}

Chunk main::generateChunk(int cx, int cz) {
	Chunk chunk;
	chunk.cx = cx;
	chunk.cz = cz;

	static FastNoiseLite noise;
	noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
	noise.SetFrequency(0.023f);

	const float HEIGHT_SCALE = 10.0f;

	float originX = cx * CHUNK_SIZE * CELL_SIZE;
	float originZ = cz * CHUNK_SIZE * CELL_SIZE;

	std::vector<GLfloat> vertices;
	std::vector<GLuint>  indices;

	for (int8_t z = 0; z <= CHUNK_SIZE; ++z) {
		for (int8_t x = 0; x <= CHUNK_SIZE; ++x) {
			float wx = originX + x * CELL_SIZE;
			float wz = originZ + z * CELL_SIZE;

			float n = noise.GetNoise(wx, wz);
			float hy = n * HEIGHT_SCALE;

			vertices.push_back(wx);  // X
			vertices.push_back(hy);  // Y
			vertices.push_back(wz);  // Z
		}
	}

	int8_t vertsPerSide = CHUNK_SIZE + 1;
	for (int8_t z = 0; z < CHUNK_SIZE; ++z) {
		for (int8_t x = 0; x < CHUNK_SIZE; ++x) {
			int topLeft = z * vertsPerSide + x;
			int topRight = topLeft + 1;
			int bottomLeft = (z + 1) * vertsPerSide + x;
			int bottomRight = bottomLeft + 1;

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

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	return chunk;
}

void main::deleteChunk(Chunk& chunk) {
	glDeleteVertexArrays(1, &chunk.VAO);
	glDeleteBuffers(1, &chunk.VBO);
	glDeleteBuffers(1, &chunk.EBO);
}

void main::unloadFarChunks(const glm::vec3& cameraPosition, int8_t camChunkX, int8_t camChunkZ, int8_t renderDistance) {
	for (auto it = g_Chunks.begin(); it != g_Chunks.end(); ) {
		int chunkX = it->second.cx;
		int chunkZ = it->second.cz;

		if (abs(chunkX - camChunkX) > renderDistance || abs(chunkZ - camChunkZ) > renderDistance) {
			main::deleteChunk(it->second);
			it = g_Chunks.erase(it);
		}
		else {
			++it;
		}
	}
}

void main::cleanupAllChunks() {
	for (auto& entry : g_Chunks) {
		deleteChunk(entry.second);
	}
	g_Chunks.clear();
}

void main::initializeGLFW(GLFWwindow*& window)
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	primaryMonitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* videoMode = glfwGetVideoMode(primaryMonitor);

	window = glfwCreateWindow(windowedWidth, windowedHeight, "Terrain", nullptr, nullptr);

	if (!window)
	{
		std::cerr << "Failed to create GLFW window!" << std::endl;
		glfwTerminate();
		exit(-1);
	}

	glfwMakeContextCurrent(window);

	glfwSetFramebufferSizeCallback(window, [](GLFWwindow*, GLint width, GLint height) {
		glViewport(0, 0, width, height);
	});
}

void main::initializeGLAD()
{
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cerr << "Failed to initialize GLAD!" << std::endl;
		glfwTerminate();
		exit(-1);
	}
}

void main::framebuffer_size_callback(GLFWwindow* window, GLint width, GLint height)
{
	glViewport(0, 0, width, height);
}

void main::processInput(GLFWwindow* window)
{
	static bool lastEscState = false;
	bool currentEscState = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;

	if (currentEscState && !lastEscState) {
		isGUIEnabled = !isGUIEnabled;

		if (isGUIEnabled) {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
		else {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		}
		if (!isGUIEnabled) {
			GLdouble xpos, ypos;
			glfwGetCursorPos(window, &xpos, &ypos);
			lastX = static_cast<GLfloat>(xpos);
			lastY = static_cast<GLfloat>(ypos);
		}
	}
	lastEscState = currentEscState;

	if (!isGUIEnabled) {
		camera.setMovementState(Direction::FORWARD, glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS);
		camera.setMovementState(Direction::BACKWARD, glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS);
		camera.setMovementState(Direction::LEFT, glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS);
		camera.setMovementState(Direction::RIGHT, glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS);
		camera.setMovementState(Direction::UP, glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS);
		camera.setMovementState(Direction::DOWN, glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS);

		camera.update(deltaTime);
	}
}

void main::mouse_callback(GLFWwindow* window, GLdouble xposIn, GLdouble yposIn)
{
	if (isGUIEnabled) {
		return;
	}

	GLfloat xpos = static_cast<float>(xposIn);
	GLfloat ypos = static_cast<float>(yposIn);

	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	GLfloat xoffset = xpos - lastX;
	GLfloat yoffset = lastY - ypos;

	lastX = xpos;
	lastY = ypos;

	GLfloat sensitivity = 0.1f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	camera.updateCameraOrientation(camera.getYaw() + xoffset, camera.getPitch() + yoffset);
}

void main::setupRenderingState()
{
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void main::initializeImGui(GLFWwindow* window)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");
	ImGui::StyleColorsDark();
}

void main::renderImGui(GLFWwindow* window, const glm::vec3& playerPosition)
{
	if (!isGUIEnabled) return;

	glDisable(GL_DEPTH_TEST);
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Menu");

	ImGui::Text("FPS: %.1f", fps); // FPS counter

	ImGui::Text("Player Position: (%.2f, %.2f, %.2f)", playerPosition.x, playerPosition.y, playerPosition.z);

	if (ImGui::Button("Exit Game")) glfwSetWindowShouldClose(window, true);

	ImGui::End();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	glEnable(GL_DEPTH_TEST);
}

void main::cleanupImGui()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void main::updateFPS()
{
	GLfloat currentTime = static_cast<GLfloat>(glfwGetTime());
	nbFrames++;
	if (currentTime - lastTime >= 1.0) {
		fps = nbFrames;
		nbFrames = 0;
		lastTime += 1.0;
	}
	deltaTime = currentTime - lastFrame;
	lastFrame = currentTime;
}

void main::cleanup(shader& mainShader)
{
	mainShader.Delete();
}
