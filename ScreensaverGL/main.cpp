#include "main.h"

#include <fstream>
#include <sstream>

#define GLSL_VERSION	"#version 330 core"

#define WIN_WIDTH	800
#define WIN_HEIGHT	600
#define WIN_TITLE	"Screensaver GL"

struct 
{
private:
	GLuint VAO[2], VBO[2], EBO[2];
	unsigned int id = 0;

public:
	void init()
	{
		glGenBuffers(2, VBO);
		glGenVertexArrays(2, VAO);
		glGenBuffers(2, EBO);
	}

	int createID()
	{
		int retId = this->id++;
		return retId;
	}

	GLuint getVAO(GLuint id)
	{
		return this->VAO[id];
	}

	GLuint getVBO(GLuint id)
	{
		return this->VBO[id];
	}

	GLuint getEBO(GLuint id)
	{
		return this->EBO[id];
	}
} GLBuffer;

struct Triangle 
{
private:
	unsigned int id, bufferID;
	float verts[24];

public:
	Triangle(int id, ImVec4 pts[3], ImVec4 col[3], ImVec4 tex[3])
	{
		this->bufferID = id;

		for (int i = 0; i < 3; i++)
		{
			// Row
			int r = (i * 8);

			// Points position
			verts[r + 0] = pts[i].x;
			verts[r + 1] = pts[i].y;
			verts[r + 2] = pts[i].z;

			// Colors
			verts[r + 3] = col[i].x;
			verts[r + 4] = col[i].y;
			verts[r + 5] = col[i].z;

			// Texture coordinates
			verts[r + 6] = tex[i].x;
			verts[r + 7] = tex[i].y;
		}
	}

	bool createShader(String vertFile, String fragFile)
	{
		String vertCode, fragCode;
		std::ifstream vertShaderFile, fragShaderFile;

		vertShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		fragShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

		try
		{
			vertShaderFile.open(vertFile.c_str());
			fragShaderFile.open(fragFile.c_str());

			std::stringstream vertShaderStream, fragShaderStream;

			vertShaderStream << vertShaderFile.rdbuf();
			fragShaderStream << fragShaderFile.rdbuf();

			vertShaderFile.close();
			fragShaderFile.close();

			vertCode = vertShaderStream.str();
			fragCode = fragShaderStream.str();
		}
		catch (const std::ifstream::failure e)
		{
			printf("Error. Failed to read shader files\n");
			return false;
		}

		const char *vertShaderCode = vertCode.c_str();
		const char *fragShaderCode = fragCode.c_str();

		GLuint vertex, fragment;
		int success;
		char infoLog[512];

		vertex = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertex, 1, &vertShaderCode, NULL);
		glCompileShader(vertex);

		glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(vertex, 512, NULL, infoLog);
			printf("Vertex shader compilation failed\n%s\n", infoLog);
			return false;
		}

		fragment = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragment, 1, &fragShaderCode, NULL);
		glCompileShader(fragment);

		glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(fragment, 512, NULL, infoLog);
			printf("Fragment shader compilation failed\n%s\n", infoLog);
			return false;
		}

		this->id = glCreateProgram();
		glAttachShader(this->id, vertex);
		glAttachShader(this->id, fragment);
		glLinkProgram(this->id);

		glGetProgramiv(this->id, GL_LINK_STATUS, &success);
		if (!success)
		{
			glGetProgramInfoLog(this->id, 512, NULL, infoLog);
			printf("Program link failed\n%s\n", infoLog);
			return false;
		}

		glDeleteShader(vertex);
		glDeleteShader(fragment);

		return true;
	}

	void createBufferData()
	{
		glBindVertexArray(GLBuffer.getVAO(this->bufferID));
		glBindBuffer(GL_ARRAY_BUFFER, GLBuffer.getVBO(this->bufferID));
		glBufferData(GL_ARRAY_BUFFER, sizeof(this->verts), this->verts, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (GLvoid *)0);
		glEnableVertexAttribArray(0);

		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (GLvoid *)(3 * sizeof(float)));
		glEnableVertexAttribArray(1);
		
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (GLvoid *)(6 * sizeof(float)));
		glEnableVertexAttribArray(2);
	}

	void use()
	{
		glUseProgram(this->id);
	}

	void draw()
	{
		this->use();

		glBindVertexArray(GLBuffer.getVAO(this->bufferID));
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}

	GLuint getID()
	{
		return (GLuint)this->id;
	}

	GLuint getBufferID()
	{
		return (GLuint)this->bufferID;
	}
};

void frameBufferCallback(GLFWwindow *window, int width, int height)
{
	glViewport(0, 0, width, height);
}

int main()
{
	// OpenGL init
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, GLFW_VERSION_MAJOR);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, GLFW_VERSION_MINOR);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow *window = glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT, WIN_TITLE, NULL, NULL);
	if (!window)
	{
		printf("Cannot create Window for GLFW!\n");
		return -1;
	}

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		printf("Failed to initialize GLAD\n");
		return -1;
	}

	glViewport(0, 0, WIN_WIDTH, WIN_HEIGHT);
	glfwSetFramebufferSizeCallback(window, frameBufferCallback);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO(); (void)io;

	ImGui::StyleColorsDark;

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(GLSL_VERSION);
	// OpenGL init

	// Create shaders
	ImVec4 t1Pos[] = {
		ImVec4(0.0f, 0.5f, 0.0f, 0.0f),
		ImVec4(0.5f, -0.5f, 0.0f, 0.0f),
		ImVec4(-0.5f, -0.5f, 0.0f, 0.0f)
	};

	ImVec4 t1Col[] = {
		ImVec4(1.0f, 0.0f, 0.0f, 0.0f),
		ImVec4(0.0f, 1.0f, 0.0f, 0.0f),
		ImVec4(0.0f, 0.0f, 1.0f, 0.0f)
	};

	ImVec4 t1TexCoord[] = {
		ImVec4(0.0f, 0.5f, 0.0f, 0.0f),
		ImVec4(0.5f, -0.5f, 0.0f, 0.0f),
		ImVec4(-0.5f, -0.5f, 0.0f, 0.0f)
	};

	Triangle t1(GLBuffer.createID(), t1Pos, t1Col, t1TexCoord);
	t1.createShader("T1_Shader.vert", "T1_Shader.frag");
	// Create shaders

	GLBuffer.init();
	t1.createBufferData();

	while (!glfwWindowShouldClose(window))
	{
		glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		t1.draw();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}