#ifndef LANDMARKER_H_20210213
#define LANDMARKER_H_20210213
#include "image.h"

BEGIN_VKW_SUPPRESS_WARNING
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing/render_face_detections.h>
#include <dlib/image_processing.h>
#include <dlib/gui_widgets.h>
#include <dlib/image_io.h>
END_VKW_SUPPRESS_WARNING

// -----------------------------------------------------------------------------
// -------------------------------- Landmarker ---------------------------------
// -----------------------------------------------------------------------------
class Landmarker {
public:
    Landmarker();
    void detect(const FloatImage& col_img);

private:
    dlib::frontal_face_detector m_detector;
};

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

#endif /* end of include guard */

