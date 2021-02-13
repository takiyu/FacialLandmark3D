#ifndef IMAGE_H_20210213
#define IMAGE_H_20210213

#include <iostream>
#include <sstream>
#include <vector>

// -----------------------------------------------------------------------------
// -------------------------------- Float Image --------------------------------
// -----------------------------------------------------------------------------
struct FloatImage {
    std::vector<float> pixels;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t n_ch = 0;
};

FloatImage CreateImage(uint32_t width, uint32_t height, uint32_t n_ch);

FloatImage LoadImage(const std::string& filename, uint32_t n_ch = 4);

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

#endif /* end of include guard */
