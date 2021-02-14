#include "landmarker.h"

#include <limits>

namespace {
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
inline uint8_t CastUint(float v) {
    return static_cast<uint8_t>(std::min(std::max(v * 255.f, 0.f), 255.f));
}

dlib::array2d<dlib::rgb_pixel> CastToDlibImg(const FloatImage& img) {
    // Allocate
    dlib::array2d<dlib::rgb_pixel> dlib_img;
    dlib_img.set_size(img.height, img.width);

    // Cast
    for (uint32_t y = 0; y < img.height; y++) {
        for (uint32_t x = 0; x < img.width; x++) {
            const float& r = img.pixels[(y * img.width + x) * img.n_ch + 0];
            const float& g = img.pixels[(y * img.width + x) * img.n_ch + 1];
            const float& b = img.pixels[(y * img.width + x) * img.n_ch + 2];
            dlib_img[y][x] = {CastUint(r), CastUint(g), CastUint(b)};
        }
    }

    return dlib_img;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
}  // namespace

// -----------------------------------------------------------------------------
// ----------------------------- Landmark Detector -----------------------------
// -----------------------------------------------------------------------------
LandmarkDetector::LandmarkDetector(const std::string& predictor_path) {
    m_detector = dlib::get_frontal_face_detector();
    dlib::deserialize(predictor_path) >> m_predictor;
}

std::vector<Landmark> LandmarkDetector::detect(const FloatImage& col_img,
                                               const FloatImage& pos_img,
                                               const Mesh& mesh) {
    // Cast to dlib image
    auto&& col_img_dlib = CastToDlibImg(col_img);

    // Detect face
    const std::vector<dlib::rectangle> face_rects = m_detector(col_img_dlib);
    if (face_rects.empty()) {
        return {};
    }
    std::cout << face_rects.size() << " faces are detected." << std::endl;
    if (face_rects.size() != 1) {
        return {};
    }
    const auto& face_rect = face_rects[0];

    // Predict 2D landmarks
    dlib::full_object_detection dlib_lmk = m_predictor(col_img_dlib, face_rect);

    // Show debug image (color)
    // dlib::image_window col_dlib_window(col_img_dlib);
    // col_dlib_window.add_overlay(dlib::render_face_detections({dlib_lmk}));
    // col_dlib_window.wait_until_closed();
    // Show debug image (position)
    // auto&& pos_img_dlib = CastToDlibImg(pos_img);
    // dlib::image_window pos_dlib_window(pos_img_dlib);
    // pos_dlib_window.add_overlay(dlib::render_face_detections({dlib_lmk}));
    // pos_dlib_window.wait_until_closed();

    // Parse landmarks
    std::vector<Landmark> landmarks;
    for (uint32_t i = 0; i < dlib_lmk.num_parts(); i++) {
        // 2D landmark
        const uint32_t x_2d = dlib_lmk.part(i).x();
        const uint32_t y_2d = dlib_lmk.part(i).y();
        glm::ivec2 lmk_2d{x_2d, y_2d};

        // 3D landmark
        const auto& width = pos_img.width;
        const auto& n_ch = pos_img.n_ch;
        const float& x_3d = pos_img.pixels[(y_2d * width + x_2d) * n_ch + 0];
        const float& y_3d = pos_img.pixels[(y_2d * width + x_2d) * n_ch + 1];
        const float& z_3d = pos_img.pixels[(y_2d * width + x_2d) * n_ch + 2];
        glm::vec3 lmk_3d{x_3d, y_3d, z_3d};

        // Search nearest vertex (TODO: Use KD-tree)
        uint32_t vtx_idx = uint32_t(~0);
        if (x_3d != 0.f && y_3d != 0.f && z_3d != 0.f) {  // Escape background
            float min_dist = std::numeric_limits<float>::max();
            for (auto&& vtx : mesh.vertices) {
                float dist = glm::distance(vtx.pos, lmk_3d);
                if (dist < min_dist) {
                    min_dist = dist;
                    vtx_idx = vtx.vtx_idx;
                }
            }
        }

        // Pack
        Landmark lmk{lmk_2d, lmk_3d, vtx_idx};
        landmarks.push_back(lmk);
    }

    return landmarks;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
