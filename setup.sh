# Setup
sudo apt-get -y update && \
sudo apt-get install -y \
    build-essential \
    git \
    libglew-dev \
    cmake \
    ffmpeg \
    libavcodec-dev \
    libavutil-dev \
    libavformat-dev \
    libswscale-dev \
    libavdevice-dev \
    libjpeg-dev \
    libtiff5-dev \
    libopencv-dev \
    libopenexr-dev \
    libdc1394-22-dev \
    libraw1394-dev

# Pangolin
git clone https://github.com/stevenlovegrove/Pangolin.git && \
cd Pangolin && \
mkdir build && \
cd build && \
cmake .. && \
cmake --build . && \ 
cd ../..

# Eigen
sudo -H git clone https://github.com/eigenteam/eigen-git-mirror.git /usr/local/include/eigen

# chmod +x build.sh
# ./build.sh
