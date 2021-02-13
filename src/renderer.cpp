#include "renderer.h"

BEGIN_VKW_SUPPRESS_WARNING
#include <tinyobjloader/tiny_obj_loader.h>
END_VKW_SUPPRESS_WARNING

namespace {

// -----------------------------------------------------------------------------
// ----------------------------------- Mesh ------------------------------------
// -----------------------------------------------------------------------------
std::string ExtractDirname(const std::string& path) {
    return path.substr(0, path.find_last_of('/') + 1);
}

Mesh LoadObj(const std::string& filename) {
    const std::string& dirname = ExtractDirname(filename);

    // Load with tiny obj
    tinyobj::ObjReader obj_reader;
    const bool ret = obj_reader.ParseFromFile(filename);
    if (!ret) {
        std::stringstream ss;
        ss << "Error:" << obj_reader.Error() << std::endl;
        ss << "Warning:" << obj_reader.Warning() << std::endl;
        throw std::runtime_error(ss.str());
    }
    const std::vector<tinyobj::shape_t>& tiny_shapes = obj_reader.GetShapes();
    const tinyobj::attrib_t& tiny_attrib = obj_reader.GetAttrib();
    const std::vector<tinyobj::real_t>& tiny_vertices = tiny_attrib.vertices;
    const std::vector<tinyobj::real_t>& tiny_texcoords = tiny_attrib.texcoords;

    // Parse to mesh structure
    Mesh ret_mesh;
    for (const tinyobj::shape_t& tiny_shape : tiny_shapes) {
        const tinyobj::mesh_t& tiny_mesh = tiny_shape.mesh;
        for (const tinyobj::index_t& tiny_idx : tiny_mesh.indices) {
            // Parse one vertex
            Vertex ret_vtx = {};
            if (0 <= tiny_idx.vertex_index) {
                // Vertex
                auto idx0 = static_cast<uint32_t>(tiny_idx.vertex_index * 3);
                ret_vtx.pos = {tiny_vertices[idx0 + 0], tiny_vertices[idx0 + 1],
                               tiny_vertices[idx0 + 2]};
            }
            if (0 <= tiny_idx.texcoord_index) {
                // Texture coordinate
                auto idx0 = static_cast<uint32_t>(tiny_idx.texcoord_index * 2);
                ret_vtx.uv = {tiny_texcoords[idx0 + 0],
                              tiny_texcoords[idx0 + 1]};
            }
            // Register
            ret_mesh.vertices.push_back(std::move(ret_vtx));
        }
    }

    // Load textures
    const auto& tiny_mats = obj_reader.GetMaterials();
    if (tiny_mats.empty()) {
        throw std::runtime_error("Empty texture is not supported");
    }
    if (1 < tiny_mats.size()) {
        throw std::runtime_error("Multiple textures are not supported");
    }
    // Supports only 1 materials
    const tinyobj::material_t tiny_mat = tiny_mats[0];
    // Load color texture
    ret_mesh.color_tex = LoadImage(dirname + tiny_mat.diffuse_texname, 4);

    return ret_mesh;
}

// -----------------------------------------------------------------------------
// ---------------------------------- Shaders ----------------------------------
// -----------------------------------------------------------------------------
const std::string VERT_SOURCE = R"(
#version 460

layout(binding = 0) uniform UniformBuffer {
    mat4 mvp_mat;
} uniform_buf;

layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 uv;

layout (location = 0) out vec3 vtx_pos;
layout (location = 1) out vec2 vtx_uv;

void main() {
    gl_Position = uniform_buf.mvp_mat * vec4(pos, 1.0);
    vtx_pos = pos;
    vtx_uv = uv;
}
)";

const std::string FRAG_SOURCE = R"(
#version 460

layout (binding = 1) uniform sampler2D tex;

layout (location = 0) in vec3 vtx_pos;
layout (location = 1) in vec2 vtx_uv;

layout (location = 0) out vec4 frag_color;
layout (location = 1) out vec4 frag_pos;

void main() {
    vec2 uv = vec2(vtx_uv.x, 1.0 - vtx_uv.y);  // Y-flip
    frag_color = texture(tex, uv);
    frag_pos = vec4(vtx_pos, 1.0);
}
)";

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
}  // namespace

// -----------------------------------------------------------------------------
// ----------------------- 3D Renderer by Vulkan Backend -----------------------
// -----------------------------------------------------------------------------
Renderer::Renderer(const vkw::WindowPtr& window) {
    m_window = window;
}

void Renderer::loadObj(const std::string& filename) {
    m_mesh = LoadObj(filename);
    m_inited = false;
}

std::tuple<FloatImage, FloatImage> Renderer::draw(const glm::mat4& mvp_mat) {
    // Initialize once
    if (!m_inited) {
        m_inited = true;
        init();
    }

    // Send matrix to uniform buffer
    const glm::mat4 CLIP_MAT = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f,
                                0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f,
                                0.0f, 0.0f, 0.5f, 1.0f};
    glm::mat4 mvpc_mat = CLIP_MAT * mvp_mat;
    vkw::SendToDevice(m_device, m_uniform_buf, &mvpc_mat[0], sizeof(glm::mat4));

    // Acquire screen frame
    auto img_acquired_semaphore = vkw::CreateSemaphore(m_device);
    uint32_t curr_img_idx = vkw::AcquireNextImage(
            m_device, m_swapchain, img_acquired_semaphore, nullptr);

    // Draw
    auto draw_fence = vkw::CreateFence(m_device);
    vkw::QueueSubmit(m_queue, m_cmd_bufs->cmd_bufs[curr_img_idx], draw_fence,
                     {{img_acquired_semaphore,
                       vk::PipelineStageFlagBits::eColorAttachmentOutput}},
                     {});
    vkw::QueuePresent(m_queue, m_swapchain, curr_img_idx);
    vkw::WaitForFences(m_device, {draw_fence});

    return {};
}

void Renderer::init() {
    // Create instance
    const bool DISPLAY_ENABLE = true;
    const bool DEBUG_ENABLE = true;
    m_instance =
            vkw::CreateInstance("", 1, "", 0, DEBUG_ENABLE, DISPLAY_ENABLE);
    // Get a physical_device
    m_physical_device = vkw::GetFirstPhysicalDevice(m_instance);
    // Create surface
    m_surface = vkw::CreateSurface(m_instance, m_window);
    m_surface_format = vkw::GetSurfaceFormat(m_physical_device, m_surface);
    // Select queue family
    m_queue_family_idx =
            vkw::GetGraphicPresentQueueFamilyIdx(m_physical_device, m_surface);
    // Create device
    const uint32_t N_QUEUES = 1;
    m_device = vkw::CreateDevice(m_queue_family_idx, m_physical_device,
                                 N_QUEUES, DISPLAY_ENABLE);
    // Create swapchain
    m_swapchain =
            vkw::CreateSwapchainPack(m_physical_device, m_device, m_surface);
    // Get queue
    m_queue = vkw::GetQueue(m_device, m_queue_family_idx, 0);

    // Create depth buffer
    const auto DEPTH_FORMAT = vk::Format::eD32Sfloat;
    m_depth_img = vkw::CreateImagePack(
            m_physical_device, m_device, DEPTH_FORMAT, m_swapchain->size, 1,
            vk::ImageUsageFlagBits::eDepthStencilAttachment, {}, true,
            vk::ImageAspectFlagBits::eDepth);

    // Create uniform buffer
    m_uniform_buf = vkw::CreateBufferPack(
            m_physical_device, m_device, sizeof(glm::mat4),
            vk::BufferUsageFlagBits::eUniformBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible |
                    vk::MemoryPropertyFlagBits::eHostCoherent);
    // Create color texture
    m_color_tex = vkw::CreateTexturePack(
            vkw::CreateImagePack(
                    m_physical_device, m_device,
                    vk::Format::eR32G32B32A32Sfloat,
                    {m_mesh.color_tex.width, m_mesh.color_tex.height}, 1,
                    vk::ImageUsageFlagBits::eSampled |
                            vk::ImageUsageFlagBits::eTransferDst,
                    {}, true,  // tiling
                    vk::ImageAspectFlagBits::eColor),
            m_device);

    // Create descriptor set for uniform buffer and texture
    m_desc_set = vkw::CreateDescriptorSetPack(
            m_device, {{vk::DescriptorType::eUniformBufferDynamic, 1,
                        vk::ShaderStageFlagBits::eVertex},
                       {vk::DescriptorType::eCombinedImageSampler, 1,
                        vk::ShaderStageFlagBits::eFragment}});
    // Bind descriptor set with actual buffer
    m_write_desc_set = vkw::CreateWriteDescSetPack();
    vkw::AddWriteDescSet(m_write_desc_set, m_desc_set, 0, {m_uniform_buf});
    vkw::AddWriteDescSet(m_write_desc_set, m_desc_set, 1,
                         {m_color_tex},  // layout is still undefined.
                         {vk::ImageLayout::eShaderReadOnlyOptimal});
    vkw::UpdateDescriptorSets(m_device, m_write_desc_set);

    // Create render pass
    m_render_pass = vkw::CreateRenderPassPack();
    // Add color attachment
    vkw::AddAttachientDesc(
            m_render_pass, m_surface_format, vk::AttachmentLoadOp::eClear,
            vk::AttachmentStoreOp::eStore, vk::ImageLayout::ePresentSrcKHR);
    // Add depth attachment
    vkw::AddAttachientDesc(m_render_pass, DEPTH_FORMAT,
                           vk::AttachmentLoadOp::eClear,
                           vk::AttachmentStoreOp::eDontCare,
                           vk::ImageLayout::eDepthStencilAttachmentOptimal);
    // Add subpass
    vkw::AddSubpassDesc(m_render_pass, {},
                        {{0, vk::ImageLayout::eColorAttachmentOptimal}},
                        {1, vk::ImageLayout::eDepthStencilAttachmentOptimal});
    // Create render pass instance
    vkw::UpdateRenderPass(m_device, m_render_pass);

    // Create frame buffers for swapchain images
    m_framebuffers = vkw::CreateFrameBuffers(
            m_device, m_render_pass, {nullptr, m_depth_img}, m_swapchain);

    // Compile shaders
    vkw::GLSLCompiler glsl_compiler;
    m_vert_shader = glsl_compiler.compileFromString(
            m_device, VERT_SOURCE, vk::ShaderStageFlagBits::eVertex);
    m_frag_shader = glsl_compiler.compileFromString(
            m_device, FRAG_SOURCE, vk::ShaderStageFlagBits::eFragment);

    // Create vertex buffer
    size_t vertex_buf_size = m_mesh.vertices.size() * sizeof(Vertex);
    m_vtx_buf = vkw::CreateBufferPack(
            m_physical_device, m_device, vertex_buf_size,
            vk::BufferUsageFlagBits::eVertexBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible |
                    vk::MemoryPropertyFlagBits::eHostCoherent);
    // Send vertices to GPU
    vkw::SendToDevice(m_device, m_vtx_buf, m_mesh.vertices.data(),
                      vertex_buf_size);

    // Create pipeline
    vkw::PipelineInfo pipeline_info;
    pipeline_info.color_blend_infos.resize(1);
    pipeline_info.face_culling = vk::CullModeFlagBits::eNone;
    m_pipeline = vkw::CreateGraphicsPipeline(
            m_device, {m_vert_shader, m_frag_shader},
            {{0, sizeof(Vertex), vk::VertexInputRate::eVertex}},
            {{0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos)},
             {1, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, uv)}},
            pipeline_info, {m_desc_set}, m_render_pass);

    // Create command buffers
    uint32_t n_cmd_bufs = static_cast<uint32_t>(m_framebuffers.size());
    m_cmd_bufs = vkw::CreateCommandBuffersPack(m_device, m_queue_family_idx,
                                               n_cmd_bufs);

    // Send texture to GPU
    uint64_t tex_n_bytes = m_mesh.color_tex.pixels.size() * sizeof(float);
    auto trans_buf_pack = vkw::CreateBufferPack(  // Create temporal buffer
            m_physical_device, m_device, tex_n_bytes,
            vk::BufferUsageFlagBits::eTransferSrc, vkw::HOST_VISIB_COHER_PROPS);
    vkw::SendToDevice(m_device, trans_buf_pack,  // Send from CPU to buf
                      m_mesh.color_tex.pixels.data(), tex_n_bytes);
    auto& send_cmd_buf = m_cmd_bufs->cmd_bufs[0];  // Use 1st command buffer
    vkw::BeginCommand(send_cmd_buf);
    vkw::CopyBufferToImage(send_cmd_buf,  // Stack sending cmd from buf to img
                           trans_buf_pack, m_color_tex->img_pack);
    vkw::EndCommand(send_cmd_buf);
    auto send_fence = vkw::CreateFence(m_device);
    vkw::QueueSubmit(m_queue, send_cmd_buf, send_fence, {}, {});  // Execute
    vkw::WaitForFences(m_device, {send_fence});

    // Stack draw command
    for (uint32_t cmd_idx = 0; cmd_idx < n_cmd_bufs; cmd_idx++) {
        auto& cmd_buf = m_cmd_bufs->cmd_bufs[cmd_idx];

        vkw::ResetCommand(cmd_buf);
        vkw::BeginCommand(cmd_buf);
        const std::array<float, 4> clear_color = {0.f, 0.f, 0.f, 1.f};
        vkw::CmdBeginRenderPass(cmd_buf, m_render_pass, m_framebuffers[cmd_idx],
                                {
                                        vk::ClearColorValue(clear_color),
                                        vk::ClearDepthStencilValue(1.0f, 0),
                                });
        vkw::CmdBindPipeline(cmd_buf, m_pipeline);
        const std::vector<uint32_t> dynamic_offsets = {0};
        vkw::CmdBindDescSets(cmd_buf, m_pipeline, {m_desc_set},
                             dynamic_offsets);
        vkw::CmdBindVertexBuffers(cmd_buf, 0, {m_vtx_buf});
        vkw::CmdSetViewport(cmd_buf, m_swapchain->size);
        vkw::CmdSetScissor(cmd_buf, m_swapchain->size);
        vkw::CmdDraw(cmd_buf, m_mesh.vertices.size());
        vkw::CmdEndRenderPass(cmd_buf);
        vkw::EndCommand(cmd_buf);
    }
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
