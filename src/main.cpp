#include <vkw/vkw.h>
#include <vkw/warning_suppressor.h>

BEGIN_VKW_SUPPRESS_WARNING
#include <stb/stb_image.h>
#include <tinyobjloader/tiny_obj_loader.h>

#include <glm/geometric.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
END_VKW_SUPPRESS_WARNING

#include <iostream>
#include <sstream>

namespace {

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
const std::string VERT_SOURCE = R"(
#version 460

layout(binding = 0) uniform UniformBuffer {
    mat4 mvp_mat;
} uniform_buf;

layout (location = 0) in vec3 pos;
layout (location = 1) in float id;
layout (location = 2) in vec2 uv;

layout (location = 0) out vec2 vtx_uv;
layout (location = 1) out float vtx_id;

void main() {
    gl_Position = uniform_buf.mvp_mat * vec4(pos, 1.0);
    vtx_uv = uv;
    vtx_id = id;
}
)";

const std::string GEOM_SOURCE = R"(
#version 460

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

layout (location = 0) in vec2 vtx_uv[3];
layout (location = 1) in float vtx_id[3];

layout (location = 0) out vec2 geom_uv;
layout (location = 1) out vec3 geom_ids;
layout (location = 2) out vec3 geom_bary;

void main(void) {
    for (int i = 0; i < 3; i++) {
        gl_Position = gl_in[i].gl_Position;
        geom_uv = vtx_uv[i];
        geom_ids = vec3(vtx_id[0], vtx_id[1], vtx_id[2]);
        geom_bary = vec3(0.0);
        geom_bary[i] = 1.0;
        EmitVertex();
    }
    EndPrimitive();
}
)";

const std::string FRAG_SOURCE = R"(
#version 460

layout (binding = 1) uniform sampler2D tex;

layout (location = 0) in vec2 geom_uv;
layout (location = 1) in vec3 geom_ids;
layout (location = 2) in vec3 geom_bary;

layout (location = 0) out vec4 frag_color;

int ObtainArgMin(vec3 elems) {
    float min_elem = elems[0];
    int min_idx = 0;
    for (int i = 1; i < 3; i++) {
        if (elems[i] < min_elem) {
            min_elem = elems[i];
            min_idx = i;
        }
    }
    return min_idx;
}

void main() {
    int closest_idx = ObtainArgMin(geom_bary);
    float id = geom_ids[closest_idx];
    frag_color = vec4(vec3(id) / 10000, 1.0);
    //frag_color = texture(tex, vec2(geom_uv.x, 1.0 - geom_uv.y));
}
)";

struct Vertex {
    glm::vec3 pos;  // Position
    float id;    // Vertex ID
    glm::vec2 uv;   // Texture Coordinate
};

struct Mesh {
    std::vector<Vertex> vertices;

    std::vector<float> color_tex;  // Color texture (Unlit Shading)
    uint32_t color_tex_w = 0;
    uint32_t color_tex_h = 0;
};

std::string ExtractDirname(const std::string& path) {
    return path.substr(0, path.find_last_of('/') + 1);
}

std::vector<float> LoadTexture(const std::string& filename, const uint32_t n_ch,
                               uint32_t* w, uint32_t* h) {
    int tmp_w, tmp_h, dummy_c;
    uint8_t* data = stbi_load(filename.c_str(), &tmp_w, &tmp_h, &dummy_c,
                              static_cast<int>(n_ch));
    (*w) = static_cast<uint32_t>(tmp_w);
    (*h) = static_cast<uint32_t>(tmp_h);

    std::vector<uint8_t> ret_tex_u8(data, data + (*w) * (*h) * n_ch);

    std::vector<float> ret_tex;
    for (auto&& a : ret_tex_u8) {
        ret_tex.push_back(a / 255.f);
    }

    stbi_image_free(data);

    return ret_tex;
}

static Mesh LoadObj(const std::string& filename) {
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
                // Vertex ID
                ret_vtx.id = static_cast<float>(tiny_idx.vertex_index) + 0.5f;
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
    if (!tiny_mats.empty()) {
        // Supports only 1 materials
        const tinyobj::material_t tiny_mat = tiny_mats[0];
        // Load color texture
        ret_mesh.color_tex =
                LoadTexture(dirname + tiny_mat.diffuse_texname, 4,
                            &ret_mesh.color_tex_w, &ret_mesh.color_tex_h);
    }

    return ret_mesh;
}

}  // namespace

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

int main(int argc, char const* argv[]) {
    // Load mesh
    const std::string OBJ_FILENAME = "../data/Rhea_45_Small.obj";
    const Mesh mesh = LoadObj(OBJ_FILENAME);

    const std::string& TITLE = "Face Landmark 3D";
    const bool DISPLAY_ENABLE = true;
    const bool DEBUG_ENABLE = true;
    const uint32_t N_QUEUES = 1;

    auto window = vkw::InitGLFWWindow(TITLE, 600, 600);
    // Create instance
    auto instance =
            vkw::CreateInstance(TITLE, 1, "", 0, DEBUG_ENABLE, DISPLAY_ENABLE);
    // Get a physical_device
    auto physical_device = vkw::GetFirstPhysicalDevice(instance);

    // Set features
    auto features = vkw::GetPhysicalFeatures(physical_device);
    features->geometryShader = true;

    // Create surface
    auto surface = vkw::CreateSurface(instance, window);
    auto surface_format = vkw::GetSurfaceFormat(physical_device, surface);

    // Select queue family
    uint32_t queue_family_idx =
            vkw::GetGraphicPresentQueueFamilyIdx(physical_device, surface);
    // Create device
    auto device = vkw::CreateDevice(queue_family_idx, physical_device, N_QUEUES,
                                    DISPLAY_ENABLE, features);

    // Create swapchain
    auto swapchain_pack =
            vkw::CreateSwapchainPack(physical_device, device, surface);

    // Get queues
    std::vector<vk::Queue> queues;
    queues.reserve(N_QUEUES);
    for (uint32_t i = 0; i < N_QUEUES; i++) {
        queues.push_back(vkw::GetQueue(device, queue_family_idx, i));
    }

    // Create depth buffer
    const auto depth_format = vk::Format::eD32Sfloat;
    auto depth_img_pack = vkw::CreateImagePack(
            physical_device, device, depth_format, swapchain_pack->size, 1,
            vk::ImageUsageFlagBits::eDepthStencilAttachment, {},
            true,  // tiling
            vk::ImageAspectFlagBits::eDepth);

    // Create uniform buffer
    auto uniform_buf_pack = vkw::CreateBufferPack(
            physical_device, device, sizeof(glm::mat4),
            vk::BufferUsageFlagBits::eUniformBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible |
                    vk::MemoryPropertyFlagBits::eHostCoherent);

    // Create color texture
    auto color_img_pack = vkw::CreateImagePack(
            physical_device, device, vk::Format::eR32G32B32A32Sfloat,
            {mesh.color_tex_w, mesh.color_tex_h}, 1,
            vk::ImageUsageFlagBits::eSampled |
                    vk::ImageUsageFlagBits::eTransferDst,
            {}, true,  // tiling
            vk::ImageAspectFlagBits::eColor);
    auto color_tex_pack = vkw::CreateTexturePack(color_img_pack, device);

    // Create descriptor set for uniform buffer and texture
    auto desc_set_pack = vkw::CreateDescriptorSetPack(
            device, {{vk::DescriptorType::eUniformBufferDynamic, 1,
                      vk::ShaderStageFlagBits::eVertex},
                     {vk::DescriptorType::eCombinedImageSampler, 1,
                      vk::ShaderStageFlagBits::eFragment}});

    // Bind descriptor set with actual buffer
    auto write_desc_set_pack = vkw::CreateWriteDescSetPack();
    vkw::AddWriteDescSet(write_desc_set_pack, desc_set_pack, 0,
                         {uniform_buf_pack});
    vkw::AddWriteDescSet(write_desc_set_pack, desc_set_pack, 1,
                         {color_tex_pack},  // layout is still undefined.
                         {vk::ImageLayout::eShaderReadOnlyOptimal});
    vkw::UpdateDescriptorSets(device, write_desc_set_pack);

    // Create render pass
    auto render_pass_pack = vkw::CreateRenderPassPack();
    // Add color attachment
    vkw::AddAttachientDesc(
            render_pass_pack, surface_format, vk::AttachmentLoadOp::eClear,
            vk::AttachmentStoreOp::eStore, vk::ImageLayout::ePresentSrcKHR);
    // Add depth attachment
    vkw::AddAttachientDesc(render_pass_pack, depth_format,
                           vk::AttachmentLoadOp::eClear,
                           vk::AttachmentStoreOp::eDontCare,
                           vk::ImageLayout::eDepthStencilAttachmentOptimal);
    // Add subpass
    vkw::AddSubpassDesc(render_pass_pack,
                        {
                                // No input attachments
                        },
                        {
                                {0, vk::ImageLayout::eColorAttachmentOptimal},
                        },
                        {1, vk::ImageLayout::eDepthStencilAttachmentOptimal});
    // Create render pass instance
    vkw::UpdateRenderPass(device, render_pass_pack);

    // Create frame buffers for swapchain images
    auto frame_buffer_packs =
            vkw::CreateFrameBuffers(device, render_pass_pack,
                                    {nullptr, depth_img_pack}, swapchain_pack);

    // Compile shaders
    vkw::GLSLCompiler glsl_compiler;
    auto vert_shader_module_pack = glsl_compiler.compileFromString(
            device, VERT_SOURCE, vk::ShaderStageFlagBits::eVertex);
    auto geom_shader_module_pack = glsl_compiler.compileFromString(
            device, GEOM_SOURCE, vk::ShaderStageFlagBits::eGeometry);
    auto frag_shader_module_pack = glsl_compiler.compileFromString(
            device, FRAG_SOURCE, vk::ShaderStageFlagBits::eFragment);

    // Create vertex buffer
    const size_t vertex_buf_size = mesh.vertices.size() * sizeof(Vertex);
    auto vertex_buf_pack = vkw::CreateBufferPack(
            physical_device, device, vertex_buf_size,
            vk::BufferUsageFlagBits::eVertexBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible |
                    vk::MemoryPropertyFlagBits::eHostCoherent);
    // Send vertices to GPU
    vkw::SendToDevice(device, vertex_buf_pack, mesh.vertices.data(),
                      vertex_buf_size);

    // Create pipeline
    vkw::PipelineInfo pipeline_info;
    pipeline_info.color_blend_infos.resize(1);
    pipeline_info.face_culling = vk::CullModeFlagBits::eNone;
    auto pipeline_pack = vkw::CreateGraphicsPipeline(
            device,
            {vert_shader_module_pack, geom_shader_module_pack,
             frag_shader_module_pack},
            {{0, sizeof(Vertex), vk::VertexInputRate::eVertex}},
            {{0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos)},
             {1, 0, vk::Format::eR32Sfloat, offsetof(Vertex, id)},
             {2, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, uv)}},
            pipeline_info, {desc_set_pack}, render_pass_pack);

    const uint32_t n_cmd_bufs = 1;
    auto cmd_bufs_pack =
            vkw::CreateCommandBuffersPack(device, queue_family_idx, n_cmd_bufs);
    auto& cmd_buf = cmd_bufs_pack->cmd_bufs[0];

    // -----------------
    // Create source transfer buffer
    const uint64_t tex_n_bytes =
            mesh.color_tex.size() * sizeof(mesh.color_tex[0]);
    vkw::BufferPackPtr trans_buf_pack = vkw::CreateBufferPack(
            physical_device, device, tex_n_bytes,
            vk::BufferUsageFlagBits::eTransferSrc, vkw::HOST_VISIB_COHER_PROPS);

    // ------------------
    glm::mat4 model_mat = glm::scale(glm::vec3(1.00f));
    const glm::mat4 view_mat = glm::lookAt(glm::vec3(0.0f, 10.0f, 100.0f),
                                           glm::vec3(0.0f, 10.0f, 0.0f),
                                           glm::vec3(0.0f, 1.0f, 0.0f));
    const float aspect = static_cast<float>(swapchain_pack->size.width) /
                         static_cast<float>(swapchain_pack->size.height);
    const glm::mat4 proj_mat =
            glm::perspective(glm::radians(45.0f), aspect, 0.1f, 1000.0f);
    // vulkan clip space has inverted y and half z !
    const glm::mat4 clip_mat = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f,
                                0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f,
                                0.0f, 0.0f, 0.5f, 1.0f};

    bool is_col_tex_sent = false;
    while (!glfwWindowShouldClose(window.get())) {
        // model_mat = glm::rotate(0.01f, glm::vec3(0.f, 1.f, 0.f)) * model_mat;
        glm::mat4 mvpc_mat = clip_mat * proj_mat * view_mat * model_mat;
        vkw::SendToDevice(device, uniform_buf_pack, &mvpc_mat[0],
                          sizeof(mvpc_mat));

        vkw::ResetCommand(cmd_buf);

        auto img_acquired_semaphore = vkw::CreateSemaphore(device);
        uint32_t curr_img_idx = vkw::AcquireNextImage(
                device, swapchain_pack, img_acquired_semaphore, nullptr);

        vkw::BeginCommand(cmd_buf);

        // Send color texture to GPU
        if (!is_col_tex_sent) {
            is_col_tex_sent = true;

            // Send to buffer
            vkw::SendToDevice(device, trans_buf_pack, mesh.color_tex.data(),
                              tex_n_bytes);
            // Send buffer to image
            vkw::CopyBufferToImage(cmd_buf, trans_buf_pack,
                                   color_tex_pack->img_pack);
        }

        const std::array<float, 4> clear_color = {0.2f, 0.2f, 0.2f, 1.0f};
        vkw::CmdBeginRenderPass(cmd_buf, render_pass_pack,
                                frame_buffer_packs[curr_img_idx],
                                {
                                        vk::ClearColorValue(clear_color),
                                        vk::ClearDepthStencilValue(1.0f, 0),
                                });

        vkw::CmdBindPipeline(cmd_buf, pipeline_pack);

        const std::vector<uint32_t> dynamic_offsets = {0};
        vkw::CmdBindDescSets(cmd_buf, pipeline_pack, {desc_set_pack},
                             dynamic_offsets);

        vkw::CmdBindVertexBuffers(cmd_buf, 0, {vertex_buf_pack});

        vkw::CmdSetViewport(cmd_buf, swapchain_pack->size);
        vkw::CmdSetScissor(cmd_buf, swapchain_pack->size);

        const uint32_t n_instances = 1;
        vkw::CmdDraw(cmd_buf, mesh.vertices.size(), n_instances);

        // vkw::CmdNextSubPass(cmd_buf);
        vkw::CmdEndRenderPass(cmd_buf);

        vkw::EndCommand(cmd_buf);

        auto draw_fence = vkw::CreateFence(device);

        vkw::QueueSubmit(queues[0], cmd_buf, draw_fence,
                         {{img_acquired_semaphore,
                           vk::PipelineStageFlagBits::eColorAttachmentOutput}},
                         {});

        vkw::QueuePresent(queues[0], swapchain_pack, curr_img_idx);

        vkw::WaitForFences(device, {draw_fence});

        vkw::PrintFps();

        glfwPollEvents();
    }

    return 0;
}
