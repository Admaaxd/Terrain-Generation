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
#pragma endregion

int main()
{
	GLFWwindow* window;
	main::initializeGLFW(window);
	main::initializeGLAD();

	glfwSetFramebufferSizeCallback(window, main::framebuffer_size_callback);
	glfwSetCursorPosCallback(window, main::mouse_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	ChunkManager chunkManager;

	shader mainShader("main.vs", "main.fs");

	main::initializeImGui(window);
	main::setupRenderingState();

	while (!glfwWindowShouldClose(window))
	{
		main::updateFPS();

		main::processInput(window);

		main::processRendering(window, mainShader, chunkManager);

		main::renderImGui(window, camera.getPosition());

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	main::cleanup(mainShader);
	main::cleanupImGui();

	glfwTerminate();
	return 0;
}

void main::processRendering(GLFWwindow* window, shader& mainShader, ChunkManager& chunkManager)
{
	glm::mat4 view = camera.getViewMatrix();
	glm::mat4 projection = glm::perspective(glm::radians(75.0f), (GLfloat)(SCR_WIDTH / (GLfloat)SCR_HEIGHT), 0.1f, 1000.0f);
	glm::mat4 gVP = projection * view;

	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	mainShader.use();
	mainShader.setMat4("gVP", gVP);
	mainShader.setVec3("skycolor", glm::vec3(1.0f, 1.0f, 1.0f));

	glm::vec3 cameraWorldPos = camera.getPosition();

	chunkManager.renderChunks(gVP, cameraWorldPos, 2);
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
