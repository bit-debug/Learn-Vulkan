#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// GNU
#include <unistd.h>

// C
#include <cstdio>
#include <cstdlib>

// C++
#include <iostream>
#include <fstream>

#include <exception>
#include <optional>
#include <string>
#include <limits>
#include <chrono>

#include <algorithm>
#include <vector>
#include <array>
#include <set>

#include "main.hpp"

/********************************************************************************************************************************/
#define vkCritical(result) if (result != VK_SUCCESS) { throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__) + ": Vulkan Failure\n"); }

#if defined(ENABLE_LOGGING)
#define LOG(...) printf(BRIGHT_RED); printf(__VA_ARGS__); printf(CLEAR);
#else
#define LOG(...)
#endif

#if defined(ENABLE_DEBUG_LOGGING)
#define DLOG(...) printf(RED); printf(__VA_ARGS__); printf(CLEAR);
#else
#define DLOG(...)
#endif
/********************************************************************************************************************************/

/********************************************************************************************************************************/
struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }

    void getQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        uint32_t i = 0;
        for (const auto& queueFamily : queueFamilies) {
            // VK_QUEUE_GRAPHICS_BIT implicitly the support of VK_QUEUE_TRANSFER_BIT
            if (!this->graphicsFamily.has_value() && (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
                this->graphicsFamily = i;
            }

            if (!this->presentFamily.has_value()) {
                VkBool32 presentSupport = false;
                vkCritical(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport));
                if (presentSupport) {
                    this->presentFamily = i;
                }
            }
            
            if (this->isComplete()) {
                break;
            }
            i++;
        }
    }
};

struct SwapchainDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;

    bool isComplete() {
        return (!formats.empty()) && (!presentModes.empty());
    }

    void getSwapchainDetails(const VkPhysicalDevice& device, VkSurfaceKHR surface) {
        vkCritical(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &this->capabilities));

        uint32_t formatCount;
        vkCritical(vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr));
        if (formatCount != 0) {
            this->formats.resize(formatCount);
            vkCritical(vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, this->formats.data()));
        }

        uint32_t presentModeCount;
        vkCritical(vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr));
        if (presentModeCount != 0) {
            this->presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, this->presentModes.data());
        }
    }
};

std::vector<const char*> desiredLayers = {};

std::vector<const char*> deviceExtensions = {};

void readFileAsByteArray(const std::string& filepath, std::vector<int8_t>& buffer) {
    std::ifstream ifs(filepath, std::ios::ate | std::ios::binary);
    if (!ifs.is_open()) {
        throw std::runtime_error("Failed to open the file.\n");
    }

    size_t filesize = (size_t) ifs.tellg();
    buffer.resize(filesize);
    ifs.seekg(0);
    ifs.read(reinterpret_cast<char*>(buffer.data()), filesize);

    if (ifs.fail()) {
        throw std::runtime_error("Failed to read the file.\n");
    }
    ifs.close();
}
/********************************************************************************************************************************/

/********************************************************************************************************************************/
class HelloTriangleApplication
{
public:
    void run() {
        setupWindow();
        configVulkan();
        setupVulkan();
        mainLoop();
        cleanup();
    }

private:
    const uint32_t WIDTH = 1024;
    const uint32_t HEIGHT = 768;
    GLFWwindow* window;

    struct Vertex {
        glm::vec2 position;
        glm::vec3 color;

        static VkVertexInputBindingDescription getBindingDescription() {
            VkVertexInputBindingDescription bindingDescription;
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(Vertex);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            // copy return
            return bindingDescription;
        }

        static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
            std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions;
            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[0].offset = offsetof(Vertex, position);

            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[1].offset = offsetof(Vertex, color);
            // copy return
            return attributeDescriptions;
        }
    };

    #if 0
    const std::vector<Vertex> vertices = {
        {{0.0f, -0.5f}, {1.0f, 1.0f, 1.0f}},
        {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
        {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
    };
    #endif

    const std::vector<Vertex> vertices = {
        {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
        {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
    };
    using Index_t = uint16_t;
    const VkIndexType VK_INDEX_TYPE = VK_INDEX_TYPE_UINT16;
    const std::vector<Index_t> indices = {0, 1, 2, 2, 3, 0};

    struct UniformBufferObject {
        alignas(16) glm::mat4 model;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 projection;
    };

    void setupWindow() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    }

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
        app->framebufferResized = true;
    }

    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkSurfaceKHR surface;

    VkPhysicalDevice physicalDevice;
    VkDevice device;

    QueueFamilyIndices queueFamilyIndices;
    SwapchainDetails swapchainDetails;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkSwapchainKHR swapchain;
    
    std::vector<VkImage> swapchainImages;
    VkFormat swapchainImageFormat;
    VkExtent2D swapchainImageExtent;
    std::vector<VkImageView> swapchainImageViews;

    VkRenderPass renderPass;
    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    std::vector<VkFramebuffer> swapchainFramebuffers;
    
    VkBuffer vertexBuffer;
    VkDeviceMemory memoryVertexBuffer;
    VkBuffer indexBuffer;
    VkDeviceMemory memoryIndexBuffer;
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> memoryUniformBuffers;
    
    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;
        
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;
    size_t currentFrame = 0;

    bool framebufferResized = false;

    void createDescriptorSetLayout() {
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        uboLayoutBinding.pImmutableSamplers = nullptr; // Optional
        
        VkDescriptorSetLayoutCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        createInfo.bindingCount = 1;
        createInfo.pBindings = &uboLayoutBinding;
        vkCritical(vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &descriptorSetLayout));
    }

    void setupVulkan() {
        createInstance();
        createDebugMessenger();
        createSurface();
        selectPhysicalDevice();
        createDevice();
        createSwapchain();
        createImageViews();
        createRenderPass();
        createDescriptorSetLayout();
        createGraphicsPipeline();
        createFramebuffers();
        createCommandPool();
        createVertexBuffer();
        createIndexBuffer();
        createUniformBuffers();
        createDescriptorPool();
        createDescriptorSets();
        createCommandBuffers();
        createSemaphores();
        createFences();
    }

    void refreshSwapchain() {
        int width = 0, height = 0;
        // glfwGetFramebufferSize(window, &width, &height);
        // ToDo: should avoid free spinning
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(device);

        #if 0
        // cleanup before recreating new objects
        for (auto& framebuffer : swapchainFramebuffers) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }

        vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
        
        vkDestroyPipeline(device, graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        vkDestroyRenderPass(device, renderPass, nullptr);

        for (size_t i = 0; i < swapchainImageViews.size(); i++) {
            vkDestroyImageView(device, swapchainImageViews[i], nullptr);
            vkDestroySwapchainKHR(device, swapchain, nullptr);
        }
        
        for (size_t i = 0; i < swapchainImages.size(); i++) {
            vkDestroyBuffer(device, uniformBuffers[i], nullptr);
            vkFreeMemory(device, memoryUniformBuffers[i], nullptr);
        }

        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        #endif

        cleanupSwapchainRelated();

        // recreate swapchain and related objects
        createSwapchain();
        createImageViews();
        createRenderPass();
        createGraphicsPipeline();
        createFramebuffers();
        createUniformBuffers();
        createDescriptorPool();
        createDescriptorSets();
        createCommandBuffers();
    }

    void cleanupSwapchainRelated() {
        for (auto& framebuffer : swapchainFramebuffers) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }

        vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
        
        vkDestroyPipeline(device, graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        vkDestroyRenderPass(device, renderPass, nullptr);

        for (size_t i = 0; i < swapchainImageViews.size(); i++) {
            vkDestroyImageView(device, swapchainImageViews[i], nullptr);
        }

        vkDestroySwapchainKHR(device, swapchain, nullptr);
        
        for (size_t i = 0; i < swapchainImages.size(); i++) {
            vkDestroyBuffer(device, uniformBuffers[i], nullptr);
            vkFreeMemory(device, memoryUniformBuffers[i], nullptr);
        }

        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    }

    void cleanup() {
        #if 0
        for (auto& framebuffer : swapchainFramebuffers) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }

        vkDestroyPipeline(device, graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        vkDestroyRenderPass(device, renderPass, nullptr);

        for (auto& imageView : swapchainImageViews) {
            vkDestroyImageView(device, imageView, nullptr);
        }

        vkDestroySwapchainKHR(device, swapchain, nullptr);
        
        for (size_t i = 0; i < swapchainImages.size(); i++) {
            vkDestroyBuffer(device, uniformBuffers[i], nullptr);
            vkFreeMemory(device, memoryUniformBuffers[i], nullptr);
        }
        
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        #endif
        
        cleanupSwapchainRelated();

        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

        vkDestroyBuffer(device, indexBuffer, nullptr);
        vkFreeMemory(device, memoryIndexBuffer, nullptr);
        vkDestroyBuffer(device, vertexBuffer, nullptr);
        vkFreeMemory(device, memoryVertexBuffer, nullptr);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroyFence(device, inFlightFences[i], nullptr);
            vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        }

        vkDestroyCommandPool(device, commandPool, nullptr);

        vkDestroyDevice(device, nullptr);
        teardownDebugMessenger();
        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyInstance(instance, nullptr);

        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            renderFrame();
        }
        vkDeviceWaitIdle(device);
    }

    void configVulkan() {
        if (ENABLE_VALIDATION_LAYER) {
            desiredLayers.emplace_back("VK_LAYER_KHRONOS_validation");
        }

        deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    }

    void createInstance() {
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "LearnVulkan";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        // ToDo: check validation layers availability
        createInfo.enabledLayerCount = static_cast<uint32_t>(desiredLayers.size());
        createInfo.ppEnabledLayerNames = desiredLayers.data();

        // Print available extensions
        getVulkanInstanceExtensions();

        // Enable extensions
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (ENABLE_DEBUG_MESSENGER) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        vkCritical(vkCreateInstance(&createInfo, nullptr, &instance));
    }

    void getVulkanInstanceExtensions() {
        uint32_t extensionCount = 0;
        std::vector<VkExtensionProperties> extensions;
        vkCritical(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr));
        extensions.resize(extensionCount);
        vkCritical(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data()));
        LOG("Available Vulkan Instance Extensions:\n");
        for (const auto& extension : extensions) {
            LOG(WHITE "\t%s\n" CLEAR, extension.extensionName);
        }
    }

    void createDebugMessenger() {
        if (!ENABLE_DEBUG_MESSENGER) {
            return;
        }

        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = 
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | 
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | 
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = 
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | 
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
        createInfo.pUserData = nullptr; // optional

        auto vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (vkCreateDebugUtilsMessengerEXT != nullptr) {
            vkCritical(vkCreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger));
        } else {
            vkCritical(VK_ERROR_EXTENSION_NOT_PRESENT);
        }
    }

    static VkBool32 debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
        
        switch (messageSeverity)
        {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: {

        }
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: {
            // printf(GRAY "%s\n" CLEAR, pCallbackData->pMessage);
            break;
        }
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: {
            printf(RED "%s\n" CLEAR, pCallbackData->pMessage);
            break;
        }
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: {
            printf(RED "%s\n" CLEAR, pCallbackData->pMessage);
            break;
        }
        default:
            return VK_TRUE;
        }
        
        return VK_FALSE; 
    }

    void teardownDebugMessenger() {
        if (!ENABLE_DEBUG_MESSENGER) {
            return;
        }
        
        auto vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (vkDestroyDebugUtilsMessengerEXT != nullptr) {
            vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        } else {
            vkCritical(VK_ERROR_EXTENSION_NOT_PRESENT);
        }
    }

    void createSurface() {
        vkCritical(glfwCreateWindowSurface(instance, window, nullptr, &surface));
    }

    void selectPhysicalDevice() {
        uint32_t deviceCount = 0;
        vkCritical(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr));
        // LOG("%u GPU(s) available\n", deviceCount);
        if (deviceCount == 0) {
            throw std::runtime_error("Failed to find any GPUs with Vulkan\n");
        }
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkCritical(vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()));

        for (const auto& device : devices) { 
            if (evaluatePhysicalDevice(device) > 0) {
                physicalDevice = device;
                break;
            }
        }

        if (physicalDevice == VK_NULL_HANDLE) {
            throw std::runtime_error("Failed to find a suitable GPU with Vulkan\n");
        }
    }

    uint32_t evaluatePhysicalDevice(const VkPhysicalDevice& device) {
        uint32_t score = 100;

        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        LOG("Evaluating Device %s:\n", deviceProperties.deviceName)
        LOG(WHITE "\tType: %d\n\tAPI: %u\n\tDriver: %u\n" CLEAR, deviceProperties.deviceType, deviceProperties.apiVersion, deviceProperties.driverVersion);

        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        queueFamilyIndices.getQueueFamilies(device, surface);
        if (queueFamilyIndices.isComplete()) {
            score += 1;
        } else {
            return 0;
        }

        if (evaluateDeviceExtensions(device)) {
            score += 1;
        } else {
            return 0;
        }

        swapchainDetails.getSwapchainDetails(device, surface);
        if (swapchainDetails.isComplete()) {
            score += 1;
        } else {
            return 0;
        }
        
        return score;
    }

    bool evaluateDeviceExtensions(const VkPhysicalDevice& device) {
        uint32_t extensionCount;
        vkCritical(vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr));
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkCritical(vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data()));
        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
        for (const auto& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }
        return requiredExtensions.empty();
    }

    void createDevice() {
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {queueFamilyIndices.graphicsFamily.value(), queueFamilyIndices.presentFamily.value()};

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkDeviceCreateInfo deviceCreateInfo{};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();

        VkPhysicalDeviceFeatures deviceFeatures{};
        deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
        if (ENABLE_VALIDATION_LAYER) {
            // Device validation layers are ignored by modern Vulkan implementation
            deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(deviceExtensions.size());
            deviceCreateInfo.ppEnabledLayerNames = deviceExtensions.data();
        }

        // VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME is optionally required (beta) for Vulkan on top of native graphics library (Metal)
        deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
        vkCritical(vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device));

        vkGetDeviceQueue(device, queueFamilyIndices.graphicsFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(device, queueFamilyIndices.presentFamily.value(), 0, &presentQueue);
    }

    void createSwapchain() {
        swapchainDetails.getSwapchainDetails(physicalDevice, surface);

        VkSurfaceFormatKHR surfaceFormat = selectSurfaceFormat(swapchainDetails.formats);
        VkPresentModeKHR presentMode = selectPresentMode(swapchainDetails.presentModes);
        VkExtent2D extent = selectSwapchainExtent(swapchainDetails.capabilities);

        uint32_t imageCount = swapchainDetails.capabilities.minImageCount + 1;
        if (swapchainDetails.capabilities.maxImageCount > 0 && imageCount > swapchainDetails.capabilities.maxImageCount) {
            imageCount = swapchainDetails.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        uint32_t indices[] = {queueFamilyIndices.graphicsFamily.value(), queueFamilyIndices.presentFamily.value()};
        if (queueFamilyIndices.graphicsFamily != queueFamilyIndices.presentFamily) { 
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = indices;
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0; // optional
            createInfo.pQueueFamilyIndices = nullptr; // optional
        }
        
        createInfo.preTransform = swapchainDetails.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        // Only ever create one swapchain; Never resize
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        vkCritical(vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain));

        vkCritical(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr)); 
        swapchainImages.resize(imageCount);
        vkCritical(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data()));

        swapchainImageFormat = surfaceFormat.format;
        swapchainImageExtent = extent;
    }

    VkSurfaceFormatKHR selectSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
            for (const auto& availableFormat : availableFormats) {
                if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                    return availableFormat;
                }
            }
            return availableFormats[0];
        }

    VkPresentModeKHR selectPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }
        
        // VK_PRESENT_MODE_FIFO_KHR is guaranteed to be available
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D selectSwapchainExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max() && capabilities.currentExtent.height != std::numeric_limits<uint32_t>::max()) { 
            LOG("Current Swapchain Extent: (%d x %d)\n", capabilities.currentExtent.width, capabilities.currentExtent.height);
            return capabilities.currentExtent;
        } else {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);
            VkExtent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
            actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
            actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
            return actualExtent;
        }
    }

    void createImageViews() {
        swapchainImageViews.resize(swapchainImages.size());
        for (size_t i = 0; i < swapchainImages.size(); i++) {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = swapchainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = swapchainImageFormat;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;
            vkCritical(vkCreateImageView(device, &createInfo, nullptr, &swapchainImageViews[i]));
        }
    }

    void createRenderPass() {
        VkRenderPassCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapchainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                
        createInfo.attachmentCount = 1;
        createInfo.pAttachments = &colorAttachment;

        VkAttachmentReference colorAttachmentReference{};
        colorAttachmentReference.attachment = 0;
        colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentReference;

        createInfo.subpassCount = 1;
        createInfo.pSubpasses = &subpass;

        // There are two built-in dependencies that take care of the transition at the start of the render pass and at the end of the render pass, but the former does not occur at the right time. It assumes that the transi- tion occurs at the start of the pipeline, but we haven’t acquired the image yet at that point! There are two ways to deal with this problem. We could change the waitStages for the imageAvailableSemaphore to VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT to ensure that the render passes don’t begin until the image is available, or we can make the render pass wait for the VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT stage.
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        
        createInfo.dependencyCount = 1;
        createInfo.pDependencies = &dependency;

        vkCritical(vkCreateRenderPass(device, &createInfo, nullptr, &renderPass));
    }

    void createGraphicsPipeline() {
        std::vector<int8_t> vShaderCode;
        std::vector<int8_t> fShaderCode;
        readFileAsByteArray("build/vertex.spv", vShaderCode);
        readFileAsByteArray("build/fragment.spv", fShaderCode);

        VkShaderModule vShaderModule, fShaderModule;
        createShaderModule(vShaderCode, vShaderModule);
        createShaderModule(fShaderCode, fShaderModule);

        VkPipelineShaderStageCreateInfo vShaderStageInfo{};
        vShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vShaderStageInfo.module = vShaderModule;
        vShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fShaderStageInfo{};
        fShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fShaderStageInfo.module = fShaderModule;
        fShaderStageInfo.pName = "main";
        
        VkPipelineShaderStageCreateInfo shaderStages[] = {vShaderStageInfo, fShaderStageInfo};

        // following are fixed (non-programmable) stages, still we need to create them explicitly
        VkPipelineVertexInputStateCreateInfo vertexInputStageCreateInfo{};
        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();
        vertexInputStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputStageCreateInfo.vertexBindingDescriptionCount = 1;
        vertexInputStageCreateInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputStageCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputStageCreateInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo{};
        inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(swapchainImageExtent.width);
        viewport.height = static_cast<float>(swapchainImageExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapchainImageExtent;

        VkPipelineViewportStateCreateInfo viewportStateCreateInfo{};
        viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportStateCreateInfo.viewportCount = 1;
        viewportStateCreateInfo.pViewports = &viewport;
        viewportStateCreateInfo.scissorCount = 1;
        viewportStateCreateInfo.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo{};
        rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
        rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
        rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizationStateCreateInfo.lineWidth = 1.0f;
        rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
        rasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f; // optional
        rasterizationStateCreateInfo.depthBiasClamp = 0.0f; // optional
        rasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f; // optional

        VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo{};
        multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
        multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampleStateCreateInfo.minSampleShading = 1.0f; // optional
        multisampleStateCreateInfo.pSampleMask = nullptr; // optional
        multisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE; // optional
        multisampleStateCreateInfo.alphaToOneEnable = VK_FALSE; // optional

        VkPipelineColorBlendAttachmentState colorBlendAttachmentState{};
        colorBlendAttachmentState.colorWriteMask = 
            VK_COLOR_COMPONENT_R_BIT 
            | VK_COLOR_COMPONENT_G_BIT 
            | VK_COLOR_COMPONENT_B_BIT 
            | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachmentState.blendEnable = VK_FALSE;
        colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // optional
        colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // optional
        colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD; // optional
        colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // optional
        colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // optional
        colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD; // optional

        VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo{};
        colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendStateCreateInfo.logicOpEnable = VK_FALSE; 
        colorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY; // optional
        colorBlendStateCreateInfo.attachmentCount = 1;
        colorBlendStateCreateInfo.pAttachments = &colorBlendAttachmentState;
        colorBlendStateCreateInfo.blendConstants[0] = 0.0f; // optional
        colorBlendStateCreateInfo.blendConstants[1] = 0.0f; // optional
        colorBlendStateCreateInfo.blendConstants[2] = 0.0f; // optional
        colorBlendStateCreateInfo.blendConstants[3] = 0.0f; // optional

        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
        pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.setLayoutCount = 1;
        pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
        pipelineLayoutCreateInfo.pushConstantRangeCount = 0; // optional
        pipelineLayoutCreateInfo.pPushConstantRanges = nullptr; // optional
        vkCritical(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

        VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineCreateInfo.stageCount = 2;
        pipelineCreateInfo.pStages = shaderStages;
        pipelineCreateInfo.pVertexInputState = &vertexInputStageCreateInfo;
        pipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
        pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
        pipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
        pipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
        pipelineCreateInfo.pDepthStencilState = nullptr; // optional
        pipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
        pipelineCreateInfo.pDynamicState = nullptr; // optional
        pipelineCreateInfo.layout = pipelineLayout;
        pipelineCreateInfo.renderPass = renderPass;
        pipelineCreateInfo.subpass = 0;
        pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE; // optional
        pipelineCreateInfo.basePipelineIndex = -1; // optional

        vkCritical(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &graphicsPipeline));

        vkDestroyShaderModule(device, fShaderModule, nullptr);
        vkDestroyShaderModule(device, vShaderModule, nullptr);
    }

    void createShaderModule(const std::vector<int8_t>& code, VkShaderModule& shaderModule) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        vkCritical((vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule)));
    }

    void createFramebuffers() {
        swapchainFramebuffers.resize(swapchainImageViews.size());
        for (size_t i = 0; i < swapchainImageViews.size(); i++) {
            VkImageView attachments[] = {
                swapchainImageViews[i]
            };
            VkFramebufferCreateInfo createInfo{}; 
            createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            createInfo.renderPass = renderPass;
            createInfo.attachmentCount = 1;
            createInfo.pAttachments = attachments; 
            createInfo.width = swapchainImageExtent.width;
            createInfo.height = swapchainImageExtent.height;
            createInfo.layers = 1;
            vkCritical(vkCreateFramebuffer(device, &createInfo, nullptr, &swapchainFramebuffers[i]));
        }
    }

    void createCommandPool() {
        VkCommandPoolCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        createInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
        createInfo.flags = 0; // optional
        vkCritical(vkCreateCommandPool(device, &createInfo, nullptr, &commandPool));
    }

    void createVertexBuffer() {
        #if 0
        VkBufferCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        createInfo.size = sizeof(Vertex) * vertices.size();
        createInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        vkCritical(vkCreateBuffer(device, &createInfo, nullptr, &vertexBuffer));

        VkMemoryRequirements memoryRequirements;
        vkGetBufferMemoryRequirements(device, vertexBuffer, &memoryRequirements);

        VkMemoryAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocateInfo.allocationSize = memoryRequirements.size;
        allocateInfo.memoryTypeIndex = selectMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        vkCritical(vkAllocateMemory(device, &allocateInfo, nullptr, &memoryVertexBuffer));

        vkBindBufferMemory(device, vertexBuffer, memoryVertexBuffer, 0);

        void* data;
        vkMapMemory(device, memoryVertexBuffer, 0, createInfo.size, 0, &data);
        memcpy(data, vertices.data(), (size_t) createInfo.size);
        vkUnmapMemory(device, memoryVertexBuffer);
        #endif
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        VkDeviceSize bufferSize = sizeof(Vertex) * vertices.size();
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
        
        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, vertices.data(), (size_t) bufferSize);
        vkUnmapMemory(device, stagingBufferMemory);
        
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, memoryVertexBuffer);

        copyBuffer(stagingBuffer, vertexBuffer, bufferSize);
        
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }

    void createIndexBuffer() {
        VkDeviceSize bufferSize = sizeof(Index_t) * indices.size();
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
        
        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, indices.data(), (size_t)bufferSize);
        vkUnmapMemory(device, stagingBufferMemory);

        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, memoryIndexBuffer);

        copyBuffer(stagingBuffer, indexBuffer, bufferSize);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }

    void createUniformBuffers() {
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);
        uniformBuffers.resize(swapchainImages.size());
        memoryUniformBuffers.resize(swapchainImages.size());
        for (size_t i = 0; i < swapchainImages.size(); i++) {
            createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], memoryUniformBuffers[i]);
        }
    }

    void updateUniformBuffer(uint32_t currentImage) {
        static auto startTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        float timeElapsed = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        UniformBufferObject ubo{};
        ubo.model = glm::rotate(glm::mat4(1.0f), timeElapsed * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.projection = glm::perspective(glm::radians(45.0f), swapchainImageExtent.width / (float) swapchainImageExtent.height, 0.1f, 10.0f);
        ubo.projection[1][1] *= -1;

        void* data;
        vkMapMemory(device, memoryUniformBuffers[currentImage], 0, sizeof(UniformBufferObject), 0, &data);
        memcpy(data, &ubo, sizeof(UniformBufferObject));
        vkUnmapMemory(device, memoryUniformBuffers[currentImage]);
    }

    void createDescriptorPool() {
        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount = static_cast<uint32_t>(swapchainImages.size());
        
        VkDescriptorPoolCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        createInfo.poolSizeCount = 1;
        createInfo.pPoolSizes = &poolSize;
        createInfo.maxSets = static_cast<uint32_t>(swapchainImages.size());;

        vkCritical(vkCreateDescriptorPool(device, &createInfo, nullptr, &descriptorPool));
    }

    void createDescriptorSets() {
        std::vector<VkDescriptorSetLayout> layouts(swapchainImages.size(), descriptorSetLayout);
        VkDescriptorSetAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocateInfo.descriptorPool = descriptorPool;
        allocateInfo.descriptorSetCount = static_cast<uint32_t>(swapchainImages.size());
        allocateInfo.pSetLayouts = layouts.data();

        descriptorSets.resize(swapchainImages.size());
        vkCritical(vkAllocateDescriptorSets(device, &allocateInfo, descriptorSets.data()));

        for (size_t i = 0; i < swapchainImages.size(); i++) {
            VkDescriptorBufferInfo descriptorBufferInfo{};
            descriptorBufferInfo.buffer = uniformBuffers[i];
            descriptorBufferInfo.offset = 0;
            descriptorBufferInfo.range = sizeof(UniformBufferObject);
            
            VkWriteDescriptorSet descriptorSetWrite{};
            descriptorSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorSetWrite.dstSet = descriptorSets[i];
            descriptorSetWrite.dstBinding = 0;
            descriptorSetWrite.dstArrayElement = 0;
            descriptorSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorSetWrite.descriptorCount = 1;
            descriptorSetWrite.pBufferInfo = &descriptorBufferInfo;
            descriptorSetWrite.pImageInfo = nullptr; // Optional
            descriptorSetWrite.pTexelBufferView = nullptr; // Optional
            vkUpdateDescriptorSets(device, 1, &descriptorSetWrite, 0, nullptr);
        } 
    }

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags bufferUsageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
        //LOG("Max Memory Allocation Count: %u", maxMemoryAllocationCount);

        VkBufferCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        createInfo.size = size;
        createInfo.usage = bufferUsageFlags;
        createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        vkCritical(vkCreateBuffer(device, &createInfo, nullptr, &buffer));

        VkMemoryRequirements memoryRequirements;
        vkGetBufferMemoryRequirements(device, buffer, &memoryRequirements);

        VkMemoryAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocateInfo.allocationSize = memoryRequirements.size;
        allocateInfo.memoryTypeIndex = selectMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        vkCritical(vkAllocateMemory(device, &allocateInfo, nullptr, &bufferMemory));

        vkCritical(vkBindBufferMemory(device, buffer, bufferMemory, 0));
    }

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
        VkCommandBuffer commandBuffer;
        VkCommandBufferAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocateInfo.commandPool = commandPool;
        allocateInfo.commandBufferCount = 1;
        vkCritical(vkAllocateCommandBuffers(device, &allocateInfo, &commandBuffer));

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0; // Optional
        copyRegion.dstOffset = 0; // Optional
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        
        vkQueueWaitIdle(graphicsQueue);

        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }

    uint32_t selectMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags flags) {
        VkPhysicalDeviceMemoryProperties memoryProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

        for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
            if (typeFilter & (1 << i) && (memoryProperties.memoryTypes[i].propertyFlags & flags) == flags) {
                return i;
            }
        }

        throw std::runtime_error("Failed to find suitable memory type!\n");
    }

    void createCommandBuffers() {
        commandBuffers.resize(swapchainFramebuffers.size());
        VkCommandBufferAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocateInfo.commandPool = commandPool;
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocateInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
        vkCritical(vkAllocateCommandBuffers(device, &allocateInfo, commandBuffers.data()));

        for (size_t i = 0; i < commandBuffers.size(); i++) {
            VkCommandBufferBeginInfo commandBufferBeginInfo{};
            commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            commandBufferBeginInfo.flags = 0; // optional
            commandBufferBeginInfo.pInheritanceInfo = nullptr; // optional
            vkCritical(vkBeginCommandBuffer(commandBuffers[i], &commandBufferBeginInfo));
        
            VkRenderPassBeginInfo renderPassBeginInfo{};
            renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassBeginInfo.renderPass = renderPass;
            renderPassBeginInfo.framebuffer = swapchainFramebuffers[i];
            renderPassBeginInfo.renderArea.offset = {0, 0};
            renderPassBeginInfo.renderArea.extent = swapchainImageExtent;
            VkClearValue clearColor = {1.0f, 1.0f, 1.0f, 1.0f};
            renderPassBeginInfo.clearValueCount = 1;
            renderPassBeginInfo.pClearValues = &clearColor;

            // vk commands are predefined here
            vkCmdBeginRenderPass(commandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
            VkBuffer vertexBuffers[] = {vertexBuffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE);
            // vkCmdDraw(commandBuffers[i], static_cast<uint32_t>(vertices.size()), 1, 0, 0);
            vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[i], 0, nullptr);
            vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
            vkCmdEndRenderPass(commandBuffers[i]);
            
            vkCritical(vkEndCommandBuffer(commandBuffers[i]));
        }
    }

    void createSemaphores() {
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkCritical(vkCreateSemaphore(device, &createInfo, nullptr, &imageAvailableSemaphores[i]));
            vkCritical(vkCreateSemaphore(device, &createInfo, nullptr, &renderFinishedSemaphores[i]));
        }
    }

    void createFences() {
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
        VkFenceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // fences are initially signaled
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkCritical(vkCreateFence(device, &createInfo, nullptr, &inFlightFences[i]));
        }

        imagesInFlight.resize(swapchainImages.size(), VK_NULL_HANDLE);
    }

    void renderFrame() {
        static uint64_t frameCount = 0;
        if (frameCount++ % 90 == 0) {
            LOG(WHITE "Render Frame-%05llu\n" CLEAR, frameCount-1);
        }

        VkResult result;

        vkCritical(vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max()));

        uint32_t imageIndex;
        result = vkAcquireNextImageKHR(device, swapchain, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            LOG("Swapchain Out Of Date\n");        
            refreshSwapchain();
            framebufferResized = false;
            return;
        }
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to acquire the swapchain image!\n");
        } 

        updateUniformBuffer(imageIndex);

        // check if a previous frame is using this image (i.e. there is its fence to wait on)
        if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
            vkCritical(vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, std::numeric_limits<uint64_t>::max()));
        }

        // mark the image as now being in use by this frame
        imagesInFlight[imageIndex] = inFlightFences[currentFrame];
        
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
        VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        vkCritical(vkResetFences(device, 1, &inFlightFences[currentFrame]));
        vkCritical(vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]));
        
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        VkSwapchainKHR swapchains[] = {swapchain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapchains;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = nullptr; // optional

        // vkCritical(vkQueuePresentKHR(presentQueue, &presentInfo));
        result = vkQueuePresentKHR(presentQueue, &presentInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
            refreshSwapchain();
            framebufferResized = false;
            return;
        }
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to present the swapchain image.\n");
        }

        // vkCritical(vkQueueWaitIdle(presentQueue));

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }
};
/********************************************************************************************************************************/

/********************************************************************************************************************************/
int main()
{
    HelloTriangleApplication app;

    try {
        app.run();
    } catch (const std::exception &e) {
        LOG("%s", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
/********************************************************************************************************************************/