#include <vulkan/vulkan.h>

#include <stdio.h>
#include <iostream>
#include <GLFW/glfw3.h>
#include <vector>
/*
Sources:
huge credits go to the totorial from: https://vulkan-tutorial.com/
Vulkan samples totorial from: https://vulkan.lunarg.com/doc/view/1.2.154.1/windows/tutorial/html/index.html 
GLFW vulkan guide: https://www.glfw.org/docs/3.3/vulkan_guide.html
GLFW getiing started: https://www.glfw.org/docs/3.3/quick.html
c Multiline makros: https://www.geeksforgeeks.org/multiline-macros-in-c/?ref=lbp

*/

#define CHECK_VK_ERROR(f) {\
	VkResult result = f;\
	if (result != VK_SUCCESS){ \
		throw std::runtime_error("failed to create instance!"); \
	}\
}

int main() {
	
	if (!glfwInit())
	{
		// Initialization failed
	}
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	

	GLFWwindow* window = glfwCreateWindow(640, 480, "My Title", NULL, NULL);

	VkInstance instance = {};
	
	VkApplicationInfo vulkanAppInfo = {};
	vulkanAppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	vulkanAppInfo.pApplicationName = "PacManWithVulkan";
	vulkanAppInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	vulkanAppInfo.pEngineName = "No Engine";
	vulkanAppInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	vulkanAppInfo.apiVersion = VK_API_VERSION_1_1;


	VkInstanceCreateInfo instanceCreateInfo = {};
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pNext = NULL;
	instanceCreateInfo.flags = 0;
	instanceCreateInfo.pApplicationInfo = &vulkanAppInfo;

	//Extensions
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
	instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	instanceCreateInfo.ppEnabledExtensionNames = extensions.data();
	
	//We no longer need to specifyi that we need to use the validation layers
	//Just use the external vulkan configurator exe provided by Vulkan SDK
	instanceCreateInfo.enabledLayerCount = 0;
	instanceCreateInfo.ppEnabledLayerNames = nullptr;

	CHECK_VK_ERROR(vkCreateInstance(&instanceCreateInfo, nullptr, &instance));

	//Picking physical Device
	//WARNING: this way of picking a gpu may not work on other machines
	//TODO: let the user pick their GPU at the start of the application?
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	uint32_t physicalDeviceCount = 0;
	CHECK_VK_ERROR(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, NULL));
	std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
	CHECK_VK_ERROR(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data()));
	for (VkPhysicalDevice device : physicalDevices) {
			VkPhysicalDeviceProperties deviceProperties;
			VkPhysicalDeviceFeatures deviceFeatures;
			vkGetPhysicalDeviceProperties(device, &deviceProperties);
			vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
			if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU){
				physicalDevice = device;
			}
	}




	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
	}

	vkDestroyInstance(instance, nullptr);

	glfwDestroyWindow(window);

	glfwTerminate();

	return 0;
}



