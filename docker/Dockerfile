# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

ARG ROS_VERSION=humble
ARG UBUNTU_VERSION=jammy

FROM ros:${ROS_VERSION}-ros-base-${UBUNTU_VERSION}

ENV WORKSPACE=/data/workspace

WORKDIR $WORKSPACE

# O3DE Variables
ARG O3DE_REPO=https://github.com/o3de/o3de.git
ARG O3DE_BRANCH=main
ARG O3DE_COMMIT=2310.1
ENV O3DE_ROOT=$WORKSPACE/o3de

# O3DE Extras Variables
ARG O3DE_EXTRAS_REPO=https://github.com/o3de/o3de-extras.git
ARG O3DE_EXTRAS_BRANCH=main
ARG O3DE_EXTRAS_COMMIT=2310.1
ENV O3DE_EXTRAS_ROOT=$WORKSPACE/o3de-extras

# ROSConDemo Variables
ARG ROSCON_DEMO_REPO=https://github.com/maciejmajek/ROSConDemo.git
ARG ROSCON_DEMO_BRANCH=main
ARG ROSCON_DEMO_COMMIT=HEAD
ENV ROSCON_DEMO_ROOT=$WORKSPACE/ROSConDemo
ENV ROSCON_DEMO_PROJECT=$ROSCON_DEMO_ROOT/Project
ENV ROSCON_DEMO_NAV_ROOT=$ROSCON_DEMO_ROOT/kraken_nav

# Copy the release-patch in case its needed
COPY o3de_2.1.0.patch $WORKSPACE/

ENV LANG=en_US.UTF-8

# Setup time zone and locale data (necessary for SSL and HTTPS packages)
RUN apt-get update \
    && DEBIAN_FRONTEND="noninteractive" apt-get -y install tzdata locales keyboard-configuration curl \
    && sed -i -e 's/# en_US.UTF-8 UTF-8/en_US.UTF-8 UTF-8/' /etc/locale.gen \
    && dpkg-reconfigure --frontend=noninteractive locales \
    && update-locale LANG=en_US.UTF-8 \
    && sh -c 'echo "deb [arch=amd64,arm64] http://repo.ros2.org/ubuntu/main `lsb_release -cs` main" > /etc/apt/sources.list.d/ros2-latest.list' \
    && curl -s https://raw.githubusercontent.com/ros/rosdistro/master/ros.asc | sudo apt-key add - \
    && rm -rf /var/lib/apt/lists/*

# Install the required ubuntu packages
RUN apt-get update \
    && apt-get install -y \
            binutils \
            clang \
            cmake \
            git \
            git-lfs \
            libglu1-mesa-dev \
            libxcb-xinerama0 \
            libfontconfig1-dev \
            libnvidia-gl-470 \
            libxcb-xkb-dev \
            libxkbcommon-x11-dev \
            libxkbcommon-dev \
            libxcb-xfixes0-dev \
            libxcb-xinput-dev \
            libxcb-xinput0 \
            libpcre2-16-0 \
            ninja-build \
            python3-pip \
            software-properties-common \
            ros-${ROS_DISTRO}-ackermann-msgs \
            ros-${ROS_DISTRO}-control-toolbox \
            ros-${ROS_DISTRO}-gazebo-msgs \
            ros-${ROS_DISTRO}-joy \
            ros-${ROS_DISTRO}-navigation2 \
            ros-${ROS_DISTRO}-rviz2 \
            ros-${ROS_DISTRO}-tf2-ros \
            ros-${ROS_DISTRO}-urdfdom \
            ros-${ROS_DISTRO}-vision-msgs \
            ros-${ROS_DISTRO}-cyclonedds \
            ros-${ROS_DISTRO}-rmw-cyclonedds-cpp \
            ros-${ROS_DISTRO}-slam-toolbox \
            ros-${ROS_DISTRO}-nav2-bringup \
            ros-${ROS_DISTRO}-pointcloud-to-laserscan \
            ros-${ROS_DISTRO}-teleop-twist-keyboard \
            ros-${ROS_DISTRO}-topic-tools \
            python3-colcon-common-extensions \
            && rm -rf /var/lib/apt/lists/* \
            && pip install python-statemachine

ENV RMW_IMPLEMENTATION=rmw_cyclonedds_cpp

# Clone, register, and build the projects
RUN . /opt/ros/${ROS_DISTRO}/setup.sh \
    \
    # Clone o3de \
    \
    && git clone --single-branch -b $O3DE_BRANCH $O3DE_REPO $O3DE_ROOT \
    && git -C $O3DE_ROOT lfs install \
    && git -C $O3DE_ROOT lfs pull \
    && git -C $O3DE_ROOT reset --hard $O3DE_COMMIT \
    \
    ############################################################### \
    # Apply a patch to fix a release compile issue if needed \
    ############################################################### \
    && if [ "$(md5sum $O3DE_ROOT/Gems/Atom/RPI/Code/Source/RPI.Public/RPISystem.cpp | awk '{print $1}')" = "07389c00378a13f31f1d6faf097a026b" ];then git -C $O3DE_ROOT apply $WORKSPACE/o3de_2.1.0.patch; fi \
    \
    ############################################################### \
    # Clone o3de-extras   \
    ############################################################### \
    && git clone --single-branch -b $O3DE_EXTRAS_BRANCH $O3DE_EXTRAS_REPO $O3DE_EXTRAS_ROOT \
    && git -C $O3DE_EXTRAS_ROOT lfs install \
    && git -C $O3DE_EXTRAS_ROOT lfs pull \
    && git -C $O3DE_EXTRAS_ROOT reset --hard $O3DE_EXTRAS_COMMIT \
    \
    ############################################################### \
    # Clone the ROSConDemo \
    ############################################################### \
    && git clone --single-branch -b $ROSCON_DEMO_BRANCH $ROSCON_DEMO_REPO $ROSCON_DEMO_ROOT \
    && git -C $ROSCON_DEMO_ROOT lfs install \
    && git -C $ROSCON_DEMO_ROOT lfs pull \
    && git -C $ROSCON_DEMO_ROOT reset --hard $ROSCON_DEMO_COMMIT \
    \
    ############################################################### \
    # Get the O3DE python and register the gem paths and projects \
    ############################################################### \
    && $O3DE_ROOT/python/get_python.sh \
    && $O3DE_ROOT/scripts/o3de.sh register -ep $O3DE_ROOT \
    && $O3DE_ROOT/scripts/o3de.sh register -gp $O3DE_EXTRAS_ROOT/Gems/ROS2 \
    && $O3DE_ROOT/scripts/o3de.sh register -pp $ROSCON_DEMO_PROJECT \
    \
    ############################################################### \
    # Build the Editor, Tools, and clients
    ############################################################### \
    && cmake -B $ROSCON_DEMO_PROJECT/build/linux -G "Ninja Multi-Config" \
                            -S $ROSCON_DEMO_PROJECT \
                            -DLY_DISABLE_TEST_MODULES=ON \
                            -DLY_STRIP_DEBUG_SYMBOLS=ON \
                            -DAZ_USE_PHYSX5=ON \
    && cmake --build $ROSCON_DEMO_PROJECT/build/linux --config profile \
             --target ROSConDemo.GameLauncher Editor ROSConDemo.Assets \
    \
    ############################################################### \
    # Remove build artifacts
    ############################################################### \
    && rm -rf ~/.o3de/3rdParty/ \
    && rm -rf $O3DE_ROOT/.git \
    && rm -rf $O3DE_ROOT/AutomatedTesting \
    && rm -rf $O3DE_ROOT/python/downloaded_packages \
    && rm -rf $ROSCON_DEMO_PROJECT/build/linux/Azcg/ \
    && rm -rf $ROSCON_DEMO_PROJECT/build/linux/CMake \
    && rm -rf $ROSCON_DEMO_PROJECT/build/linux/CMakeFiles/ \
    && rm -rf $ROSCON_DEMO_PROJECT/build/linux/External/ \
    && rm -rf $ROSCON_DEMO_PROJECT/build/linux/Testing/ \
    && rm -rf $ROSCON_DEMO_PROJECT/build/linux/_deps/ \
    && rm -rf $ROSCON_DEMO_PROJECT/build/linux/cmake \
    && rm -rf $ROSCON_DEMO_PROJECT/build/linux/lib/ \
    && rm -rf $ROSCON_DEMO_PROJECT/build/linux/o3de/ \
    && rm -rf $ROSCON_DEMO_PROJECT/build/linux/packages/

# Build the navigation stack
RUN . /opt/ros/${ROS_DISTRO}/setup.sh \
    && cd $ROSCON_DEMO_NAV_ROOT \
    && colcon build --symlink-install

# Add the appropriate ros2 environment setup script to the system startup
RUN echo "[Unit]" > /etc/systemd/system/setup_ros.service \
    && echo "After=network.target" >> /etc/systemd/system/setup_ros.service \
    && echo "" >> /etc/systemd/system/setup_ros.service \
    && echo "[Service]" >> /etc/systemd/system/setup_ros.service \
    && if [ "${ROS_DISTRO}" = "iron" ]; then \
         echo "ExecStart=/opt/ros/iron/setup.bash" >> /etc/systemd/system/setup_ros.service; \
       elif [ "${ROS_DISTRO}" = "humble" ]; then \
         echo "ExecStart=/opt/ros/humble/setup.bash" >> /etc/systemd/system/setup_ros.service; \
       elif [ "${ROS_DISTRO}" = "galactic" ]; then \
         echo "ExecStart=/opt/ros/galactic/setup.bash" >> /etc/systemd/system/setup_ros.service; \
       fi \
    && echo "" >> /etc/systemd/system/setup_ros.service \
    && echo "[Install]" >> /etc/systemd/system/setup_ros.service \
    && echo "WantedBy=default.target" >> /etc/systemd/system/setup_ros.service \
    && echo "" >> /etc/systemd/system/setup_ros.service

ENV NVIDIA_VISIBLE_DEVICES all
ENV NVIDIA_DRIVER_CAPABILITIES all
