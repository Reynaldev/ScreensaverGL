#include <iostream>
#include <cmath>
#include <thread>
#include <fstream>
#include <sstream>
#include <vector>
#include <thread>
#include <mutex>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <Windows.h>
#include <ShObjIdl.h>

#define GLSL_VERSION	"#version 330 core"

typedef std::string String;

void frameBufferCallback(GLFWwindow *window, int width, int height);

struct Shader
{
private:
	static constexpr int VERT_LENGTH = 20;

	float verts[VERT_LENGTH];
	int indices[6] = {
		0, 2, 3,
		0, 1, 2
	};

	// Buffers
	GLuint VAO, VBO, EBO;

	// Box properties
	GLuint programId;

	// Texture properties
	GLuint texture = 0;
	int txWidth, txHeight, txChannels;

public:
	ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

	void create(float size, ImVec4 color)
	{
		this->color = color;

		float newVerts[VERT_LENGTH] = {
			// Pos				// Texture coordinate
			-size,  size, 0.0f,	0.0f, 1.0f,
			 size,  size, 0.0f,	1.0f, 1.0f,
			 size, -size, 0.0f,	1.0f, 0.0f,
			-size, -size, 0.0f,	0.0f, 0.0f
		};

		for (int i = 0; i < VERT_LENGTH; i++)
			verts[i] = newVerts[i];

		glGenBuffers(1, &VBO);
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &EBO);
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
		glBindVertexArray(VAO);

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(this->verts), this->verts, GL_DYNAMIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(this->indices), this->indices, GL_DYNAMIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (GLvoid *)0);
		glEnableVertexAttribArray(0);

		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (GLvoid *)(3 * sizeof(float)));
		glEnableVertexAttribArray(1);
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

		stbi_image_free(data);
	}

	void deleteTexture()
	{
		glDeleteTextures(1, &texture);

		glUniform1i(glGetUniformLocation(this->programId, "uUseTexture"), false);

		texture = NULL;
	}

	void use()
	{
		glUseProgram(this->programId);
	}

	void draw()
	{
		this->use();

		if (hasTexture())
		{
			glUniform1i(glGetUniformLocation(this->programId, "uUseTexture"), true);

			glBindTexture(GL_TEXTURE_2D, this->texture);
		}

		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, sizeof(this->indices) / sizeof(int), GL_UNSIGNED_INT, 0);

		glUniform4f(glGetUniformLocation(getProgramID(), "uColor"), color.x, color.y, color.z, color.w);
	}

	GLuint getVAO(GLuint id)
	{
		return this->VAO;
	}

	GLuint getVBO(GLuint id)
	{
		return this->VBO;
	}

	GLuint getEBO(GLuint id)
	{
		return this->EBO;
	}

	GLuint getProgramID()
	{
		return this->programId;
	}

	GLuint getTexture()
	{
		return this->texture;
	}

	int getTextureWidth()
	{
		return this->txWidth;
	}

	int getTextureHeight()
	{
		return this->txHeight;
	}

	bool hasTexture()
	{
		return (texture != 0) ? true : false;
	}
};

struct Box : public Shader
{
	glm::mat4 model = glm::mat4(1.0f);

	glm::vec3 pos = glm::vec3(0.0f);

	Box(const char *vf, const char *ff, float size, ImVec4 color)
	{
		create(size, color);
		createShader(vf, ff);
		createBufferData();
	}
};

struct
{
	ImVec4 backgroundColor = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);

	int width = 1280, height = 720;
	const char *title = "ScreenSaver GL";

	bool showDemoWindow			= false;
	bool showAppOptions			= false;
	bool showBoxConfig			= false;
	bool showTextureModalChange	= false;
	bool showTextureModalDelete = false;

	String filePath;

	// Modal
	String modalName;

	//Create a modal, use showModal() to start showing the modal and endModal() to en the modal.
	void beginModal(const char *name)
	{
		this->modalName = name;

		ImGui::OpenPopup(modalName.c_str());
		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	}

	// Call beginModal() before using this function.
	bool showModal() const { return ImGui::BeginPopupModal(modalName.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize); }

	// A function to end the already created modal.
	void endModal()
	{
		modalName.clear();
		ImGui::EndPopup();
	}

	// A simple modal to use. For advanced modal, use beginModal.
	void simpleModal(const char *modalName, void (*modalUi)())
	{
		this->modalName = modalName;

		ImGui::OpenPopup(this->modalName.c_str());
		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		if (ImGui::BeginPopupModal(modalName, NULL, ImGuiWindowFlags_AlwaysAutoResize))
		{
			modalUi();

			endModal();
		}
	}

	void showOpenFileDialog(String *out)
	{
		std::mutex mutex;

		HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
		if (SUCCEEDED(hr))
		{
			IFileOpenDialog *fileOpenDialog;

			// Create the FileOpenDialog
			hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
				IID_IFileOpenDialog, reinterpret_cast<void **>(&fileOpenDialog));
			if (SUCCEEDED(hr))
			{
				// Show the Open File Dialog
				hr = fileOpenDialog->Show(NULL);

				// Get the file name from the dialog box
				if (SUCCEEDED(hr))
				{
					IShellItem *shItem;

					hr = fileOpenDialog->GetResult(&shItem);
					if (SUCCEEDED(hr))
					{
						PWSTR filePath;
						hr = shItem->GetDisplayName(SIGDN_FILESYSPATH, &filePath);

						// Display the file name to the user
						if (SUCCEEDED(hr))
						{
							mutex.lock();

							std::wstring wFilePath(filePath);
							*out = String(wFilePath.begin(), wFilePath.end());

							mutex.unlock();

							CoTaskMemFree(filePath);
						}

						shItem->Release();
					}
				}

				fileOpenDialog->Release();
			}

			CoUninitialize();
		}
	}
} App;

void frameBufferCallback(GLFWwindow *window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void changeTexture_concurrent()
{
	App.showOpenFileDialog(&App.filePath);

	if (!App.filePath.empty())
		App.showTextureModalChange = true;
}

int main()
{
	// OpenGL init
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, GLFW_VERSION_MAJOR);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, GLFW_VERSION_MINOR);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow *window = glfwCreateWindow(App.width, App.height, App.title, NULL, NULL);
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

	glViewport(0, 0, App.width, App.height);
	glfwSetFramebufferSizeCallback(window, frameBufferCallback);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO(); (void)io;

	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(GLSL_VERSION);

	// Create box
	Box box("T1_Shader.vert", "T1_Shader.frag", 0.2f, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

	float lastTime = 0;
	while (!glfwWindowShouldClose(window))
	{
		float currTime = (float)glfwGetTime();

		float deltaTime = currTime - lastTime;

		lastTime = currTime;

		float boxSpeed = 0.25f * deltaTime;

		if (GLFW_PRESS == glfwGetKey(window, GLFW_KEY_UP))
			box.pos += glm::vec3(0.0f, 1.0f, 0.0f) * boxSpeed;

		if (GLFW_PRESS == glfwGetKey(window, GLFW_KEY_DOWN))
			box.pos += glm::vec3(0.0f, -1.0f, 0.0f) * boxSpeed;

		if (GLFW_PRESS == glfwGetKey(window, GLFW_KEY_LEFT))
			box.pos += glm::vec3(-1.0f, 0.0f, 0.0f) * boxSpeed;

		if (GLFW_PRESS == glfwGetKey(window, GLFW_KEY_RIGHT))
			box.pos += glm::vec3(1.0f, 0.0f, 0.0f) * boxSpeed;

		glClearColor(
			App.backgroundColor.x,
			App.backgroundColor.y,
			App.backgroundColor.z,
			App.backgroundColor.w
		);
		glClear(GL_COLOR_BUFFER_BIT);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				ImGui::MenuItem("Settings", NULL, &App.showAppOptions);

				ImGui::Separator();

				if (ImGui::MenuItem("Exit"))
					break;

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Tools"))
			{
				ImGui::MenuItem("Config", NULL, &App.showBoxConfig);

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Help"))
			{
#ifdef _DEBUG
				ImGui::MenuItem("Demo window", NULL, &App.showDemoWindow);
#endif // _DEBUG
				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}

		if (App.showDemoWindow)
			ImGui::ShowDemoWindow(&App.showDemoWindow);

		if (App.showAppOptions)
		{
			ImGui::Begin("Settings", &App.showAppOptions);

			ImGui::SeparatorText("Window");

			ImGui::ColorPicker4("Background Color", (float *)&App.backgroundColor);

			ImGui::End();
		}

		if (App.showBoxConfig)
		{
			ImGui::Begin("Box Config", &App.showBoxConfig);

			if (ImGui::CollapsingHeader("Graphics"))
			{
				ImGui::SeparatorText("Color");

				static ImGuiColorEditFlags colorPickerFlag;
				colorPickerFlag |= ImGuiColorEditFlags_NoLabel;
				colorPickerFlag |= ImGuiColorEditFlags_AlphaBar;
				colorPickerFlag |= ImGuiColorEditFlags_AlphaPreview;
				colorPickerFlag |= ImGuiColorEditFlags_NoSidePreview;
				colorPickerFlag |= ImGuiColorEditFlags_NoSmallPreview;

				ImGui::ColorPicker4("Box Color", (float *)&box.color, colorPickerFlag);

				ImGui::SeparatorText("Texture");

				if (box.hasTexture())
				{
					ImGui::Image((ImTextureID)box.getTexture(), ImVec2(64, 64), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
					ImGui::SameLine();
				}

				ImGui::BeginGroup();
				{
					if (ImGui::Button((box.hasTexture()) ? "Change" : "Add"))
					{
						std::thread tr(changeTexture_concurrent);
						tr.detach();
					}
					ImGui::SetItemTooltip("Add/Change the texture of the box");

					if (box.hasTexture())
					{
						if (ImGui::Button("Delete"))
							App.showTextureModalDelete = true;

						ImGui::SetItemTooltip("Delete the texture of the box");
					}
				}
				ImGui::EndGroup();
			}

			if (ImGui::CollapsingHeader("Position"))
			{
				ImGui::Text("X:\t%.2f | Y:\t%.2f", box.pos.x, box.pos.y);
			}

			ImGui::End();
		}

		if (App.showTextureModalChange)
		{
			App.beginModal("Change texture");
			if (App.showModal())
			{
				GLuint wrapper[] = { GL_REPEAT, GL_MIRRORED_REPEAT, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER };
				GLuint filters[] = { GL_NEAREST, GL_LINEAR, GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR_MIPMAP_LINEAR };
				GLuint fmts[] = { GL_RGB, GL_RGBA };

				static int wrapperCurrent = 0;
				static int filterCurrent = 0;
				static int formatCurrent = 0;

				ImGui::Combo("Wrapper", &wrapperCurrent, "Repeat\0Mirrored-Repeat\0Clamp to edge\0Clamp to border");
				ImGui::Combo("Filter", &filterCurrent, "Nearest\0Linear\0Linear - Mipmap Nearest\0Linear - Mipmap Linear");
				ImGui::Combo("Format", &formatCurrent, "RGB\0RGBA");

				if (ImGui::Button("OK", ImVec2(120, 0)))
				{
					box.createTexture(
						App.filePath.c_str(),
						wrapper[wrapperCurrent],
						wrapper[wrapperCurrent],
						filters[filterCurrent],
						filters[filterCurrent],
						fmts[formatCurrent]);
					App.showTextureModalChange = false;
					App.filePath.clear();
				}

				ImGui::SameLine();

				if (ImGui::Button("Cancel", ImVec2(120, 0)))
				{
					App.showTextureModalChange = false;
					App.filePath.clear();
				}

				App.endModal();
			}
		}

		if (App.showTextureModalDelete)
		{
			App.beginModal("Delete texture?");
			if (App.showModal())
			{
				ImGui::Text("Do you want to delete the texture?");

				if (ImGui::Button("OK", ImVec2(120, 0)))
				{
					box.deleteTexture();
					App.showTextureModalDelete = false;
				}

				ImGui::SameLine();

				if (ImGui::Button("Cancel", ImVec2(120, 0)))
				{
					App.showTextureModalDelete = false;
				}
			}
		}

		// Update projection
		//App.proj = glm::ortho(0.0f, (float)App.width, 0.0f, (float)App.height, 0.1f, 100.0f);
		//App.proj = glm::perspective(glm::radians(75.0f), (float)(App.width / App.height), 0.1f, 100.0f);

		// Reset the box if it goes outside the frame
		if (
			box.pos.x >  1.0f ||
			box.pos.x < -1.0f ||
			box.pos.y >  1.0f ||
			box.pos.y < -1.0f
		) {
			box.pos = glm::vec3(0.0f);
		}

		// Draw box
		static glm::vec3 direction(0.5f, 0.5f, 0.0f);

		if (box.pos.x > 0.8f)
			direction.x *= -1.0f;

		if (box.pos.x < -0.8f)
			direction.x *= -1.0f;

		if (box.pos.y > 0.75f)
			direction.y *= -1.0f;

		if (box.pos.y < -0.8f)
			direction.y *= -1.0f;

		box.pos += direction * boxSpeed;
		box.model = glm::translate(box.model, box.pos);

		box.draw();

		glUniformMatrix4fv(
			glGetUniformLocation(box.getProgramID(), "uModel"),
			1,
			GL_FALSE,
			glm::value_ptr(box.model)
		);

		// Reset to matrix identity
		box.model = glm::mat4(1.0f);

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