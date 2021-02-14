#ifndef LANDMARKER_H_20210213
#define LANDMARKER_H_20210213
#include "image.h"
#include "renderer.h"

BEGIN_VKW_SUPPRESS_WARNING
#include <dlib/gui_widgets.h>
#include <dlib/image_io.h>
#include <dlib/image_processing.h>
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing/render_face_detections.h>
END_VKW_SUPPRESS_WARNING

// -----------------------------------------------------------------------------
// -------------------------- Landmark Correspondence --------------------------
// -----------------------------------------------------------------------------
struct Landmark {
    glm::ivec2 lmk_2d;
    glm::vec3 lmk_3d;
    uint32_t vtx_idx = 0;
};

// -----------------------------------------------------------------------------
// ----------------------------- Landmark Detector -----------------------------
// -----------------------------------------------------------------------------
class LandmarkDetector {
public:
    LandmarkDetector(const std::string& predictor_path);
    std::vector<Landmark> detect(const FloatImage& col_img,
                                 const FloatImage& pos_img, const Mesh& mesh);
    void show() const;

private:
    dlib::frontal_face_detector m_detector;
    dlib::shape_predictor m_predictor;

    dlib::array2d<dlib::rgb_pixel> m_prev_col_img;
    dlib::full_object_detection m_prev_lmk;
};

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

#endif /* end of include guard */
