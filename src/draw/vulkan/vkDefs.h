#pragma once
#define VK_NO_PROTOTYPES
#include "include\vulkan.h"

namespace hax {

	namespace draw {

		namespace vk {

			typedef struct Functions {
				PFN_vkGetSwapchainImagesKHR pVkGetSwapchainImagesKHR;
				PFN_vkCreateCommandPool pVkCreateCommandPool;
				PFN_vkDestroyCommandPool pVkDestroyCommandPool;
				PFN_vkAllocateCommandBuffers pVkAllocateCommandBuffers;
				PFN_vkFreeCommandBuffers pVkFreeCommandBuffers;
				PFN_vkCreateSampler pVkCreateSampler;
				PFN_vkDestroySampler pVkDestroySampler;
				PFN_vkCreateDescriptorPool pVkCreateDescriptorPool;
				PFN_vkDestroyDescriptorPool pVkDestroyDescriptorPool;
				PFN_vkCreateRenderPass pVkCreateRenderPass;
				PFN_vkDestroyRenderPass pVkDestroyRenderPass;
				PFN_vkCreatePipelineLayout pVkCreatePipelineLayout;
				PFN_vkDestroyPipelineLayout pVkDestroyPipelineLayout;
				PFN_vkCreateDescriptorSetLayout pVkCreateDescriptorSetLayout;
				PFN_vkDestroyDescriptorSetLayout pVkDestroyDescriptorSetLayout;
				PFN_vkCreateShaderModule pVkCreateShaderModule;
				PFN_vkDestroyShaderModule pVkDestroyShaderModule;
				PFN_vkCreateGraphicsPipelines pVkCreateGraphicsPipelines;
				PFN_vkDestroyPipeline pVkDestroyPipeline;
				PFN_vkGetDeviceQueue pVkGetDeviceQueue;
				PFN_vkCreateBuffer pVkCreateBuffer;
				PFN_vkDestroyBuffer pVkDestroyBuffer;
				PFN_vkGetBufferMemoryRequirements pVkGetBufferMemoryRequirements;
				PFN_vkAllocateMemory pVkAllocateMemory;
				PFN_vkBindBufferMemory pVkBindBufferMemory;
				PFN_vkFreeMemory pVkFreeMemory;
				PFN_vkCreateImageView pVkCreateImageView;
				PFN_vkDestroyImageView pVkDestroyImageView;
				PFN_vkAllocateDescriptorSets  pVkAllocateDescriptorSets;
				PFN_vkUpdateDescriptorSets pVkUpdateDescriptorSets;
				PFN_vkFreeDescriptorSets pVkFreeDescriptorSets;
				PFN_vkResetCommandBuffer pVkResetCommandBuffer;
				PFN_vkBeginCommandBuffer pVkBeginCommandBuffer;
				PFN_vkEndCommandBuffer pVkEndCommandBuffer;
				PFN_vkCmdPipelineBarrier pVkCmdPipelineBarrier;
				PFN_vkCmdCopyBufferToImage pVkCmdCopyBufferToImage;
				PFN_vkQueueSubmit pVkQueueSubmit;
				PFN_vkQueueWaitIdle pVkQueueWaitIdle;
				PFN_vkCreateFramebuffer pVkCreateFramebuffer;
				PFN_vkDestroyFramebuffer pVkDestroyFramebuffer;
				PFN_vkCreateFence pVkCreateFence;
				PFN_vkDestroyFence pVkDestroyFence;
				PFN_vkWaitForFences pVkWaitForFences;
				PFN_vkResetFences pVkResetFences;
				PFN_vkCreateImage pVkCreateImage;
				PFN_vkDestroyImage pVkDestroyImage;
				PFN_vkBindImageMemory pVkBindImageMemory;
				PFN_vkGetImageMemoryRequirements pVkGetImageMemoryRequirements;
				PFN_vkCmdBeginRenderPass pVkCmdBeginRenderPass;
				PFN_vkCmdEndRenderPass pVkCmdEndRenderPass;
				PFN_vkMapMemory pVkMapMemory;
				PFN_vkUnmapMemory pVkUnmapMemory;
				PFN_vkFlushMappedMemoryRanges pVkFlushMappedMemoryRanges;
				PFN_vkCmdBindPipeline pVkCmdBindPipeline;
				PFN_vkCmdBindVertexBuffers pVkCmdBindVertexBuffers;
				PFN_vkCmdBindIndexBuffer pVkCmdBindIndexBuffer;
				PFN_vkCmdPushConstants pVkCmdPushConstants;
				PFN_vkCmdSetViewport pVkCmdSetViewport;
				PFN_vkCmdDrawIndexed pVkCmdDrawIndexed;
				PFN_vkCmdSetScissor pVkCmdSetScissor;
			}Functions;

		}

	}

}