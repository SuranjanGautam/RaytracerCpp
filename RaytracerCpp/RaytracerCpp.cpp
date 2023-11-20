#include "general.h"
#include "hittable.h"
#include "hittablelist.h"
#include "sphere.h"
#include "camera.h"
#include "material.h"

#include "glad/gl.h"
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <iostream>
#include <thread>


void RenderWorld(camera& cam, hittable_list& world);

//screenspace quad
const float vertices[] = {
	 1,  1, 0.0f,  1.0f, 0.0f,// top right
	 1, -1, 0.0f,  1.0f, 1.0f,// bottom right
	-1, -1, 0.0f,  0.0f, 1.0f,// bottom left
	-1,  1, 0.0f,   0.0f, 0.0f // top left 
};
const unsigned int indices[] = {  // note that we start from 0!
	0, 1, 3,   // first triangle
	1, 2, 3    // second triangle
};

const char* vertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"layout(location = 1) in vec2 aTexCoord;\n"
"out vec2 TexCoord;\n"
"void main()\n"
"{\n"
"   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
"	TexCoord = aTexCoord;\n"
"}\0";

const char* fragmentShaderSource = "#version 330 core\n"
"out vec4 FragColor;\n"
"in vec2 TexCoord;\n"
"\n"
"uniform sampler2D FrameTexture;\n"
"void main()\n"
"{\n"
"    FragColor = texture(FrameTexture, TexCoord);\n"
"    //FragColor = vec4(1,0.5,1,1);\n"
"    FragColor.a = 1;\n"
"} ";

static double lasttime = 0;

int main()
{
	//camera setup
	camera cam;
	cam.aspect_ratio = 16.0 / 9.0;
	cam.image_width = 800;
	cam.samples_per_pixel = 50;
	cam.max_depth = 10;

	cam.lookfrom = point3(-2,2,1);
	cam.lookat = point3(0, 0, -1);
	cam.vup = point3(0, 1, 0);
	cam.vertical_fov = 20;
	cam.defocus_angle = 10;
	cam.focus_dist = 3.4;


	
	//GLFW
	if (!glfwInit()) {
		// Initialization failed
		return -1;
	}

	const char* glsl_version = "#version 130";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	cam.initialize();

	GLFWwindow* window = glfwCreateWindow(cam.image_width, cam.image_height, "Raytracer", NULL, NULL);
	if (!window) {
		// Window or OpenGL context creation failed
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	gladLoadGL(glfwGetProcAddress);

	//shaders	
#pragma region shaders
	
	unsigned int vertexShader, fragmentShader;
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);

	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);

	unsigned int shaderProgram;
	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	glUseProgram(shaderProgram);

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	//setting up vertex
	unsigned int VAO;
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	unsigned int VBO;
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	unsigned int EBO;
	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
#pragma endregion	

	// Setup Dear ImGui context
	
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);
	

	//material setup
	auto mat_ground = make_shared<lambertian>(color(0.8, 0.8, 0.0));
	auto mat_red = make_shared<lambertian>(color(0.2, 0.2, 0.8));
	auto mat_glass = make_shared<dielectric>(1.5);
	auto mat_chrome = make_shared<metal>(color(0.8, 0.8, 0.8), 0.1);
	auto mat_gold = make_shared<metal>(color(0.8, 0.6, 0.2), 0.2);

	//world setup
	hittable_list world;
	world.add(make_shared<sphere>(color(1, 0, -1), 0.5, mat_gold));
	world.add(make_shared<sphere>(color(0, 0, -1), 0.5, mat_red));
	world.add(make_shared<sphere>(color(-1, 0, -1), 0.5, mat_glass));
	world.add(make_shared<sphere>(color(-1, 0, -1), -0.4, mat_glass));
	world.add(make_shared<sphere>(color(0, -100.5, -1), 100, mat_ground));	


	//texture init
	GLuint image_texture;
	glGenTextures(1, &image_texture);
	glBindTexture(GL_TEXTURE_2D, image_texture);

	// Setup filtering parameters for display
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same
	
	while (!glfwWindowShouldClose(window))
	{	
		glfwPollEvents();
		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		static float f = 0.0f;
		static int counter = 0;
		static bool continious = false;

		ImGui::Begin("Render Settings");	// Create a window called "Hello, world!" and append into it.
		ImGui::Text("Raytracing Settings");	// Display some text (you can use a format strings too)

		ImGui::InputInt("Samples Per Pixel", &cam.samples_per_pixel);            // Edit 1 float using a slider from 0.0f to 1.0f
		ImGui::InputInt("Bounches", &cam.max_depth);

		ImGui::Text("Camera Settings");
		ImGui::InputDouble("Aspect ratio", &cam.aspect_ratio);
		ImGui::InputInt("Camera Width", &cam.image_width);
		ImGui::InputInt("vertical FOV", &cam.vertical_fov);
		ImGui::InputDouble("focus distance", &cam.focus_dist);
		ImGui::InputDouble("defocus angle", &cam.defocus_angle);
		
		ImGui::Text("Camera Position");
		ImGui::PushItemWidth(100);
		ImGui::InputDouble("Cx", &cam.lookfrom[0]);ImGui::SameLine();
		ImGui::InputDouble("Cy", &cam.lookfrom[1]);ImGui::SameLine();
		ImGui::InputDouble("Cz", &cam.lookfrom[2]);

		ImGui::Text("Look at");
		ImGui::InputDouble("Lx", &cam.lookat[0]);ImGui::SameLine();
		ImGui::InputDouble("Ly", &cam.lookat[1]);ImGui::SameLine();
		ImGui::InputDouble("Lz", &cam.lookat[2]);
		ImGui::PopItemWidth();

		if (ImGui::Button("Render")||continious) {	
			RenderWorld(cam, world);
		}
		ImGui::SameLine();
		ImGui::Checkbox("Multithreading", &cam.multithreading);
		ImGui::Checkbox("Realtime", &continious);

		ImGui::Text("Last render time %.3f seconds", (float)lasttime);
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
		ImGui::End();
		

		// Rendering
		ImGui::Render();
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(0,0,0,1);
		glClear(GL_COLOR_BUFFER_BIT);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, image_texture);
		glUseProgram(shaderProgram);
		glUniform1i(glGetUniformLocation(shaderProgram, "FrameTexture"), 0);
		glBindVertexArray(VAO);		
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
	}	

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;	
}

void RenderWorld(camera& cam, hittable_list& world)
{
	int starttime = glfwGetTime();
	cam.seedMultiplier = glfwGetTime();
	cam.render(world);

	float* pixels = (float*)malloc((cam.image_width * cam.image_height) * 3 * sizeof(float));

	int c = 0;

	//make file
	//std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";
	for (int i=0;i<cam.image_width*cam.image_height;i++)
	{
		auto& pixel = cam.pixelarray[i];
		//write_color(std::cout, pixel);
		pixels[c] = (float)(pixel[0]);
		pixels[c + 1] = (float)(pixel[1]);
		pixels[c + 2] = (float)(pixel[2]);
		c += 3;
	}

	// Upload pixels into texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, cam.image_width, cam.image_height, 0, GL_RGB, GL_FLOAT, pixels);
	free(pixels);
	lasttime = glfwGetTime() - starttime;
}
