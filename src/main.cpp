#include <iostream>
#include <sstream>

#include "image.h"
#include "renderer.h"

namespace {

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

}  // namespace

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

int main(int argc, char const* argv[]) {
    // Create window
    const std::string WIN_TITLE = "Face Landmark 3D";
    const uint32_t WIN_W = 600;
    const uint32_t WIN_H = 600;
    auto window = vkw::InitGLFWWindow(WIN_TITLE, WIN_W, WIN_H);

    // Create Renderer
    Renderer renderer(window);

    // Load mesh
    const std::string OBJ_FILENAME = "../data/Rhea_45_Small.obj";
    renderer.loadObj(OBJ_FILENAME);

    // Camera matrix
    const glm::mat4 model_mat = glm::scale(glm::vec3(1.00f));
    const glm::mat4 view_mat =
            glm::lookAt(glm::vec3(0.f, 10.f, 100.f), glm::vec3(0.f, 10.f, 0.f),
                        glm::vec3(0.f, 1.f, 0.f));
    const glm::mat4 proj_mat = glm::perspective(
            glm::radians(45.0f),
            static_cast<float>(WIN_W) / static_cast<float>(WIN_H), 0.1f,
            1000.0f);

    // Rendering loop
    while (!glfwWindowShouldClose(window.get())) {
        glm::mat4 mvp_mat = proj_mat * view_mat * model_mat;
        renderer.draw(mvp_mat);

        vkw::PrintFps();
        glfwPollEvents();
    }

    return 0;
}
