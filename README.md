# Benchmarking LiDAR-Based SLAM Algorithms (ROS Noetic)

This repository contains the code and logs for a mobile robot used to **benchmark LiDAR-based SLAM algorithms** on ROS Noetic:

- Cartographer
- Hector SLAM
- Gmapping

The system is split into code for the robot (Raspberry Pi) and for the laptop (mapping and logging), plus log files and motor driver code.

---

## 1. Repository Structure

- `Motor_driver_code/` – Low‑level motor driver code for the robot base.
- `src_pi/` – ROS packages and nodes intended to run on the Raspberry Pi (onboard the robot).
- `src_laptop/` – ROS packages and nodes intended to run on the laptop (SLAM, visualization, logging).
- `carto.log`, `gmapping.log`, `hector_usage.log` – Example log files produced during benchmarking.
- `logging.py` – Utility script for logging SLAM performance/usage.

---

## 2. System Requirements

- Ubuntu 20.04
- ROS Noetic (desktop-full or base installation)
- Catkin workspace (catkin_make or catkin_tools)
- A 2D LiDAR sensor supported by ROS (e.g., publishing on `/scan`)
- Network connection between laptop and robot (for ROS master/slave setup)

The instructions below assume:

- ROS workspace on both machines: `~/catkin_ws`
- ROS distribution: `noetic`
- Shell: `bash`

Adjust paths and distributions as needed.

---

## 3. Clone and Build This Repository

### 3.1. On the Laptop

1. Create a catkin workspace (if you don’t already have one):

    ```bash
    mkdir -p ~/catkin_ws/src
    cd ~/catkin_ws
    catkin_make
    echo "source ~/catkin_ws/devel/setup.bash" >> ~/.bashrc
    source ~/.bashrc
    ```

2. Clone this repository into the workspace:

    ```bash
    cd ~/catkin_ws/src
    git clone https://github.com/zaarAli/Benchmarking_SLAM.git
    ```

3. Build the workspace:

    ```bash
    cd ~/catkin_ws
    catkin_make
    ```

### 3.2. On the Raspberry Pi (Robot)

1. Install ROS Noetic (ROS-Base is enough) and create a workspace:

    ```bash
    mkdir -p ~/catkin_ws/src
    cd ~/catkin_ws
    catkin_make
    echo "source ~/catkin_ws/devel/setup.bash" >> ~/.bashrc
    source ~/.bashrc
    ```

2. Clone this repository (only robot‑side code is used on the Pi, but cloning the full repo is simplest):

    ```bash
    cd ~/catkin_ws/src
    git clone https://github.com/zaarAli/Benchmarking_SLAM.git
    ```

3. Build:

    ```bash
    cd ~/catkin_ws
    catkin_make
    ```

---

## 4. Installing Required Dependencies

### 4.1. Common ROS Dependencies

On both laptop and Pi (as needed):

```bash
sudo apt update
sudo apt install ros-noetic-tf2-ros ros-noetic-tf2-geometry-msgs \
                 ros-noetic-nav-msgs ros-noetic-geometry-msgs \
                 ros-noetic-sensor-msgs ros-noetic-map-server \
                 ros-noetic-amcl ros-noetic-robot-state-publisher \
                 ros-noetic-xacro
```

You may already have many of these from a standard ROS installation.

---

## 5. Install and Configure Gmapping (ROS Noetic)

### 5.1. Install Gmapping

On the **laptop**:

```bash
sudo apt update
sudo apt install ros-noetic-slam-gmapping
```

### 5.2. Typical Launch Setup

Gmapping expects at least:

- A 2D laser scan topic (default: `/scan`)
- Proper TF tree (e.g., `map -> odom -> base_link -> laser`)

Example launch (to be adapted to your robot, frame names, and this repo’s launch files):

```bash
roslaunch Benchmarking_SLAM gmapping.launch
```

If `gmapping.launch` is inside one of the packages in `src_laptop`, use that package name accordingly:

```bash
roslaunch <your_gmapping_pkg> gmapping.launch
```

Check that:

- Laser topic name in the launch file matches your sensor.
- TF frames in the launch file match your robot’s URDF / TF setup.

---

## 6. Install and Configure Hector SLAM (ROS Noetic)

### 6.1. Install Hector SLAM

On the **laptop**:

```bash
sudo apt update
sudo apt install ros-noetic-hector-slam
```

This typically provides packages such as:

- `hector_mapping`
- `hector_geotiff`
- `hector_trajectory_server`

### 6.2. Typical Launch Setup

Hector SLAM mainly needs:

- A 2D laser scan topic (often `/scan`)
- Approximate robot motion can be inferred from scan matching, but you can provide IMU and odometry if available.

Example launch:

```bash
roslaunch Benchmarking_SLAM hector_slam.launch
```

Or, if using the standard `hector_mapping` launch within this repo’s config:

```bash
roslaunch <your_hector_pkg> hector_slam.launch
```

Verify that:

- `base_frame`, `odom_frame`, and `map_frame` in the launch file match your TF tree.
- Laser scan topic is consistent with your robot.

---

## 7. Install and Configure Cartographer (ROS Noetic)

Cartographer is not officially packaged for ROS Noetic on all platforms, so the usual approach is to build from source in a separate workspace or within your existing one. Below is a typical Noetic‑on‑Ubuntu 20.04 workflow.

### 7.1. Install Cartographer Prerequisites

On the **laptop**:

## **Prerequisites**

Install the required tools:

```bash
sudo apt-get update
sudo apt-get install -y python3-wstool python3-rosdep ninja-build stow
```

### 7.2. Install Cartographer ROS Dependencies

```bash
    sudo rosdep init  # Ignore error if already executed
    rosdep update
    source /opt/ros/noetic/setup.bash
    sed -i -e "s%<depend>libabsl-dev</depend>%<!--<depend>libabsl-dev</depend>-->%g" src/cartographer/package.xml
    rosdep install --from-paths src --ignore-src -r -y
```
Install the `abseil-cpp` library manually:

```bash
    src/cartographer/scripts/install_abseil.sh
```

If there are conflicting versions, uninstall the ROS version of `abseil-cpp`:

    ```bash
    sudo apt-get remove ros-${ROS_DISTRO}-abseil-cpp
    ```

### 7.3. Configure and Launch Cartographer

Cartographer requires:

- A 2D laser scan topic (e.g., `/scan`)
- TF between `map`, `odom`, `base_link`, and `laser` frames
- A Lua configuration file specifying sensor topics, frames, and options

In this repo, configuration and launch files for Cartographer are in `src_laptop` (e.g., `cartographer_config.lua` and `cartographer.launch`).



## 8. Robot-Side Setup (Raspberry Pi)

On the **Pi**, you typically run:

- Motor driver node(s) from `Motor_driver_code/`.
- Sensor drivers (LiDAR, IMU).
- Any robot base controller node publishing `/cmd_vel`, `/odom`, etc.

### 8.1. Install Robot-Side Dependencies

On the Pi:

```bash
sudo apt update
sudo apt install ros-noetic-rosserial-python ros-noetic-rosserial-arduino \
                 ros-noetic-joy ros-noetic-teleop-twist-keyboard
```

(Exact list depends on the hardware interface used by `Motor_driver_code` and `src_pi`.)

### 8.2. Running the Robot Stack

1. Start ROS master on the **laptop** or Pi (choose one as master). Example (master on laptop):

    ```bash
    # On laptop
    roscore
    ```

2. On the **Pi**, set `ROS_MASTER_URI` to point to the laptop and `ROS_HOSTNAME` or `ROS_IP` appropriately (in `~/.bashrc`):

    ```bash
    export ROS_MASTER_URI=http://<laptop_ip>:11311
    export ROS_HOSTNAME=<pi_ip>
    ```

3. Launch the robot nodes


---

## 9. Running SLAM Algorithms for Benchmarking

With the robot running and publishing sensor data:

1. **Start RViz** on the laptop for visualization:

    ```bash
    rviz
    ```

2. **Run one SLAM algorithm at a time** to avoid conflicting map publishers.

   - Gmapping:

     ```bash
     roslaunch <your_gmapping_pkg> gmapping.launch
     ```

   - Hector SLAM:

     ```bash
     roslaunch <your_hector_pkg> hector_slam.launch
     ```

   - Cartographer:

     ```bash
     roslaunch <your_cartographer_pkg> cartographer.launch
     ```

3. Use the robot to drive through the environment while logging data. You can:

   - Record bag files:

     ```bash
     rosbag record -O slam_benchmark.bag /scan /tf /odom /cmd_vel
     ```

   - Use `logging.py` from this repo to collect algorithm‑specific metrics (see the script’s header / docstring or comments for usage).

4. Repeat the same trajectory for each SLAM algorithm and compare:

   - Map quality (visually in RViz)
   - CPU usage and memory (e.g., via `top` / `htop`)
   - Real‑time performance (lag, dropped frames)
   - Any metrics logged in `carto.log`, `gmapping.log`, `hector_usage.log`

---

## 10. Notes and Troubleshooting

- Ensure the TF tree is consistent; misconfigured frames are a common cause of SLAM failures.
- Check that the laser topic name in each launch file matches the actual LiDAR driver topic.
- If Cartographer fails to start, verify that all required libraries are installed and that the Lua configuration is in the right path.
- For reproducible benchmarking, keep robot speed, path, and environment constant across all three SLAM algorithms.

---

## 11. License and Citation

If you use this repository for academic work, please consider citing the associated final-year project:

> Benchmarking LiDAR-Based SLAM Algorithms (Cartographer, Hector, Gmapping) on ROS Noetic
