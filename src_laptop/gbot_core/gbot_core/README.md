# Installing Google Cartographer with ROS Noetic on Ubuntu Focal

Follow these steps to install and configure Google Cartographer for 2D SLAM using RPLIDAR A1.

---

## **Prerequisites**

Install the required tools:

```bash
sudo apt-get update
sudo apt-get install -y python3-wstool python3-rosdep ninja-build stow
```

---

## **Setting Up the Workspace**

1. Create a new `cartographer_ros` workspace in `catkin_ws`:

    ```bash
    cd catkin_ws
    wstool init src
    wstool merge -t src https://raw.githubusercontent.com/cartographer-project/cartographer_ros/master/cartographer_ros.rosinstall
    wstool update -t src
    ```

2. Install `cartographer_ros` dependencies:

    ```bash
    sudo rosdep init  # Ignore error if already executed
    rosdep update
    source /opt/ros/noetic/setup.bash
    sed -i -e "s%<depend>libabsl-dev</depend>%<!--<depend>libabsl-dev</depend>-->%g" src/cartographer/package.xml
    rosdep install --from-paths src --ignore-src -r -y
    ```

3. Install the `abseil-cpp` library manually:

    ```bash
    src/cartographer/scripts/install_abseil.sh
    ```

4. If there are conflicting versions, uninstall the ROS version of `abseil-cpp`:

    ```bash
    sudo apt-get remove ros-${ROS_DISTRO}-abseil-cpp
    ```

---

## **Building the Workspace**

Build and install:

```bash
catkin build
```

---

## **Handling Errors**

If you encounter this error:

```text
ImportError: cannot import name 'soft_unicode' from 'markupsafe'
```

Run:

```bash
pip install MarkupSafe==2.0.1
```

---

## **Running Cartographer 2D SLAM**

### **Configuration**

The configuration file (`gbot_lidar_2d.lua`) in the `configuration_files` directory is a key component. Here's an example configuration:

```lua
include "map_builder.lua"
include "trajectory_builder.lua"

options = {
  map_builder = MAP_BUILDER,
  trajectory_builder = TRAJECTORY_BUILDER,
  map_frame = "map",
  tracking_frame = "base_link",
  published_frame = "base_link",
  odom_frame = "odom",
  provide_odom_frame = true,
  publish_frame_projected_to_2d = true,
  use_odometry = false,
  use_nav_sat = false,
  use_landmarks = false,
  num_laser_scans = 1,
  num_multi_echo_laser_scans = 0,
  num_subdivisions_per_laser_scan = 1,
  num_point_clouds = 0,
  lookup_transform_timeout_sec = 0.2,
  submap_publish_period_sec = 0.3,
  pose_publish_period_sec = 5e-3,
  trajectory_publish_period_sec = 30e-3,
  rangefinder_sampling_ratio = 1.,
  odometry_sampling_ratio = 1.,
  fixed_frame_pose_sampling_ratio = 1.,
  imu_sampling_ratio = 1.,
  landmarks_sampling_ratio = 1.,
}

MAP_BUILDER.use_trajectory_builder_2d = true
TRAJECTORY_BUILDER_2D.min_range = 0.5
TRAJECTORY_BUILDER_2D.max_range = 8.
TRAJECTORY_BUILDER_2D.missing_data_ray_length = 8.5
TRAJECTORY_BUILDER_2D.use_imu_data = false
TRAJECTORY_BUILDER_2D.use_online_correlative_scan_matching = true
TRAJECTORY_BUILDER_2D.real_time_correlative_scan_matcher.linear_search_window = 0.1
TRAJECTORY_BUILDER_2D.real_time_correlative_scan_matcher.translation_delta_cost_weight = 10.
TRAJECTORY_BUILDER_2D.real_time_correlative_scan_matcher.rotation_delta_cost_weight = 1e-1
TRAJECTORY_BUILDER_2D.motion_filter.max_angle_radians = math.rad(0.2)
TRAJECTORY_BUILDER_2D.num_accumulated_range_data = 1
POSE_GRAPH.constraint_builder.min_score = 0.65
POSE_GRAPH.constraint_builder.global_localization_min_score = 0.65
POSE_GRAPH.optimization_problem.huber_scale = 1e2
POSE_GRAPH.optimize_every_n_nodes = 35

return options
```

---

### **Launching SLAM**

The `gbot.launch` file launches the SLAM process. Below is its structure:

```xml
<launch>
    <!-- Load robot description and start state publisher -->
    <param name="robot_description" textfile="$(find gbot_core)/urdf/head_2d.urdf" />
    <node name="robot_state_publisher" pkg="robot_state_publisher" type="robot_state_publisher" />
    
    <!-- Start RPLIDAR sensor node -->
    <node name="rplidarNode" pkg="rplidar_ros" type="rplidarNode" output="screen">
        <param name="serial_port" type="string" value="/dev/ttyUSB0"/>
        <param name="serial_baudrate" type="int" value="115200"/>
        <param name="frame_id" type="string" value="laser"/>
        <param name="inverted" type="bool" value="false"/>
        <param name="angle_compensate" type="bool" value="true"/>
    </node>
    
    <!-- Start Google Cartographer node -->
    <node name="cartographer_node" pkg="cartographer_ros" type="cartographer_node" args="
        -configuration_directory $(find gbot_core)/configuration_files
        -configuration_basename gbot_lidar_2d.lua" output="screen" />

    <!-- Convert Cartographer map to occupancy grid -->
    <node name="cartographer_occupancy_grid_node" pkg="cartographer_ros" type="cartographer_occupancy_grid_node" args="-resolution 0.05" />

    <!-- Launch RViz for visualization -->
    <node pkg="rviz" type="rviz" name="show_rviz" args="-d $(find gbot_core)/rviz/demo.rviz"/> 
</launch>
```

Run the SLAM process:

```bash
roslaunch gbot_core gbot.launch
```

---


# Explanation of Key Parameters and Supporting Files

Most of the parameters in the configuration were derived experimentally or calculated based on RPLIDAR's specifications. Below is a breakdown of key parameters and their significance:

---

## **Highlighted Parameters**

1. **Localization Source**  
   Since RPLIDAR is the only sensor, localization relies on the spatial differences between scanned data and a previously calculated map. Cartographer acts as the source of odometry.

2. **Key Parameters**  
   - `provide_odom_frame = true`  
     Cartographer publishes transforms between `published_frame` and `map_frame`. In this case, it provides the transform between **`base_link`** and **`map`**.

   - `publish_frame_projected_to_2d = true`  
     The published transforms are limited to the X and Y coordinates only, excluding elevation data.

   - `use_odometry = false`  
     This disables other odometry sources, making Cartographer directly link the `odom` frame to the `map` frame. For simple cases, this is sufficient.

   - `TRAJECTORY_BUILDER_2D.use_imu_data = false`  
     IMU data is not utilized. Cartographer relies solely on the laser scan matching algorithm.

For further details on Cartographer parameters, refer to the [Cartographer documentation](https://google-cartographer-ros.readthedocs.io/en/latest/configuration.html).

---

## **RViz Configuration**

The `rviz` directory includes a preconfigured view file for RViz. This file simplifies visualization by setting up topic listeners for mapping.

---

## **URDF Robot Description**

The `urdf` directory contains the robot's physical description in ROS format. The URDF file specifies:
- **Sensor spatial positions and relationships**
- **Joints (type, length, mass, inertia, etc.)**

More information on URDF can be found on the [ROS Wiki](http://wiki.ros.org/urdf/XML). 

### **Example URDF**

For a simple test bench with only one link between the base and lidar, a full URDF may not be necessary. You can instead use a [static transform](http://wiki.ros.org/tf#static_transform_publisher). However, the following URDF is a good starting point for adding IMU sensors, range finders, or other components:

```xml
<robot name="head_2d">
  <material name="orange">
    <color rgba="1.0 0.5 0.2 1" />
  </material>
  <material name="gray">
    <color rgba="0.2 0.2 0.2 1" />
  </material>

  <link name="laser">
    <visual>
      <origin xyz="0 0 0" />
      <geometry>
        <cylinder length="0.03" radius="0.03" />
      </geometry>
      <material name="gray" />
    </visual>
  </link>

  <link name="base_link">
    <visual>
      <origin xyz="0.01 0 0.015" />
      <geometry>
        <box size="0.11 0.065 0.052" />
      </geometry>
      <material name="orange" />
    </visual>
  </link>

  <joint name="laser_joint" type="fixed">
    <parent link="base_link" />
    <child link="laser" />
    <origin rpy="0 0 3.1415926" xyz="0 0 0.05" />
  </joint>
</robot>
```

This file defines:
- A **cylindrical lidar sensor** as the `laser` link.
- A **rectangular base link** named `base_link`.
- A **fixed joint** connecting the lidar to the base with a specified origin.

---

## **Conclusion**

Cartographer can produce 2D maps and odometry with decent quality using low-cost sensors. While adding an IMU or other odometry sources can improve performance in large areas, Cartographer's loop closure algorithm is sufficient for small indoor environments (50–60 m²).
