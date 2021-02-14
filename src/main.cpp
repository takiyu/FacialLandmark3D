#include <iostream>
#include <sstream>

#include "image.h"
#include "renderer.h"
#include "landmarker.h"

namespace {

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

}  // namespace

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

int main(int argc, char const* argv[]) {
    (void)argc, (void)argv;

    // Create Vulkan window
    const std::string WIN_TITLE = "Face Landmark 3D";
    const uint32_t WIN_W = 600;
    const uint32_t WIN_H = 600;
    auto window = vkw::InitGLFWWindow(WIN_TITLE, WIN_W, WIN_H);

    // Create Renderer & Landmarker
    Renderer renderer(window);
    Landmarker landmarker;

    // Load mesh
    const std::string OBJ_FILENAME = "../data/Rhea_45_Small.obj";
    renderer.loadObj(OBJ_FILENAME);

    // Camera matrix
    const glm::mat4 MODEL_MAT = glm::scale(glm::vec3(1.00f));
    const glm::mat4 VIEW_MAT =
            glm::lookAt(glm::vec3(0.f, 20.f, 50.f), glm::vec3(0.f, 20.f, 0.f),
                        glm::vec3(0.f, 1.f, 0.f));
    const glm::mat4 PROJ_MAT = glm::perspective(
            glm::radians(45.f),
            static_cast<float>(WIN_W) / static_cast<float>(WIN_H), 0.1f,
            1000.f);
    glm::mat4 mvp_mat = PROJ_MAT * VIEW_MAT * MODEL_MAT;

    // View several frames
    for (uint32_t n_loop = 0; n_loop < 3; n_loop++) {
        renderer.draw(mvp_mat);
//         vkw::PrintFps();
//         glfwPollEvents();
    }

    // Rendering and Landmarking loop
    while (!glfwWindowShouldClose(window.get())) {
        auto&& [col_img, pos_img] = renderer.draw(mvp_mat);
        landmarker.detect(col_img);
        glfwPollEvents();
    }

    return 0;
}
