#include "main.h"

#include <fstream>
#include <sstream>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define GLSL_VERSION	"#version 330 core"

#define WIN_WIDTH		800
#define WIN_HEIGHT		600
#define WIN_TITLE		"Screensaver GL"

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

struct 
{
private:
	// Box properties
	unsigned int programId, bufferID;
	float verts[32];
	int indices[6];

	// Texture properties
	GLuint texture = 0;
	int txWidth, txHeight, txChannels;

public:
	void create(int id, ImVec4 pts[4], ImVec4 col[4], ImVec4 tex[4], int ind[6])
	{
		this->bufferID = id;

		for (int i = 0; i < 4; i++)
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

		for (int i = 0; i < (sizeof(this->indices) / sizeof(int)); i++)
			this->indices[i] = ind[i];
	}

	void createShader(const char *vertFile, const char *fragFile)
	{
		String vertCode, fragCode;
		std::ifstream vertShaderFile, fragShaderFile;

		vertShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		fragShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

		try
		{
			vertShaderFile.open(vertFile);
			fragShaderFile.open(fragFile);

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
			return;
		}

		const char *vertShader = vertCode.c_str();
		const char *fragShader = fragCode.c_str();

		GLuint vertex, fragment;
		int success;
		char infoLog[512];

		vertex = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertex, 1, &vertShader, NULL);
		glCompileShader(vertex);

		glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(vertex, 512, NULL, infoLog);
			printf("%s\n", infoLog);
			return;
		}

		fragment = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragment, 1, &fragShader, NULL);
		glCompileShader(fragment);

		glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(fragment, 512, NULL, infoLog);
			printf("%s\n", infoLog);
			return;
		}

		this->programId = glCreateProgram();
		glAttachShader(this->programId, vertex);
		glAttachShader(this->programId, fragment);
		glLinkProgram(this->programId);

		glGetProgramiv(this->programId, GL_LINK_STATUS, &success);
		if (!success)
		{
			glGetProgramInfoLog(this->programId, 512, NULL, infoLog);
			printf("Program link failed\n%s\n", infoLog);
			return;
		}

		glDeleteShader(vertex);
		glDeleteShader(fragment);
	}

	void createBufferData()
	{
		glBindVertexArray(GLBuffer.getVAO(this->bufferID));

		glBindBuffer(GL_ARRAY_BUFFER, GLBuffer.getVBO(this->bufferID));
		glBufferData(GL_ARRAY_BUFFER, sizeof(this->verts), this->verts, GL_DYNAMIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GLBuffer.getEBO(this->bufferID));
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(this->indices), this->indices, GL_DYNAMIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (GLvoid *)0);
		glEnableVertexAttribArray(0);

		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (GLvoid *)(3 * sizeof(float)));
		glEnableVertexAttribArray(1);
		
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (GLvoid *)(6 * sizeof(float)));
		glEnableVertexAttribArray(2);
	}

	void createTexture(const char *txFile, GLint wrapS, GLint wrapT, GLint filterMin, GLint filterMag, GLenum fmt = GL_RGB)
	{
		glGenTextures(1, &this->texture);
		glBindTexture(GL_TEXTURE_2D, texture);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filterMin);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filterMag);

		stbi_set_flip_vertically_on_load(true);

		unsigned char *data = stbi_load(txFile, &this->txWidth, &this->txHeight, &this->txChannels, 0);
		if (!data)
		{
			printf("Failed to load texture file\nFile: %s\n", txFile);
			return;
		}

		glTexImage2D(GL_TEXTURE_2D, 0, fmt, this->txWidth, this->txHeight, 0, fmt, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		this->use();
		glUniform1i(glGetUniformLocation(this->programId, "uUseTexture"), true);

		stbi_image_free(data);
	}

	void use()
	{
		glUseProgram(this->programId);
	}

	void draw()
	{
		this->use();

		if (this->texture != 0)
		{
			glBindTexture(GL_TEXTURE_2D, this->texture);
		}

		glBindVertexArray(GLBuffer.getVAO(this->bufferID));
		glDrawElements(GL_TRIANGLES, sizeof(this->indices) / sizeof(int), GL_UNSIGNED_INT, 0);
	}

	GLuint getProgramID()
	{
		return this->programId;
	}

	GLuint getBufferID()
	{
		return this->bufferID;
	}
} Box;

struct
{
	bool showDemoWindow = false;
} ScreenSaverGLWindow;

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

	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(GLSL_VERSION);
	// OpenGL init

	// Create box
	ImVec4 boxPos[] = {
		ImVec4(-0.25f,  0.25f, 0.0f, 0.0f),	// Top-left
		ImVec4( 0.25f,  0.25f, 0.0f, 0.0f),	// Top-right
		ImVec4( 0.25f, -0.25f, 0.0f, 0.0f),	// Bottom-right
		ImVec4(-0.25f, -0.25f, 0.0f, 0.0f)	// Bottom-left
	};

	ImVec4 boxColor[] = {
		ImVec4(1.0f, 0.0f, 0.0f, 0.0f),
		ImVec4(0.0f, 1.0f, 0.0f, 0.0f),
		ImVec4(0.0f, 0.0f, 1.0f, 0.0f),
		ImVec4(0.0f, 1.0f, 0.0f, 0.0f)
	};

	ImVec4 boxTexCoords[] = {
		ImVec4(0.0f, 1.0f, 0.0f, 0.0f),		// Top-left
		ImVec4(1.0f, 1.0f, 0.0f, 0.0f),		// Top-right
		ImVec4(1.0f, 0.0f, 0.0f, 0.0f),		// Bottom-right
		ImVec4(0.0f, 0.0f, 0.0f, 0.0f)		// Bottom-left
	};

	int boxIndices[] = {
		0, 2, 3,
		0, 1, 2
	};

	Box.create(GLBuffer.createID(), boxPos, boxColor, boxTexCoords, boxIndices);
	Box.createShader("T1_Shader.vert", "T1_Shader.frag");
	Box.createTexture("dalle.png", GL_REPEAT, GL_REPEAT, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST, GL_RGB);
	// Create box

	GLBuffer.init();
	Box.createBufferData();

	while (!glfwWindowShouldClose(window))
	{
		glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Exit"))
					break;

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Help"))
			{
				ImGui::MenuItem("Demo window", NULL, &ScreenSaverGLWindow.showDemoWindow);

				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}

		if (ScreenSaverGLWindow.showDemoWindow)
			ImGui::ShowDemoWindow();

		Box.draw();

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