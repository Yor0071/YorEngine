#include "VulkanGraphicsPipeline.h"
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <filesystem>


VulkanGraphicsPipeline::VulkanGraphicsPipeline(VulkanDevice& device, VulkanSwapChain& swapChain, VulkanRenderPass& renderPass, VkDescriptorSetLayout materialSetLayout)
	: device(device), swapChain(swapChain), renderPass(renderPass), materialSetLayout(materialSetLayout)
{
	CreateGraphicsPipeline();
}

VulkanGraphicsPipeline::~VulkanGraphicsPipeline()
{
    VkDevice logicalDevice = device.GetLogicalDevice();

    if (uniformBufferLayout != VK_NULL_HANDLE) // <-- Uniform buffer layout (set = 0)
    {
        vkDestroyDescriptorSetLayout(logicalDevice, uniformBufferLayout, nullptr);
        std::cout << "DescriptorSetLayout (UBO) destroyed\n";
    }

    if (graphicsPipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(logicalDevice, graphicsPipeline, nullptr);
        std::cout << "Graphics pipeline destroyed\n";
    }

    if (pipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(logicalDevice, pipelineLayout, nullptr);
        std::cout << "Pipeline layout destroyed\n";
    }
}

std::vector<char> VulkanGraphicsPipeline::ReadFile(const std::string& filename) {
    std::string glslFilename = filename.substr(0, filename.size() - 4) + ".glsl";

    if (!std::filesystem::exists(filename) ||
        std::filesystem::last_write_time(glslFilename) > std::filesystem::last_write_time(filename))
    {
        std::cout << "[INFO] Compiling shader: " << glslFilename << std::endl;
        CompileShader(glslFilename, filename);
    }

    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

void VulkanGraphicsPipeline::CreateGraphicsPipeline() {
    auto vertShaderCode = ReadFile(shaderDirectory + "vert.spv");
    auto fragShaderCode = ReadFile(shaderDirectory + "frag.spv");

    VkShaderModule vertShaderModule = CreateShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = CreateShaderModule(fragShaderCode);

    // Shader stages
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    // Vertex input
    auto bindingDescription = Vertex::GetBindingDescription();
    auto attributeDescriptions = Vertex::GetAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    // Input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // Viewport and scissor
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapChain.GetSwapChainExtent().width);
    viewport.height = static_cast<float>(swapChain.GetSwapChainExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = swapChain.GetSwapChainExtent();

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Depth stencil
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    // Color blending
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    // Push constants
    struct PushConstantsGPU {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
        alignas(4)  int useTexture;
        alignas(16) glm::vec3 baseColor;
    };

    VkPushConstantRange range{};
    range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    range.offset = 0;
    range.size = sizeof(PushConstantsGPU);

    // === Descriptor Set Layouts ===

    // UBO layout (Set 0)
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo uboLayoutInfo{};
    uboLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    uboLayoutInfo.bindingCount = 1;
    uboLayoutInfo.pBindings = &uboLayoutBinding;

    if (vkCreateDescriptorSetLayout(device.GetLogicalDevice(), &uboLayoutInfo, nullptr, &uniformBufferLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create uniform buffer descriptor set layout.");
    }
 
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts = {
        uniformBufferLayout,
        materialSetLayout
    };

    // === Pipeline Layout ===
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &range;

    if (vkCreatePipelineLayout(device.GetLogicalDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout.");
    }

    // === Final Graphics Pipeline ===
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass.GetRenderPass();
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(device.GetLogicalDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create graphics pipeline!");
    }

    // Cleanup shader modules
    vkDestroyShaderModule(device.GetLogicalDevice(), vertShaderModule, nullptr);
    vkDestroyShaderModule(device.GetLogicalDevice(), fragShaderModule, nullptr);

    std::cout << "Graphics pipeline created" << std::endl;
}


VkShaderModule VulkanGraphicsPipeline::CreateShaderModule(const std::vector<char>& code) {
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device.GetLogicalDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create shader module");
	}
	return shaderModule;
}

void VulkanGraphicsPipeline::CompileShader(const std::string& glslPath, const std::string& spvPath) {
    std::string stage;

    if (glslPath.ends_with("vert.glsl")) {
        stage = "vert";
    }
    else if (glslPath.ends_with("frag.glsl")) {
        stage = "frag";
    }
    else {
        throw std::runtime_error("Unknown shader stage: " + glslPath);
    }

    std::string command = "glslangValidator -V -S " + stage + " " + glslPath + " -o " + spvPath;
    std::cout << "Compiling shader: " << glslPath << std::endl;

    int result = std::system(command.c_str());
    if (result != 0) {
        throw std::runtime_error("Shader compilation failed: " + glslPath);
    }
}