#include <iostream>
#include <GLFW/glfw3.h>
int main() {
	std::cout << "hello world!";
	if (!glfwInit())
	{
		// Initialization failed
	}
	GLFWwindow* window = glfwCreateWindow(640, 480, "My Title", NULL, NULL);

	while (!glfwWindowShouldClose(window))
	{
		
		glfwPollEvents();
	}
	return 0;
}