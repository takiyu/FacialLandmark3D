#include <iostream>
#include <sstream>

#include "image.h"
#include "landmarker.h"
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
    (void)argc, (void)argv;

    // Create Vulkan window
    const std::string WIN_TITLE = "Face Landmark 3D";
    const uint32_t WIN_W = 600;
    const uint32_t WIN_H = 600;
    auto window = vkw::InitGLFWWindow(WIN_TITLE, WIN_W, WIN_H);
    glfwHideWindow(window.get());

    // Create Renderer
    Renderer renderer(window);
    // Create Landmark detector
    const std::string& PREDICTOR_PATH =
            // "../data/shape_predictor_68_face_landmarks.dat";
            "../data/shape_predictor_5_face_landmarks.dat";
    LandmarkDetector landmarker(PREDICTOR_PATH);

    // Load mesh
    const std::string OBJ_FILENAME = "../data/Rhea_45_Small.obj";
    renderer.loadObj(OBJ_FILENAME);
    const auto& mesh = renderer.getMesh();

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

    // Rendering and Landmarking loop
    while (!glfwWindowShouldClose(window.get())) {
        // Render
        auto&& col_pos_imgs = renderer.draw(mvp_mat);
        auto&& col_img = std::get<0>(col_pos_imgs);
        auto&& pos_img = std::get<1>(col_pos_imgs);

        // Detect landmarks
        const auto& lmks = landmarker.detect(col_img, pos_img, mesh);

        if (!lmks.empty()) {
            // Print result
            for (uint32_t lmk_idx = 0; lmk_idx < lmks.size(); lmk_idx++) {
                const auto& lmk = lmks[lmk_idx];
                const auto& lmk_2d = lmk.lmk_2d;
                const auto& lmk_3d = lmk.lmk_3d;
                const auto& vtx_idx = lmk.vtx_idx;

                std::cout << lmk_idx << ": " << std::endl;
                std::cout << "  2d: " << lmk_2d.x << " " << lmk_2d.y
                          << std::endl;
                std::cout << "  3d: " << lmk_3d.x << " " << lmk_3d.y << " "
                          << lmk_3d.z << std::endl;
                std::cout << "  vertex index: " << vtx_idx << std::endl;
            }

            // Show Dlib window (debug)
            landmarker.show();
        }

        glfwPollEvents();
    }

    return 0;
}
