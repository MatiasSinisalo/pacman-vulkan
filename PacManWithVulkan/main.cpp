

#include <stdio.h>
#include <iostream>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

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

struct Vertex {
	glm::vec2 pos;
	glm::vec3 inColor;
	glm::vec2 texCoord;
};

struct VulkanBuffer {
	VkBuffer buffer;
	VkDeviceMemory memory;
	VkMemoryRequirements requirements;
};

struct ubo {
	glm::mat4 worldScale;
};

struct pushConstants {
	glm::mat4 model;
};

uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, VkPhysicalDevice physicalDevice) {
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}

VulkanBuffer createBuffer(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, VkDeviceSize size, VkBufferUsageFlags usage, VkSharingMode sharingMode, VkMemoryPropertyFlags memoryProperties) {
	VulkanBuffer newVulkanBuffer = {};
	VkBufferCreateInfo vertexBuffferCreateInfo = {};
	vertexBuffferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	vertexBuffferCreateInfo.pNext = NULL;
	vertexBuffferCreateInfo.size = size;
	vertexBuffferCreateInfo.usage = usage;
	vertexBuffferCreateInfo.sharingMode = sharingMode;
	
	CHECK_VK_ERROR(vkCreateBuffer(logicalDevice, &vertexBuffferCreateInfo, nullptr, &newVulkanBuffer.buffer));

	vkGetBufferMemoryRequirements(logicalDevice, newVulkanBuffer.buffer, &newVulkanBuffer.requirements);

	VkMemoryAllocateInfo vertexAllocInfo = {};
	vertexAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	vertexAllocInfo.pNext = NULL;
	vertexAllocInfo.allocationSize = newVulkanBuffer.requirements.size;
	vertexAllocInfo.memoryTypeIndex = findMemoryType(newVulkanBuffer.requirements.memoryTypeBits, memoryProperties, physicalDevice);

	VkVertexInputBindingDescription shaderVertexBindingDescription = {};
	shaderVertexBindingDescription.binding = 0;
	shaderVertexBindingDescription.stride = sizeof(Vertex);
	shaderVertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkDeviceMemory bufferMemory = {};
	CHECK_VK_ERROR(vkAllocateMemory(logicalDevice, &vertexAllocInfo, nullptr, &newVulkanBuffer.memory));
	CHECK_VK_ERROR(vkBindBufferMemory(logicalDevice, newVulkanBuffer.buffer, newVulkanBuffer.memory, 0));

	return newVulkanBuffer;
}

void fillBufferWithData(VkDevice logicalDevice, VkDeviceMemory memory, VkDeviceSize memorySize, const void * dataToCopy, size_t dataSize) {
	void* data;
	CHECK_VK_ERROR(vkMapMemory(logicalDevice, memory, 0, memorySize, 0, &data));
	memcpy(data, dataToCopy, dataSize);
	vkUnmapMemory(logicalDevice, memory);

}

VkPipelineShaderStageCreateInfo createShaderStage(VkDevice& logicalDevice, VkShaderStageFlagBits shaderStage, const uint32_t* shaderHeader, const uint32_t shaderHeaderSize, std::vector<VkShaderModule>* shaderModules) {

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

VkDescriptorSetLayout createDescriptorSetLayout(VkDevice logicalDevice) {
	VkDescriptorSetLayoutBinding samplerDescriptionSetBinding = {};
	samplerDescriptionSetBinding.binding = 0;
	samplerDescriptionSetBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerDescriptionSetBinding.descriptorCount = 1;
	samplerDescriptionSetBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	samplerDescriptionSetBinding.pImmutableSamplers = nullptr;


	VkDescriptorSetLayoutBinding UBODescriptionSetBinding = {};
	UBODescriptionSetBinding.binding = 1;
	UBODescriptionSetBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	UBODescriptionSetBinding.descriptorCount = 1;
	UBODescriptionSetBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	UBODescriptionSetBinding.pImmutableSamplers = nullptr;

	std::vector<VkDescriptorSetLayoutBinding> bindings = { samplerDescriptionSetBinding , UBODescriptionSetBinding };

	VkDescriptorSetLayoutCreateInfo samplerDescriptionSetLayotInfo = {};
	samplerDescriptionSetLayotInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	samplerDescriptionSetLayotInfo.pNext = NULL;
	samplerDescriptionSetLayotInfo.flags = 0;
	samplerDescriptionSetLayotInfo.bindingCount = 2;
	samplerDescriptionSetLayotInfo.pBindings = bindings.data();

	VkDescriptorSetLayout descriptorSetLayout;
	vkCreateDescriptorSetLayout(logicalDevice, &samplerDescriptionSetLayotInfo, nullptr, &descriptorSetLayout);
	return descriptorSetLayout;
}

VkDescriptorPool createDescriptorPool(VkDevice logicalDevice, VkDescriptorSetLayout samplerDescriptorLayout, uint32_t swapChainImageCount) {
	VkDescriptorPoolSize samplerPoolSize = {};
	samplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerPoolSize.descriptorCount = 1;

	VkDescriptorPoolSize descriptorPoolSize = {};
	descriptorPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorPoolSize.descriptorCount = 1;

	std::vector<VkDescriptorPoolSize> sizes = { samplerPoolSize , descriptorPoolSize };


	VkDescriptorPoolCreateInfo descriptorPoolInfo{};
	descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolInfo.poolSizeCount = 2;
	descriptorPoolInfo.pPoolSizes = sizes.data();
	descriptorPoolInfo.maxSets = swapChainImageCount * 2;
	VkDescriptorPool descriptorPool;
	vkCreateDescriptorPool(logicalDevice, &descriptorPoolInfo, nullptr, &descriptorPool);
	return descriptorPool;


};

VkDescriptorSet createDescriptorSet(VkDevice logicalDevice, VkDescriptorSetLayout descriptorLayout, VkDescriptorPool descriptorPool) {
	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo;
	descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocateInfo.pNext = NULL;
	descriptorSetAllocateInfo.descriptorPool = descriptorPool;
	descriptorSetAllocateInfo.descriptorSetCount = 1;
	descriptorSetAllocateInfo.pSetLayouts = &descriptorLayout;
	
	VkDescriptorSet descriptorSet;
	vkAllocateDescriptorSets(logicalDevice, &descriptorSetAllocateInfo, &descriptorSet);

	return descriptorSet;
}

void createImageDescriptor(VkDevice logicalDevice, VkImageView imageView, VkSampler sampler, VkDescriptorSet descriptorSet) {
	VkDescriptorImageInfo imageDescriptorInfo{};
	imageDescriptorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageDescriptorInfo.imageView = imageView;
	imageDescriptorInfo.sampler = sampler;

	VkWriteDescriptorSet descriptorWrite = {};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.pNext = NULL;
	descriptorWrite.dstSet = descriptorSet;
	descriptorWrite.dstBinding = 0;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrite.pImageInfo = &imageDescriptorInfo;
	descriptorWrite.pBufferInfo; // ignored
	descriptorWrite.pTexelBufferView; // ignored

	vkUpdateDescriptorSets(logicalDevice, 1, &descriptorWrite, 0, nullptr);
}
void createBufferDescriptor(VkDevice logicalDevice, VkDescriptorSet descriptorSet, VkBuffer buffer) {
	VkDescriptorBufferInfo uboBufferInfo{};
	uboBufferInfo.buffer = buffer;
	uboBufferInfo.offset = 0;
	uboBufferInfo.range = sizeof(ubo);
	VkWriteDescriptorSet descriptorWrite{};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet = descriptorSet;
	descriptorWrite.dstBinding = 1;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pBufferInfo = &uboBufferInfo;
	vkUpdateDescriptorSets(logicalDevice, 1, &descriptorWrite, 0, nullptr);
}
VkPipelineLayout createPipelineLayout(VkDevice logicalDevice, uint32_t swapChainImageCount, VkDescriptorSetLayout descriptorSetLayout) {
	VkPushConstantRange pushConstantRange = {};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(pushConstants);

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.pNext = NULL;
	pipelineLayoutCreateInfo.flags = 0;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
	
	VkPipelineLayout pipelineLayout = {};
	CHECK_VK_ERROR(vkCreatePipelineLayout(logicalDevice, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));
	return pipelineLayout;
}

VkPipeline createPipeline(VkDevice logicalDevice, VkPipelineLayout pipelineLayout, VkRenderPass renderPass, VkShaderModule *shaderModules, VkPipelineShaderStageCreateInfo *shaderStageCreateinfos, uint32_t graphicsPipelineStageCount) {
	VkVertexInputBindingDescription shaderVertexBindingDescription = {};
	shaderVertexBindingDescription.binding = 0;
	shaderVertexBindingDescription.stride = sizeof(Vertex);
	shaderVertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkVertexInputAttributeDescription shaderVertexPositionAttributeDescription = {};
	shaderVertexPositionAttributeDescription.location = 0;
	shaderVertexPositionAttributeDescription.binding = 0;
	shaderVertexPositionAttributeDescription.format = VK_FORMAT_R32G32_SFLOAT;
	shaderVertexPositionAttributeDescription.offset = offsetof(Vertex, pos);

	VkVertexInputAttributeDescription shaderVertexColorAttributeDescription = {};
	shaderVertexColorAttributeDescription.location = 1;
	shaderVertexColorAttributeDescription.binding = 0;
	shaderVertexColorAttributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
	shaderVertexColorAttributeDescription.offset = offsetof(Vertex, inColor);

	VkVertexInputAttributeDescription shaderVertexTexCoordAttributeDescription = {};
	shaderVertexTexCoordAttributeDescription.location = 2;
	shaderVertexTexCoordAttributeDescription.binding = 0;
	shaderVertexTexCoordAttributeDescription.format = VK_FORMAT_R32G32_SFLOAT;
	shaderVertexTexCoordAttributeDescription.offset = offsetof(Vertex, texCoord);

	VkVertexInputAttributeDescription AttributeDescriptions[3] = { shaderVertexPositionAttributeDescription, shaderVertexColorAttributeDescription, shaderVertexTexCoordAttributeDescription };

	VkPipelineVertexInputStateCreateInfo vertexInputState = {};
	vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputState.pNext = NULL;
	vertexInputState.flags = 0;
	vertexInputState.vertexBindingDescriptionCount = 1;
	vertexInputState.pVertexBindingDescriptions = &shaderVertexBindingDescription;
	vertexInputState.vertexAttributeDescriptionCount = 3;
	vertexInputState.pVertexAttributeDescriptions = AttributeDescriptions;

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


	uint32_t pipelineCreateInfoCount = 1;
	VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.pNext = NULL;
	pipelineCreateInfo.flags = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
	pipelineCreateInfo.stageCount = graphicsPipelineStageCount;
	pipelineCreateInfo.pStages = shaderStageCreateinfos;
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

	VkPipeline graphicsPipeline = {};
	CHECK_VK_ERROR(vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, pipelineCreateInfoCount, &pipelineCreateInfo, nullptr, &graphicsPipeline));
	return graphicsPipeline;
}

struct vulkanTexture {
	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	VkImageView textureImageView;
};

void makeImageLayotTransition(VkCommandBuffer commandbuffer, VkQueue graphicsQueue, VkImage textureImage, VkAccessFlags srcAccessMask, 
							  VkAccessFlags dstAccessMask, VkImageLayout oldLayout, VkImageLayout newLayout, VkCommandBufferBeginInfo commandBufferBeginInfo) {
	VkImageSubresourceRange textureImageSubresourceRange = {};
	textureImageSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	textureImageSubresourceRange.baseMipLevel = 0;
	textureImageSubresourceRange.baseArrayLayer = 0;
	textureImageSubresourceRange.layerCount = 1;
	textureImageSubresourceRange.levelCount = 1;

	VkImageMemoryBarrier textureTransitionBarrier = {};
	textureTransitionBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	textureTransitionBarrier.pNext = NULL;
	textureTransitionBarrier.srcAccessMask = srcAccessMask;
	textureTransitionBarrier.dstAccessMask = dstAccessMask;
	textureTransitionBarrier.oldLayout = oldLayout;
	textureTransitionBarrier.newLayout = newLayout;
	textureTransitionBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	textureTransitionBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	textureTransitionBarrier.image = textureImage;
	textureTransitionBarrier.subresourceRange = textureImageSubresourceRange;


	//Transition image to the right format and move it from the tmpbuffer to the GPU side imagebuffer
	vkBeginCommandBuffer(commandbuffer, &commandBufferBeginInfo);
	vkCmdPipelineBarrier(commandbuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &textureTransitionBarrier);
	
	vkEndCommandBuffer(commandbuffer);

	VkSubmitInfo tmpCommandBufferSubmitInfo = {};
	tmpCommandBufferSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	tmpCommandBufferSubmitInfo.commandBufferCount = 1;
	tmpCommandBufferSubmitInfo.pCommandBuffers = &commandbuffer;
	vkQueueSubmit(graphicsQueue, 1, &tmpCommandBufferSubmitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue);

	
}



vulkanTexture createTexture(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, uint32_t graphicsQueueIndex, VkQueue graphicsQueue, VkCommandBuffer commandbuffer, VkCommandBufferBeginInfo commandBufferBeginInfo) {
	//Load an image to tmpBuffer
	int texWidth;
	int texHeight;
	int texChannels;
	stbi_uc* pixels = stbi_load("textures/smile.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	VkDeviceSize imageSize = texWidth * texHeight * 4;
	VulkanBuffer tmpImageBuffer = createBuffer(physicalDevice, logicalDevice, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE, (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
	fillBufferWithData(logicalDevice, tmpImageBuffer.memory, tmpImageBuffer.requirements.size, pixels, imageSize);
	stbi_image_free(pixels);

	VkImageCreateInfo imageCreateInfo = {};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.pNext = NULL;
	imageCreateInfo.flags = 0;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
	imageCreateInfo.extent.width = texWidth;
	imageCreateInfo.extent.height = texHeight;
	imageCreateInfo.extent.depth = 1.0f;
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.queueFamilyIndexCount = 1;
	imageCreateInfo.pQueueFamilyIndices = &graphicsQueueIndex;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	CHECK_VK_ERROR(vkCreateImage(logicalDevice, &imageCreateInfo, nullptr, &textureImage));

	//Allocate the imageBuffer
	VkMemoryRequirements textureImageRequirements;
	vkGetImageMemoryRequirements(logicalDevice, textureImage, &textureImageRequirements);
	VkMemoryAllocateInfo textureAllocInfo = {};
	textureAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	textureAllocInfo.pNext = NULL;
	textureAllocInfo.allocationSize = textureImageRequirements.size;
	textureAllocInfo.memoryTypeIndex = findMemoryType(textureImageRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, physicalDevice);
	CHECK_VK_ERROR(vkAllocateMemory(logicalDevice, &textureAllocInfo, nullptr, &textureImageMemory));
	vkBindImageMemory(logicalDevice, textureImage, textureImageMemory, 0);

	//Barrier for syncing transition from LAYOUT_UNDEFINED to OPTIMAL
	VkImageSubresourceRange textureImageSubresourceRange = {};
	textureImageSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	textureImageSubresourceRange.baseMipLevel = 0;
	textureImageSubresourceRange.baseArrayLayer = 0;
	textureImageSubresourceRange.layerCount = 1;
	textureImageSubresourceRange.levelCount = 1;

	VkImageMemoryBarrier textureTransitionBarrier = {};
	textureTransitionBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	textureTransitionBarrier.pNext = NULL;
	textureTransitionBarrier.srcAccessMask = 0;
	textureTransitionBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	textureTransitionBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	textureTransitionBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	textureTransitionBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	textureTransitionBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	textureTransitionBarrier.image = textureImage;
	textureTransitionBarrier.subresourceRange = textureImageSubresourceRange;

	VkBufferImageCopy imageCopyInformation = {};
	imageCopyInformation.bufferOffset = 0;
	imageCopyInformation.bufferRowLength = 0;
	imageCopyInformation.bufferImageHeight = 0;
	imageCopyInformation.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageCopyInformation.imageSubresource.mipLevel = 0;
	imageCopyInformation.imageSubresource.baseArrayLayer = 0;
	imageCopyInformation.imageSubresource.layerCount = 1;
	imageCopyInformation.imageOffset = { 0, 0, 0 };
	imageCopyInformation.imageExtent = {
		(uint32_t)texWidth,
		(uint32_t)texHeight,
		1
	};

	//Transition image to the right format and move it from the tmpbuffer to the GPU side imagebuffer
	vkBeginCommandBuffer(commandbuffer, &commandBufferBeginInfo);
	vkCmdPipelineBarrier(commandbuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &textureTransitionBarrier);
	vkCmdCopyBufferToImage(commandbuffer, tmpImageBuffer.buffer, textureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopyInformation);
	vkEndCommandBuffer(commandbuffer);

	VkSubmitInfo tmpCommandBufferSubmitInfo = {};
	tmpCommandBufferSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	tmpCommandBufferSubmitInfo.commandBufferCount = 1;
	tmpCommandBufferSubmitInfo.pCommandBuffers = &commandbuffer;
	vkQueueSubmit(graphicsQueue, 1, &tmpCommandBufferSubmitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue);
	vkDestroyBuffer(logicalDevice, tmpImageBuffer.buffer, nullptr);
	vkFreeMemory(logicalDevice, tmpImageBuffer.memory, nullptr);

	//translate the image to read optimal
	makeImageLayotTransition(commandbuffer, graphicsQueue, textureImage, 0, 0, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, commandBufferBeginInfo);

	//Create an imageview to the image
	VkImageViewCreateInfo imageViewCreateInfo = {};
	imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCreateInfo.pNext = NULL;
	imageViewCreateInfo.flags = 0;
	imageViewCreateInfo.image = textureImage;
	imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
	imageViewCreateInfo.subresourceRange = textureImageSubresourceRange;
	VkImageView textureImageView;
	CHECK_VK_ERROR(vkCreateImageView(logicalDevice, &imageViewCreateInfo, nullptr, &textureImageView));
	
	vulkanTexture texture;
	texture.textureImage = textureImage;
	texture.textureImageMemory = textureImageMemory;
	texture.textureImageView = textureImageView;
	return texture;
}

VkSampler createSampler(VkDevice logicalDevice) {
	//Create an sampler to use the imageView
	VkSamplerCreateInfo samplerCreateInfo = {};
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.pNext = NULL;
	samplerCreateInfo.flags = 0;
	samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.mipLodBias = 1.0f;
	samplerCreateInfo.anisotropyEnable = VK_FALSE;
	samplerCreateInfo.maxAnisotropy = 1.0f;
	samplerCreateInfo.compareEnable = VK_FALSE;
	samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerCreateInfo.minLod = 0.0f;
	samplerCreateInfo.maxLod = 0.0f;
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

	VkSampler textureSampler = {};
	CHECK_VK_ERROR(vkCreateSampler(logicalDevice, &samplerCreateInfo, nullptr, &textureSampler));
	return textureSampler;
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
	
	
	const std::vector<Vertex> vertices = {
	{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
	{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
	{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
	{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
	};

	const std::vector<uint16_t> indices = {
		0, 1, 2, 2, 3, 0
	};
	
	

	VulkanBuffer vertexBuffer = createBuffer(physicalDevice, logicalDevice, sizeof(vertices[0]) * vertices.size(), 
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, 
		(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
	fillBufferWithData(logicalDevice, vertexBuffer.memory, vertexBuffer.requirements.size, vertices.data(), sizeof(vertices[0]) * vertices.size());

	VulkanBuffer indexBuffer = createBuffer(physicalDevice, logicalDevice, sizeof(indices[0]) * indices.size(),
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, 
		(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
		);
	fillBufferWithData(logicalDevice, indexBuffer.memory, indexBuffer.requirements.size, indices.data(), sizeof(indices[0]) * indices.size());

	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	//create image, image memory and imageview and store them inside vulkanTexture struct
	vulkanTexture texture = createTexture(logicalDevice, physicalDevice, graphicsQueueIndex, graphicsQueue, commandBuffers[0], commandBufferBeginInfo);

	VkSampler textureSampler = createSampler(logicalDevice);

	
	VulkanBuffer uboBuffer = createBuffer(physicalDevice, logicalDevice, sizeof(ubo), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
	ubo testData = {};
	testData.worldScale = glm::scale(glm::mat4(1.0f), glm::vec3(0.4f, 0.4f, 1.0f));
	fillBufferWithData(logicalDevice,uboBuffer.memory, uboBuffer.requirements.size, &testData, sizeof(testData));

	VkDescriptorSetLayout descriptorSetLayout = createDescriptorSetLayout(logicalDevice);
	VkDescriptorPool descriptorPool = createDescriptorPool(logicalDevice, descriptorSetLayout, swapChainImageCount);
	VkDescriptorSet descriptorSet = createDescriptorSet(logicalDevice, descriptorSetLayout, descriptorPool);
	
	createBufferDescriptor(logicalDevice, descriptorSet, uboBuffer.buffer);
	createImageDescriptor(logicalDevice, texture.textureImageView, textureSampler, descriptorSet);

	//Create Graphics pipeline
	uint32_t graphicsPipelineStageCount = 2;
	std::vector<VkShaderModule> shaderModules;
	std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateinfos(graphicsPipelineStageCount);
	shaderStageCreateinfos[0] = createShaderStage(logicalDevice, VK_SHADER_STAGE_VERTEX_BIT, shader_vert, sizeof(shader_vert), &shaderModules);
	shaderStageCreateinfos[1] = createShaderStage(logicalDevice, VK_SHADER_STAGE_FRAGMENT_BIT, shader_frag, sizeof(shader_frag), &shaderModules);
	VkPipelineLayout pipelineLayout = createPipelineLayout(logicalDevice, swapChainImageCount, descriptorSetLayout);
	VkPipeline triangleGraphicsPipeline = createPipeline(logicalDevice, pipelineLayout, renderPass, shaderModules.data(), shaderStageCreateinfos.data(), graphicsPipelineStageCount);

	//TODO: create semaphores per frame
	VkSemaphoreCreateInfo imageAcquiredSemaphoreCreateInfo = {};
	imageAcquiredSemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	imageAcquiredSemaphoreCreateInfo.pNext = NULL;
	imageAcquiredSemaphoreCreateInfo.flags = 0;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // this avoids locking rendering on the first frame

	std::vector<VkSemaphore> imageAcquiredSemaphores(swapChainImageCount);
	std::vector<VkSemaphore> renderFinishedSemaphores(swapChainImageCount);
	std::vector<VkFence> inFlightFences (swapChainImageCount);
	for (int i = 0; i < swapChainImageCount; i++) {
		vkCreateSemaphore(logicalDevice, &imageAcquiredSemaphoreCreateInfo, NULL, &imageAcquiredSemaphores[i]);
		vkCreateSemaphore(logicalDevice, &imageAcquiredSemaphoreCreateInfo, NULL, &renderFinishedSemaphores[i]);
		vkCreateFence(logicalDevice, &fenceInfo, nullptr, &inFlightFences[i]);
	}
	
	

	VkClearValue backgroundColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };

	VkViewport viewPort = {};
	viewPort.width = 640;
	viewPort.height = 480;
	viewPort.maxDepth = 1.0f;
	viewPort.minDepth = 0.0f;
	viewPort.x = 0;
	viewPort.y = 0;

	VkRect2D sciccor = {};
	sciccor.extent.width = 640;
	sciccor.extent.height = 480;
	sciccor.offset.x = 0;
	sciccor.offset.y = 0;
	
	uint32_t imageIndex = 0;

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
	
	VkBuffer vertexBuffers[] = { vertexBuffer.buffer };
	VkDeviceSize offsets[] = { 0 };
	while (!glfwWindowShouldClose(window))
	{
		vkWaitForFences(logicalDevice, 1, &inFlightFences[imageIndex], VK_TRUE, UINT64_MAX);

		vkAcquireNextImageKHR(logicalDevice, swapChain, UINT64_MAX,	imageAcquiredSemaphores[imageIndex], VK_NULL_HANDLE, &imageIndex);

		renderPassBeginInfo.framebuffer = swapChainFrameBuffers[imageIndex];

		vkResetCommandBuffer(commandBuffers[imageIndex], 0);
		vkBeginCommandBuffer(commandBuffers[imageIndex], &commandBufferBeginInfo);
		vkCmdBindPipeline(commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, triangleGraphicsPipeline);
		vkCmdSetViewport(commandBuffers[imageIndex], 0, 1, &viewPort);
		vkCmdSetScissor(commandBuffers[imageIndex], 0, 1, &sciccor);
		vkCmdBeginRenderPass(commandBuffers[imageIndex], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindVertexBuffers(commandBuffers[imageIndex], 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffers[imageIndex], indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);
		vkCmdBindDescriptorSets(commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
		
		for (int i = 0; i < 2; i++) {
			pushConstants data = {};
			data.model = glm::translate(glm::mat4(1.0f), glm::vec3(0.5f * i, 0.5f * i, 0.0f));
			vkCmdPushConstants(commandBuffers[imageIndex], pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(data), &data);
			vkCmdDrawIndexed(commandBuffers[imageIndex], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
		}
		
	
		
		
		vkCmdEndRenderPass(commandBuffers[imageIndex]);
		vkEndCommandBuffer(commandBuffers[imageIndex]);

		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &imageAcquiredSemaphores[imageIndex];
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &renderFinishedSemaphores[imageIndex];
		
		vkResetFences(logicalDevice, 1, &inFlightFences[imageIndex]);
		vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[imageIndex]);

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &renderFinishedSemaphores[imageIndex];
		
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &swapChain;
		presentInfo.pImageIndices = &imageIndex;

		vkQueuePresentKHR(presentQueue, &presentInfo);

		glfwPollEvents();
		
		//advance the frame forwards to look at the correct semaphore and fence
		imageIndex = (imageIndex + 1) % swapChainImageCount;
	}

	vkQueueWaitIdle(graphicsQueue);

	vkDestroyDescriptorSetLayout(logicalDevice, descriptorSetLayout, nullptr);
	vkDestroyDescriptorPool(logicalDevice, descriptorPool, nullptr);

	vkDestroySampler(logicalDevice, textureSampler, nullptr);
	vkDestroyImageView(logicalDevice, texture.textureImageView, nullptr);

	vkDestroyImage(logicalDevice, texture.textureImage, nullptr);
	vkFreeMemory(logicalDevice, texture.textureImageMemory, nullptr);

	vkDestroyBuffer(logicalDevice, uboBuffer.buffer, nullptr);
	vkFreeMemory(logicalDevice, uboBuffer.memory, nullptr);

	vkDestroyBuffer(logicalDevice, vertexBuffer.buffer, nullptr);
	vkFreeMemory(logicalDevice, vertexBuffer.memory, nullptr);

	vkDestroyBuffer(logicalDevice, indexBuffer.buffer, nullptr);
	vkFreeMemory(logicalDevice, indexBuffer.memory, nullptr);

	for (int i = 0; i < swapChainImageCount; i++) {
		vkDestroySemaphore(logicalDevice, imageAcquiredSemaphores[i], nullptr);
		vkDestroySemaphore(logicalDevice, renderFinishedSemaphores[i], nullptr);
		vkDestroyFence(logicalDevice, inFlightFences[i], nullptr);
	}
	

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

	vkDestroySwapchainKHR(logicalDevice, swapChain, nullptr);

	vkDestroyRenderPass(logicalDevice, renderPass, nullptr);

	vkDestroyDevice(logicalDevice, nullptr);
	
	vkDestroySurfaceKHR(instance, surface, nullptr);

	vkDestroyInstance(instance, nullptr);

	glfwDestroyWindow(window);

	glfwTerminate();

	return 0;
}



