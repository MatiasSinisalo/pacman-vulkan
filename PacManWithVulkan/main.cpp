

#include <stdio.h>
#include <iostream>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>



#include "shader_vert.spv.h"
#include "shader_frag.spv.h"


#include <vector>
#include <algorithm> 
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

VkPipelineShaderStageCreateInfo createShaderStage(VkDevice &logicalDevice, VkShaderStageFlagBits shaderStage, const uint32_t *shaderHeader, const uint32_t shaderHeaderSize, std::vector<VkShaderModule> *shaderModules) {
	
	VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
	shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCreateInfo.pNext = NULL;
	shaderModuleCreateInfo.flags = 0;
	shaderModuleCreateInfo.codeSize = shaderHeaderSize;
	shaderModuleCreateInfo.pCode = shaderHeader;
	VkShaderModule shaderModule = {};
	CHECK_VK_ERROR(vkCreateShaderModule(logicalDevice, &shaderModuleCreateInfo, nullptr, &shaderModule));
	shaderModules->push_back(shaderModule);

	VkPipelineShaderStageCreateInfo shaderStageCreateInfo = {};
	shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageCreateInfo.pNext = NULL;
	shaderStageCreateInfo.flags = 0;
	shaderStageCreateInfo.stage = shaderStage;
	shaderStageCreateInfo.module = shaderModule;
	shaderStageCreateInfo.pName = "main";
	shaderStageCreateInfo.pSpecializationInfo = NULL;

	return shaderStageCreateInfo;
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

	//instance Extensions
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
	instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	instanceCreateInfo.ppEnabledExtensionNames = extensions.data();
	
	//We no longer need to specifyi that we need to use the validation layers
	//Just use the external vulkan configurator exe provided by Vulkan SDK
	//to my understanding layers have been deprecated?
	instanceCreateInfo.enabledLayerCount = 0;
	instanceCreateInfo.ppEnabledLayerNames = nullptr;

	CHECK_VK_ERROR(vkCreateInstance(&instanceCreateInfo, nullptr, &instance));

	//Creating a surface
	VkSurfaceKHR surface = {};
	VkWin32SurfaceCreateInfoKHR win32SurfaceCreateInfo = {};
	win32SurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	win32SurfaceCreateInfo.pNext = NULL;
	win32SurfaceCreateInfo.hwnd = glfwGetWin32Window(window);
	win32SurfaceCreateInfo.hinstance = GetModuleHandle(nullptr);
	vkCreateWin32SurfaceKHR(instance, &win32SurfaceCreateInfo, nullptr, &surface);

	//Picking physical Device
	//WARNING: this way of picking a gpu may not work on other devices
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
	

	//Choosing Queues
	VkQueue graphicsQueue = {};
	uint32_t graphicsQueueIndex = -1;
	VkDeviceQueueCreateInfo graphicsQueueCreateInfo = {};

	VkQueue presentQueue = {};
	uint32_t presentQueueIndex = -1;
	VkDeviceQueueCreateInfo presentQueueCreateInfo = {};

	//WARNING: this way of choosing queues may not work on other devices
	
	uint32_t queueCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilyInformations(queueCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, queueFamilyInformations.data());
	
	//TODO: make sure the queues are seperate, does it matter?
	for (unsigned int i = 0; i < queueCount; i++) {
		if (queueFamilyInformations[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			graphicsQueueIndex = i;
			graphicsQueueCreateInfo.queueFamilyIndex = i;
		}
		VkBool32 thisQueueSupportsPresent = VK_FALSE;
		vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &thisQueueSupportsPresent);
		if (thisQueueSupportsPresent == VK_TRUE){
			presentQueueIndex = i;
			presentQueueCreateInfo.queueFamilyIndex = i;
		}
	}
	float queuePriories[1] = { 1.0f };
	graphicsQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	graphicsQueueCreateInfo.pNext = NULL;
	graphicsQueueCreateInfo.flags = 0;
	graphicsQueueCreateInfo.pQueuePriorities = queuePriories;
	graphicsQueueCreateInfo.queueCount = 1;

	presentQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	presentQueueCreateInfo.pNext = NULL;
	presentQueueCreateInfo.flags = 0;
	presentQueueCreateInfo.pQueuePriorities = queuePriories;
	presentQueueCreateInfo.queueCount = 1;
	
	VkDeviceQueueCreateInfo queueInfos[2] = { graphicsQueueCreateInfo, presentQueueCreateInfo};

	//Creating the logical device
	//Extensions for logical device
	const std::vector<const char*> logicalDeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	
	VkDeviceCreateInfo logicalDeviceCreateInfo = {};
	logicalDeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	logicalDeviceCreateInfo.pNext = NULL;
	logicalDeviceCreateInfo.flags = 0;
	logicalDeviceCreateInfo.queueCreateInfoCount = 2;
	logicalDeviceCreateInfo.pQueueCreateInfos = queueInfos;
	logicalDeviceCreateInfo.enabledLayerCount = 0;
	logicalDeviceCreateInfo.ppEnabledLayerNames = NULL;
	logicalDeviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(logicalDeviceExtensions.size());
	logicalDeviceCreateInfo.ppEnabledExtensionNames = logicalDeviceExtensions.data();
	logicalDeviceCreateInfo.pEnabledFeatures = NULL;

	VkDevice logicalDevice;
	CHECK_VK_ERROR(vkCreateDevice(physicalDevice, &logicalDeviceCreateInfo, NULL, &logicalDevice));
	vkGetDeviceQueue(logicalDevice, graphicsQueueIndex, 0, &graphicsQueue);
	vkGetDeviceQueue(logicalDevice, presentQueueIndex, 0, &presentQueue);
	
	//Create a swapchain
	VkSurfaceCapabilitiesKHR pSurfaceCapabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &pSurfaceCapabilities);
	
	uint32_t surfaceFormatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, nullptr);
	std::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, surfaceFormats.data());

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
	std::vector<VkPresentModeKHR> presentModes(presentModeCount);
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data());


	int swapChainWidth;
	int swapChainHeight;
	glfwGetFramebufferSize(window, &swapChainWidth, &swapChainHeight);
	//WARNING: it may be possible that the width and height exeed the 
	VkExtent2D swapchainDimensions = {};
	swapchainDimensions.width = std::clamp(swapchainDimensions.width, pSurfaceCapabilities.minImageExtent.width, pSurfaceCapabilities.maxImageExtent.width);
	swapchainDimensions.height = std::clamp(swapchainDimensions.height, pSurfaceCapabilities.minImageExtent.height, pSurfaceCapabilities.maxImageExtent.height);


	VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
	swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainCreateInfo.pNext = NULL;
	swapChainCreateInfo.flags = 0;
	swapChainCreateInfo.surface = surface;
	swapChainCreateInfo.minImageCount = pSurfaceCapabilities.minImageCount + 1;
	swapChainCreateInfo.imageFormat = VK_FORMAT_B8G8R8A8_SRGB; //WARNING this is just a shortcut TODO: actually choose what image format we use
	swapChainCreateInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	swapChainCreateInfo.imageExtent = swapchainDimensions;
	swapChainCreateInfo.imageArrayLayers = 1;
	swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	if (graphicsQueueIndex != presentQueueIndex) {
		const uint32_t queueFamilyIndices[2] = { graphicsQueueIndex , presentQueueIndex };
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapChainCreateInfo.queueFamilyIndexCount = 2;
		swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapChainCreateInfo.queueFamilyIndexCount = 0;
		swapChainCreateInfo.pQueueFamilyIndices = NULL;
	}
	swapChainCreateInfo.preTransform = pSurfaceCapabilities.currentTransform;
	swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapChainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR; //We will have two frames in flight
	swapChainCreateInfo.clipped = VK_TRUE;
	swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
	
	VkSwapchainKHR swapChain = {};
	CHECK_VK_ERROR(vkCreateSwapchainKHR(logicalDevice, &swapChainCreateInfo, nullptr, &swapChain));

	//Create the renderpass
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = VK_FORMAT_B8G8R8A8_SRGB;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.pNext = NULL;
	renderPassCreateInfo.flags = 0;
	renderPassCreateInfo.attachmentCount = 1;
	renderPassCreateInfo.pAttachments = &colorAttachment;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpass;
	renderPassCreateInfo.dependencyCount = 0;
	renderPassCreateInfo.pDependencies = NULL;

	VkRenderPass renderPass = {};
	CHECK_VK_ERROR(vkCreateRenderPass(logicalDevice, &renderPassCreateInfo, nullptr, &renderPass));

	//Get the images of the swapchain
	uint32_t swapChainImageCount;
	vkGetSwapchainImagesKHR(logicalDevice, swapChain, &swapChainImageCount, nullptr);
	std::vector<VkImage> swapChainImages(swapChainImageCount);
	vkGetSwapchainImagesKHR(logicalDevice, swapChain, &swapChainImageCount, swapChainImages.data());

	//Create views for the images
	std::vector<VkImageView> swapChainImageViews(swapChainImageCount);
	for (int i = 0; i < swapChainImageCount; i++) {
		VkImageViewCreateInfo imageViewCreateInfo = {};
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.pNext = NULL;
		imageViewCreateInfo.flags = 0;
		imageViewCreateInfo.image = swapChainImages[i];
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.format = VK_FORMAT_B8G8R8A8_SRGB;
		imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.subresourceRange;
		imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 1;

		VkImageView imageView = {};
		CHECK_VK_ERROR(vkCreateImageView(logicalDevice, &imageViewCreateInfo, nullptr, &swapChainImageViews[i]));
	}
	//Create a framebuffer for each image in the swapchain
	std::vector<VkFramebuffer> swapChainFrameBuffers(swapChainImageCount);
	
	for (int i = 0; i < swapChainImageCount; i++) {
		VkFramebufferCreateInfo frameBufferCreateInfo = {};
		frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frameBufferCreateInfo.pNext = NULL;
		frameBufferCreateInfo.flags = 0;
		frameBufferCreateInfo.renderPass = renderPass;
		frameBufferCreateInfo.attachmentCount = 1; //The number of vkimageviews
		frameBufferCreateInfo.pAttachments = &swapChainImageViews[i]; // pointer to an array of the swapchains vkimageviews
		frameBufferCreateInfo.width = 640;
		frameBufferCreateInfo.height = 480;
		frameBufferCreateInfo.layers = 1;

		VkFramebuffer frameBuffer = {};
		CHECK_VK_ERROR(vkCreateFramebuffer(logicalDevice, &frameBufferCreateInfo, nullptr, &swapChainFrameBuffers[i]));
	}

	//command pool
	VkCommandPoolCreateInfo commandPoolCreateInfo;
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.pNext = NULL;
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	commandPoolCreateInfo.queueFamilyIndex = graphicsQueueIndex;

	VkCommandPool commandPool = {};
	CHECK_VK_ERROR(vkCreateCommandPool(logicalDevice, &commandPoolCreateInfo, nullptr, &commandPool));

	//Allocate command buffers from the pool
	VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.pNext = NULL;
	commandBufferAllocateInfo.commandPool = commandPool;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandBufferCount = swapChainImageCount;

	std::vector<VkCommandBuffer> commandBuffers(swapChainImageCount);
	CHECK_VK_ERROR(vkAllocateCommandBuffers(logicalDevice, &commandBufferAllocateInfo, commandBuffers.data()));
	
	//create a graphics pipeline for our triangle

	uint32_t graphicsPipelineStageCount = 2;
	std::vector<VkShaderModule> shaderModules;
	std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateinfos(graphicsPipelineStageCount);
	shaderStageCreateinfos[0] = createShaderStage(logicalDevice, VK_SHADER_STAGE_VERTEX_BIT, shader_vert, sizeof(shader_vert), &shaderModules);
	shaderStageCreateinfos[1] = createShaderStage(logicalDevice, VK_SHADER_STAGE_FRAGMENT_BIT, shader_frag, sizeof(shader_frag), &shaderModules);
	
	//NOTE: we are currently using hardcoded vertex info in our vertex shader
	VkPipelineVertexInputStateCreateInfo vertexInputState = {};
	vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputState.pNext = NULL;
	vertexInputState.flags = 0;
	vertexInputState.vertexBindingDescriptionCount = 0;
	vertexInputState.pVertexBindingDescriptions = NULL;
	vertexInputState.vertexAttributeDescriptionCount = 0;
	vertexInputState.pVertexAttributeDescriptions = NULL;

	VkPipelineInputAssemblyStateCreateInfo assemblyStateCreateInfo = {};
	assemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	assemblyStateCreateInfo.pNext = NULL;
	assemblyStateCreateInfo.flags = 0;
	assemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	assemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewPortStateCreateInfo = {};
	viewPortStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewPortStateCreateInfo.pNext = NULL;
	viewPortStateCreateInfo.flags = 0;
	viewPortStateCreateInfo.viewportCount = 1;
	viewPortStateCreateInfo.pViewports = NULL;
	viewPortStateCreateInfo.scissorCount = 1;
	viewPortStateCreateInfo.pScissors = NULL;

	VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {};
	rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationStateCreateInfo.pNext = NULL;
	rasterizationStateCreateInfo.flags = 0;
	rasterizationStateCreateInfo.depthClampEnable;
	rasterizationStateCreateInfo.rasterizerDiscardEnable;
	rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
	rasterizationStateCreateInfo.depthBiasConstantFactor = 0;
	rasterizationStateCreateInfo.depthBiasClamp = 0;
	rasterizationStateCreateInfo.depthBiasSlopeFactor = 0;
	rasterizationStateCreateInfo.lineWidth = 1.0f;

	VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {};
	multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleStateCreateInfo.pNext = NULL;
	multisampleStateCreateInfo.flags = 0;
	multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
	multisampleStateCreateInfo.minSampleShading = 0.0f;
	multisampleStateCreateInfo.pSampleMask = NULL;
	multisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
	multisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState colorBLendStateAttachmentState = {};
	colorBLendStateAttachmentState.blendEnable = VK_FALSE;
	colorBLendStateAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBLendStateAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBLendStateAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
	colorBLendStateAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBLendStateAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBLendStateAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
	colorBLendStateAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};
	colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendStateCreateInfo.pNext = NULL;
	colorBlendStateCreateInfo.flags = 0;
	colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
	colorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_NO_OP;
	colorBlendStateCreateInfo.attachmentCount = 1;
	colorBlendStateCreateInfo.pAttachments = &colorBLendStateAttachmentState;
	colorBlendStateCreateInfo.blendConstants[0] = 1.0f;
	colorBlendStateCreateInfo.blendConstants[1] = 1.0f;
	colorBlendStateCreateInfo.blendConstants[2] = 1.0f;
	colorBlendStateCreateInfo.blendConstants[3] = 1.0f;
	
	std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo pipelineDynamicStateInfo = {};
	pipelineDynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	pipelineDynamicStateInfo.pNext = NULL;
	pipelineDynamicStateInfo.flags = 0;
	pipelineDynamicStateInfo.dynamicStateCount = dynamicStateEnables.size();
	pipelineDynamicStateInfo.pDynamicStates = dynamicStateEnables.data();

	
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.pNext = NULL;
	pipelineLayoutCreateInfo.flags = 0;
	pipelineLayoutCreateInfo.setLayoutCount = 0;
	pipelineLayoutCreateInfo.pSetLayouts = NULL;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = NULL;

	VkPipelineLayout pipelineLayout = {};
	CHECK_VK_ERROR(vkCreatePipelineLayout(logicalDevice, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

	uint32_t pipelineCreateInfoCount = 1;
	VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.pNext = NULL;
	pipelineCreateInfo.flags = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
	pipelineCreateInfo.stageCount = graphicsPipelineStageCount;
	pipelineCreateInfo.pStages = shaderStageCreateinfos.data();
	pipelineCreateInfo.pVertexInputState = &vertexInputState;
	pipelineCreateInfo.pInputAssemblyState = &assemblyStateCreateInfo;
	pipelineCreateInfo.pTessellationState = NULL;
	pipelineCreateInfo.pViewportState = &viewPortStateCreateInfo;
	pipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
	pipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
	pipelineCreateInfo.pDepthStencilState = NULL;
	pipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
	pipelineCreateInfo.pDynamicState = &pipelineDynamicStateInfo;
	pipelineCreateInfo.layout = pipelineLayout;
	pipelineCreateInfo.renderPass = renderPass;
	pipelineCreateInfo.subpass = 0;
	pipelineCreateInfo.basePipelineHandle = NULL;
	pipelineCreateInfo.basePipelineIndex = 0;


	VkPipeline triangleGraphicsPipeline = {};
	CHECK_VK_ERROR(vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE,  pipelineCreateInfoCount, &pipelineCreateInfo, nullptr, &triangleGraphicsPipeline));

	VkSemaphoreCreateInfo imageAcquiredSemaphoreCreateInfo = {};
	imageAcquiredSemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	imageAcquiredSemaphoreCreateInfo.pNext = NULL;
	imageAcquiredSemaphoreCreateInfo.flags = 0;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // this avoids locking rendering on the first frame

	VkSemaphore imageAcquiredSemaphore = {};
	VkSemaphore renderFinishedSemaphore = {};
	VkFence inFlightFence = {};
	vkCreateSemaphore(logicalDevice, &imageAcquiredSemaphoreCreateInfo, NULL, &imageAcquiredSemaphore);
	vkCreateSemaphore(logicalDevice, &imageAcquiredSemaphoreCreateInfo, NULL, &renderFinishedSemaphore);
	vkCreateFence(logicalDevice, &fenceInfo, nullptr, &inFlightFence);

	
	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	VkClearValue backgroundColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };

	VkViewport viewPort = {};
	viewPort.width = 640;
	viewPort.height = 540;
	viewPort.maxDepth = 1.0f;
	viewPort.minDepth = 0.0f;
	viewPort.x = 0;
	viewPort.y = 0;

	VkRect2D sciccor = {};
	sciccor.extent.width = 640;
	sciccor.extent.height = 540;
	sciccor.offset.x = 0;
	sciccor.offset.y = 0;

	while (!glfwWindowShouldClose(window))
	{
		vkWaitForFences(logicalDevice, 1, &inFlightFence, VK_TRUE, UINT64_MAX);

		uint32_t imageIndex;
		vkAcquireNextImageKHR(logicalDevice, swapChain, UINT64_MAX,	imageAcquiredSemaphore, VK_NULL_HANDLE,	&imageIndex);

		VkRenderPassBeginInfo renderPassBeginInfo = {};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.pNext = NULL;
		renderPassBeginInfo.renderPass = renderPass;
		renderPassBeginInfo.framebuffer = swapChainFrameBuffers[imageIndex];
		renderPassBeginInfo.renderArea.offset.x = 0;
		renderPassBeginInfo.renderArea.offset.y = 0;
		renderPassBeginInfo.renderArea.extent.width = 640;
		renderPassBeginInfo.renderArea.extent.height = 480;
		renderPassBeginInfo.clearValueCount = 1;
		renderPassBeginInfo.pClearValues = &backgroundColor;
		
		vkResetCommandBuffer(commandBuffers[imageIndex], 0);
		vkBeginCommandBuffer(commandBuffers[imageIndex], &commandBufferBeginInfo);
		vkCmdBindPipeline(commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, triangleGraphicsPipeline);
		vkCmdSetViewport(commandBuffers[imageIndex], 0, 1, &viewPort);
		vkCmdSetScissor(commandBuffers[imageIndex], 0, 1, &sciccor);
		vkCmdBeginRenderPass(commandBuffers[imageIndex], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdDraw(commandBuffers[imageIndex], 3, 1, 0, 0);
		vkCmdEndRenderPass(commandBuffers[imageIndex]);
		vkEndCommandBuffer(commandBuffers[imageIndex]);


		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &imageAcquiredSemaphore;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &renderFinishedSemaphore;
		
		vkResetFences(logicalDevice, 1, &inFlightFence);
		vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence);

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &renderFinishedSemaphore;
		
		VkSwapchainKHR swapChains[] = { swapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;

		
		
		vkQueuePresentKHR(presentQueue, &presentInfo);

		glfwPollEvents();
	}

	//TODO: make destroying order correct
	vkDestroySemaphore(logicalDevice, imageAcquiredSemaphore, nullptr);
	vkDestroySemaphore(logicalDevice, renderFinishedSemaphore, nullptr);
	vkDestroyFence(logicalDevice, inFlightFence, nullptr);

	for (VkShaderModule module : shaderModules) {
		vkDestroyShaderModule(logicalDevice, module, nullptr);
	}

	vkDestroyPipeline(logicalDevice, triangleGraphicsPipeline, nullptr);

	vkDestroyPipelineLayout(logicalDevice, pipelineLayout, nullptr);

	for (auto framebuffer : swapChainFrameBuffers) {
		vkDestroyFramebuffer(logicalDevice, framebuffer, nullptr);
	}

	for (auto imageView : swapChainImageViews) {
		vkDestroyImageView(logicalDevice, imageView, nullptr);
	}

	vkDestroyCommandPool(logicalDevice, commandPool, nullptr);

	vkDestroyRenderPass(logicalDevice, renderPass, nullptr);

	vkDestroyDevice(logicalDevice, nullptr);

	vkDestroySurfaceKHR(instance, surface, nullptr);

	vkDestroyInstance(instance, nullptr);

	glfwDestroyWindow(window);

	glfwTerminate();

	return 0;
}



