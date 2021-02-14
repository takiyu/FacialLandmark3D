#ifndef RENDERER_H_20210212
#define RENDERER_H_20210212
#include <vkw/vkw.h>

#include "image.h"

BEGIN_VKW_SUPPRESS_WARNING
#include <glm/geometric.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
END_VKW_SUPPRESS_WARNING

// -----------------------------------------------------------------------------
// ------------------------------- 3D Structures -------------------------------
// -----------------------------------------------------------------------------
struct Vertex {
    glm::vec3 pos;   // Position
    glm::vec2 uv;    // Texture Coordinate
    uint32_t vtx_idx;  // Vertex Index
};

struct Mesh {
    std::vector<Vertex> vertices;  // Flatten vertices over all meshes
    FloatImage color_tex;          // Color texture (Unlit Shading)
};

// -----------------------------------------------------------------------------
// ----------------------- 3D Renderer by Vulkan Backend -----------------------
// -----------------------------------------------------------------------------
class Renderer {
public:
    Renderer(const vkw::WindowPtr& window);
    void loadObj(const std::string& filename);
    const Mesh& getMesh() const;
    std::tuple<FloatImage, FloatImage> draw(const glm::mat4& mvp_mat);

private:
    void init();

    Mesh m_mesh;
    bool m_inited = false;

    vkw::WindowPtr m_window;
    vk::UniqueInstance m_instance;
    vk::PhysicalDevice m_physical_device;
    vk::UniqueSurfaceKHR m_surface;
    vk::Format m_surface_format;
    uint32_t m_queue_family_idx;
    vk::UniqueDevice m_device;
    vkw::SwapchainPackPtr m_swapchain;
    vk::Queue m_queue;
    vkw::ImagePackPtr m_color_img;
    vkw::ImagePackPtr m_pos_img;
    vkw::ImagePackPtr m_depth_img;
    vkw::BufferPackPtr m_uniform_buf;
    vkw::TexturePackPtr m_color_tex;
    vkw::DescSetPackPtr m_desc_set;
    vkw::WriteDescSetPackPtr m_write_desc_set;
    vkw::RenderPassPackPtr m_render_pass;
    std::vector<vkw::FrameBufferPackPtr> m_framebuffers;
    vkw::ShaderModulePackPtr m_vert_shader;
    vkw::ShaderModulePackPtr m_frag_shader;
    vkw::BufferPackPtr m_vtx_buf;
    vkw::PipelinePackPtr m_pipeline;
    vkw::CommandBuffersPackPtr m_cmd_bufs;
    vkw::BufferPackPtr m_color_recv_buf;
    vkw::BufferPackPtr m_pos_recv_buf;
};

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

#endif /* end of include guard */
