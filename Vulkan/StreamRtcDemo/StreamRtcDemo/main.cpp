//#define GLFW_INCLUDE_VULKAN
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "cstream_rtc_c.h"

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 500;

static void input_device_callback(struct CInputEvent* e)
{
	printf("type = %u\n", ((struct CInputEvent*)(e))->Event);
}

int main(int argc, char *argv)
{
	//int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
	//{
	//
	//	chen::g_demo.connection = hInstance;
	//	strncpy(chen::g_demo.name, "Vulkan Cube", chen::APP_NAME_STR_LEN);
	//	return 0;
		// glfw: 初始化
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
	// uncomment this statement to fix compilation on OS X
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
	// glfw 创建窗口
	GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Test<GLFW+GLAD>", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	c_init();
	c_set_input_device_callback(&input_device_callback);
	c_startup("192.168.1.175", 9000, "chensong", "chensong");
	glfwMakeContextCurrent(window);
	// glad: load all OpenGL function pointers
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}
	 

	//////////////////////////////////////
	gl_shared_init(SCR_WIDTH, SCR_HEIGHT);
	// render loop
	while (!glfwWindowShouldClose(window))
	{
		static bool load = false;

		if (!load)
		{
			glClearColor(0.0f, 1.f, 0.0f, 1.0f);

		}
		else
		{
			glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
		}
		load = !load;
		glClear(GL_COLOR_BUFFER_BIT);

		// RENDER
		gl_shared_capture(SCR_WIDTH, SCR_HEIGHT);
		glfwSwapBuffers(window);

		glfwPollEvents();


		Sleep(1);
	}
	// glfw: terminate, clearing all previously allocated GLFWresources.
	glfwTerminate();
	return 0;
}