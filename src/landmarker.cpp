#include "landmarker.h"

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
            if (r) {
                std::cout << r << std::endl;
            }
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
// -------------------------------- Landmarker ---------------------------------
// -----------------------------------------------------------------------------
Landmarker::Landmarker() {
    m_detector = dlib::get_frontal_face_detector();
}

void Landmarker::detect(const FloatImage& col_img) {
    auto&& col_img_dlib = CastToDlibImg(col_img);
    dlib::pyramid_up(col_img_dlib);
    std::vector<dlib::rectangle> dets = m_detector(col_img_dlib);
    std::cout << dets.size() << std::endl;

    dlib::image_window dlib_window(col_img_dlib);
    dlib_window.wait_until_closed();
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
