#include "general.h"
#include "hittable.h"
#include "hittablelist.h"
#include "sphere.h"
#include "camera.h"
#include "material.h"
#include "bvh.h"
#include "quad.h"
#include "instance.h"

#include "glad/gl.h"
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <iostream>
#include <thread>


void RenderWorld(camera& cam, hittable_list& world);
void NormalScene( hittable_list& world, hittable_list& world_bvh, camera& cam);
void NormalScene2(hittable_list& world, hittable_list& world_bvh, camera& cam);
void cornell_box(hittable_list& world, hittable_list& world_bvh, camera& cam);
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

	cam.lookfrom = point3(0,0,1);
	cam.lookat = point3(0, 0, -1);
	cam.vup = point3(0, 1, 0);
	cam.vertical_fov = 90;
	cam.defocus_angle = 0;
	cam.focus_dist = 1;
	cam.background = make_shared<image_texture>("hdri.jpg");

	mat3 maty = mat3::identity();
	
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
	
	//world setup
	hittable_list world;
	hittable_list world_bvh;
	bool bvh_world = false;

	NormalScene2(world, world_bvh, cam);
	//cornell_box(world, world_bvh, cam);
	auto point = vec4(0, 1, 0,1);
	auto rotation = mat4::translation(vec3(1, 0, 0));

	point =  point * rotation;

	std::cout << point.x()<< " " << point.y() << " " << point.z()<<"\n";

	//texture init
	GLuint image_texture;
	glGenTextures(1, &image_texture);
	glBindTexture(GL_TEXTURE_2D, image_texture);

	// Setup filtering parameters for display
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
	
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

		ImGui::Begin("Render Settings");
		ImGui::Text("Raytracing Settings");

		ImGui::InputInt("Samples Per Pixel", &cam.samples_per_pixel);
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

		if (ImGui::Button("Render") || continious) {

			glfwSetWindowAspectRatio(window, cam.aspect_ratio * 100, 100);
			RenderWorld(cam, bvh_world?world_bvh: world);
		}
		ImGui::SameLine();
		ImGui::Checkbox("Multithreading", &cam.multithreading);
		if (cam.multithreading)
		{
			ImGui::SameLine();
			ImGui::PushItemWidth(100);
			ImGui::InputInt("Thread Count", &cam.threadsize);
			ImGui::PopItemWidth();
		}	
		ImGui::Checkbox("BVH?", &bvh_world);
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

void NormalScene(hittable_list& world, hittable_list& world_bvh, camera& cam)
{
	//material setup
	auto mat_ground = make_shared<lambertian>(color(0.8, 0.8, 0.0));
	auto mat_red = make_shared<lambertian>(make_shared<image_texture>("photous.jpg"));
	auto mat_glass = make_shared<dielectric>(1.5);
	auto mat_chrome = make_shared<metal>(color(0.8, 0.8, 0.8), 0.1);
	auto mat_gold = make_shared<metal>(color(0.8, 0.6, 0.2), 0.2);
	auto red = make_shared<diffuse_light>(color(0.5, 0, 0));

	cam.background = make_shared<solid_color>(color(0.5, 0.5, 0.5));

	//world.add(make_shared<sphere>(color(1, 0, -1), 0.5, mat_gold));
	world.add(make_shared<sphere>(color(0, 0, -1), 0.5, red));
	/*world.add(make_shared<sphere>(color(-1, 0, -1), 0.5, mat_glass));
	world.add(make_shared<sphere>(color(-1, 0, -1), -0.4, mat_glass));*/
	world.add(make_shared<sphere>(color(0, -100.5, -1), 100, mat_ground));
	//world.add(make_shared<quad>(point3(0, 0, 0), vec3(0, 1, 0), vec3(1, 0, 0), red));

	world_bvh = hittable_list(make_shared<bvh_node>(world));
}

void NormalScene2(hittable_list& world, hittable_list& world_bvh, camera& cam)
{
	//material setup
	auto mat_ground = make_shared<lambertian>(color(0.8, 0.8, 0.0));
	auto mat_red = make_shared<lambertian>(make_shared<image_texture>("photobaba.jpg"));
	auto mat_baba_light = make_shared<diffuse_light>(make_shared<image_texture>("photobaba.jpg"));
	auto mat_glass = make_shared<dielectric>(1.5);
	auto mat_chrome = make_shared<metal>(color(0.8, 0.8, 0.8), 0.1);
	auto mat_gold = make_shared<metal>(color(0.8, 0.6, 0.2), 0.2);
	auto red = make_shared<diffuse_light>(color(0.5, 0, 0));

	cam.background = make_shared<solid_color>(color(0.01, 0.01, 0.01));

	world.add(make_shared<sphere>(vec3(1.5, 0, -1), 0.5, mat_gold));
	auto middle = make_shared<sphere>(vec3(0, 0, 0), 0.5, mat_red);
	auto baba = make_shared<quad>(vec3(-5, 2, -5),  vec3(10, 0, 0), vec3(0, 5, 0), mat_baba_light);

	world.add(baba);
	
	for (int i = 0;i < 100;i++)
	{
		//world.add(make_shared<sphere>(vec3(random_double(-10, 10), 0, random_double(-10, 10)), 0.5, mat_red));
		world.add(make_shared<instance>(middle, vec3(random_double(-10,10),0, random_double(-10, 10)), vec3(0, random_double(0,2*pi), 0)));
	}	
	world.add(make_shared<quad>(vec3(-100, -0.5, -100), vec3(0, 0, 200), vec3(200, 0, 0), mat_ground));
	world_bvh = hittable_list(make_shared<bvh_node>(world));

	cam.lookfrom = point3(1, 2, 3);
	cam.lookat = point3(0, 2, -1);
	cam.samples_per_pixel = 100;
	cam.max_depth = 20;
	
}

void cornell_box(hittable_list& world, hittable_list& world_bvh, camera& cam) {
	

	auto red = make_shared<lambertian>(color(.65, .05, .05));
	auto white = make_shared<lambertian>(color(.73, .73, .73));
	auto green = make_shared<lambertian>(color(.12, .45, .15));
	auto light = make_shared<diffuse_light>(color(15, 15, 15));

	world.add(make_shared<quad>(point3(555, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555), green));
	world.add(make_shared<quad>(point3(0, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555), red));
	world.add(make_shared<quad>(point3(343, 554, 332), vec3(-130, 0, 0), vec3(0, 0, -105), light));
	world.add(make_shared<quad>(point3(0, 0, 0), vec3(555, 0, 0), vec3(0, 0, 555), white));
	world.add(make_shared<quad>(point3(555, 555, 555), vec3(-555, 0, 0), vec3(0, 0, -555), white));
	world.add(make_shared<quad>(point3(0, 0, 555), vec3(555, 0, 0), vec3(0, 555, 0), white));

	cam.aspect_ratio = 1.0;
	cam.image_width = 600;
	cam.samples_per_pixel = 200;
	cam.max_depth = 50;
	cam.background = make_shared<solid_color>(color(0, 0, 0));

	cam.vertical_fov = 40;
	cam.lookfrom = point3(278, 278, -800);
	cam.lookat = point3(278, 278, 0);
	cam.vup = vec3(0, 1, 0);

	cam.defocus_angle = 0;

	world_bvh = hittable_list(make_shared<bvh_node>(world));
}