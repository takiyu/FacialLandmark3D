#include "image.h"

#include <vkw/warning_suppressor.h>

BEGIN_VKW_SUPPRESS_WARNING
#include <stb/stb_image.h>
END_VKW_SUPPRESS_WARNING

// -----------------------------------------------------------------------------
// -------------------------------- Float Image --------------------------------
// -----------------------------------------------------------------------------
FloatImage LoadImage(const std::string& filename, const uint32_t n_ch) {
    // Load image file
    int w_tmp, h_tmp, dummy_c;
    uint8_t* data = stbi_load(filename.c_str(), &w_tmp, &h_tmp, &dummy_c,
                              static_cast<int>(n_ch));
    const uint32_t w = static_cast<uint32_t>(w_tmp);
    const uint32_t h = static_cast<uint32_t>(h_tmp);

    // Cast to float array
    std::vector<uint8_t> ret_tex_u8(data, data + w * h * n_ch);
    std::vector<float> ret_tex;
    for (auto&& a : ret_tex_u8) {
        ret_tex.push_back(a / 255.f);
    }
    // Release raw image array
    stbi_image_free(data);

    // Pack to structure
    return FloatImage{ret_tex, w, h, n_ch};
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
