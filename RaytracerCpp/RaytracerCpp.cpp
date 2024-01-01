#include "general.h"
#include "hittable.h"
#include "hittablelist.h"
#include "sphere.h"
#include "camera.h"
#include "material.h"
#include "bvh.h"
#include "quad.h"
#include "instance.h"
#include "triangle.h"
#include "objimporter.h"

#include "glad/gl.h"
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"


#include"external/OpenImageDenoise/oidn.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "external/stb_image_write.h"



#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>


void RenderWorld(camera& cam, hittable_list& world, float*& pixels, int& sample);
void NormalScene( hittable_list& world,  camera& cam);
void NormalScene2(hittable_list& world, camera& cam);
void cornell_box(hittable_list& world, camera& cam);
void Triangle(hittable_list& world, camera& cam);
void denoise(const camera& cam, float*& pixels);
void UpdateTexture(const camera& cam, float*& pixels);

std::string getCurrentDateTimeFilename(std::string extension) {
	auto now = std::chrono::system_clock::now();
	auto time_t_now = std::chrono::system_clock::to_time_t(now);

	// Convert to tm struct
	std::tm tm_now;
	localtime_s(&tm_now, &time_t_now); // Use localtime_s for thread safety

	std::ostringstream filename;
	filename << std::put_time(&tm_now, "%Y-%m-%d_%H-%M-%S"); // Format: YYYY-MM-DD_HH-MM-SS

	return filename.str() + extension; // or any other extension
}

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
static bool bvh_world = true;
int main()
{
	//camera setup
	camera cam;
	cam.aspect_ratio = 16.0 / 9.0;
	cam.image_width = 800;
	cam.samples_per_pixel = 50;
	cam.max_depth = 5;

	cam.lookfrom = point3(0,1,2);
	cam.lookat = point3(0, 0, -1);
	cam.vup = point3(0, 1, 0);
	cam.vertical_fov = 90;
	cam.defocus_angle = 0;
	cam.focus_dist = 1;
	cam.background = make_shared<image_texture>("photo.jpg");
	cam.threadsize = 20;

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
	

	Triangle(world,  cam);
	//NormalScene2(world,  cam);
	//cornell_box(world,  cam);
	world_bvh = hittable_list(make_shared<bvh_node>(world));
	

	//texture init
	GLuint image_texture;
	glGenTextures(1, &image_texture);
	glBindTexture(GL_TEXTURE_2D, image_texture);

	// Setup filtering parameters for display
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 

	float* buffer = nullptr;
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
		static int sample = 0;
		

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

		if (ImGui::Button("Render")) {			
			sample = 0;
			glfwSetWindowAspectRatio(window, cam.aspect_ratio * 100, 100);
			RenderWorld(cam, bvh_world?world_bvh: world,buffer,sample);
		}
		else if (continious && sample < cam.samples_per_pixel && sample>0)
		{
			RenderWorld(cam, bvh_world ? world_bvh : world, buffer, sample);
		}
		if (sample > 0)
		{
			ImGui::SameLine();
			ImGui::Text(" %i / %i", sample, cam.samples_per_pixel);
		}
		
		ImGui::Checkbox("Multithreading", &cam.multithreading);
		if (cam.multithreading)
		{
			ImGui::SameLine();
			ImGui::PushItemWidth(100);
			ImGui::InputInt("Thread Count", &cam.threadsize);
			ImGui::PopItemWidth();			
			ImGui::Checkbox("tiledthreading", &cam.tiledthreading);
			if (cam.tiledthreading)
			{
				ImGui::SameLine();
				ImGui::PushItemWidth(100);
				ImGui::InputInt("tile Count", &cam.tilesize);
				ImGui::PopItemWidth();
			}
		}	
		ImGui::Checkbox("BVH?", &bvh_world);
		ImGui::SameLine();
		ImGui::Checkbox("Realtime", &continious);
		if (buffer != nullptr)
		{
			if (ImGui::Button("Denoise")) {
				denoise(cam, buffer);
			}
			ImGui::SameLine();
			if (ImGui::Button("Save")) {

				unsigned char* data = new unsigned char[cam.image_width * cam.image_height * 3];

				for (int i = 0;i < cam.image_width * cam.image_height * 3;i++)
				{
					data[i] = static_cast<unsigned char>(std::round(buffer[i] * 255.0));
				}
				std::string filename = getCurrentDateTimeFilename(".png");
				stbi_write_png(filename.c_str(), cam.image_width, cam.image_height, 3, data, cam.image_width * 3);

				delete[] data;
			}		
			
		}


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

void denoise(const camera& cam, float*& pixels)
{
	int width = cam.image_width;
	int height = cam.image_height;

	oidn::DeviceRef device = oidn::newDevice(); // CPU or GPU if available
	device.commit();
	oidn::BufferRef colorBuf = device.newBuffer(width * height * 3 * sizeof(float));
	oidn::FilterRef filter = device.newFilter("RT"); // generic ray tracing filter
	filter.setImage("color", colorBuf, oidn::Format::Float3, width, height); // beauty
	filter.setImage("output", colorBuf, oidn::Format::Float3, width, height);
	filter.commit();
	float* colorPtr = (float*)colorBuf.getData();
	for (int i = 0; i < width * height * 3; i++)
	{
		colorPtr[i] = pixels[i];
	}
	filter.execute();

	const char* errorMessage;
	if (device.getError(errorMessage) != oidn::Error::None)
		std::cout << "Error: " << errorMessage << std::endl;
	else
	{
		for (int i = 0; i < width * height * 3; i++)
		{
			pixels[i] = colorPtr[i];
		}
		UpdateTexture(cam, pixels);
	}

}

void UpdateTexture(const camera& cam, float*& pixels)
{
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, cam.image_width, cam.image_height, 0, GL_RGB, GL_FLOAT, pixels);
}

void RenderWorld(camera& cam, hittable_list& world, float*& pixels,int& sample)
{	
	int starttime = glfwGetTime();
	cam.seedMultiplier = glfwGetTime();
	cam.render(world);
	lasttime = glfwGetTime() - starttime;

	if (sample == 0)
	{
		if (pixels != nullptr)
			free(pixels);
		pixels = (float*)malloc((cam.image_width * cam.image_height) * 3 * sizeof(float));
	}

	
	int c = 0;

	for (int i=0;i<cam.image_width*cam.image_height;i++)
	{
		auto& pixel = cam.pixelarray[i];		
		pixels[c] = ((pixels[c] * sample) + (float)(pixel[0])) / (sample + 1);
		pixels[c + 1] = ((pixels[c + 1] * sample) + (float)(pixel[1])) / (sample + 1);
		pixels[c + 2] = ((pixels[c + 2] * sample) + (float)(pixel[2])) / (sample + 1);
		
		c += 3;
		
	}

	sample++;

	UpdateTexture(cam, pixels);
	
}

void NormalScene(hittable_list& world, camera& cam)
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
	
}

void NormalScene2(hittable_list& world, camera& cam)
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
	

	cam.lookfrom = point3(1, 2, 3);
	cam.lookat = point3(0, 2, -1);
	cam.samples_per_pixel = 100;
	cam.max_depth = 20;
	
}

void Triangle(hittable_list& world, camera& cam) {
	//auto mat_red = make_shared<lambertian>(make_shared<image_texture>("photobaba.jpg"));
	
	/*auto v0 = make_shared<vertex>();
	auto v1 = make_shared<vertex>();
	auto v2 = make_shared<vertex>();
	v0->position = vec3(0, 0, 0);
	v0->u = 0;
	v0->v = 0;

	v1->position = vec3(0, 1, 0);
	v1->u = 0;
	v1->v = 1;

	v2->position = vec3(1, 0, 0);
	v2->u = 1;
	v2->v = 0;

	auto tri = make_shared<triangle>(v0,v1,v2, mat_gold);*/
	//world.add(tri);
	auto mat_gold = make_shared<metal>(color(0.8, 0.6, 0.2), 0.2);
	auto mat_ground = make_shared<lambertian>(color(0.8, 0.8, 0.0));
	//auto three_model = make_shared<bvh_node>(*LoadMesh("bidu.obj", mat_gold));	
	auto whitelight = make_shared<diffuse_light>(color(15, 15, 15));

	auto metildamat = make_shared<lambertian>(make_shared<image_texture>("matilda.jpg"));
	auto matilda = make_shared<bvh_node>(*LoadMesh("matilda.obj", metildamat));
	auto light = make_shared<quad>(vec3(0, 0, 0), vec3(0, 1, 0), vec3(0, 0, 0.5), whitelight);
	
	world.add( make_shared<instance>(light,vec3(-1,0.5,0),vec3(0, pi / 4,0)));
	
	
	world.add(make_shared<instance>(matilda, vec3(0, -0.5, 0), vec3(0, 0, 0)));
	
	/*world.add(make_shared<instance>(matilda, vec3(-1, -0.5, 0), vec3(0, pi / 2, 0)));
	world.add(make_shared<instance>(matilda, vec3(1, -0.5, -1), vec3(0, pi / 2, 0)));
	world.add(make_shared<instance>(matilda, vec3(0, -0.5, 1), vec3(0, 0, 0)));*/
	/*world.add(three_model);
	world.add( make_shared<instance>(three_model,vec3(1,0,1),vec3(0,pi/2,0)));
	world.add(make_shared<instance>(three_model, vec3(-1, 0, 1), vec3(0, -pi / 2, 0)));*/
	
	world.add(make_shared<quad>(vec3(-100, -0.5, -100), vec3(0, 0, 200), vec3(200, 0, 0), mat_ground));
	//cam.background = make_shared<solid_color>(vec3(0.5, 0.5, 0.5));
	//cam.background = make_shared<solid_color>(0.01,0.01,0.05);
	cam.lookat=vec3(0, 1.2, 0);
	cam.vertical_fov = 50;
	cam.image_width = 1920;

}

void cornell_box(hittable_list& world, camera& cam) {
	

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

	
}