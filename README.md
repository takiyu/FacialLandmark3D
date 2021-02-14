# Facial Landmark 3D
Sample program of 2D landmark back-projection onto 3D geometry.

## How to run.
    ```
    git clone https://github.com/takiyu/FacialLandmark3D
    cd FacialLandmark3D
    git submodule update --init --recursive
    mkdir build && cd build
    cmake ..
    make
    ./bin/main
    ```

## To use 68 points landmark
    1. Download [shape_predictor_68_face_landmarks.dat.bz2](http://dlib.net/files/shape_predictor_68_face_landmarks.dat.bz2)
    2. Extract it under `data` directory.
    3. Modify `main.cpp`.
