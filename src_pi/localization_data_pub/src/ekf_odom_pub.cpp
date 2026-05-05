#include "ros/ros.h"
#include "std_msgs/Int16.h"
#include <nav_msgs/Odometry.h>
#include <geometry_msgs/PoseStamped.h>
#include <tf2/LinearMath/Quaternion.h>
#include <tf2_ros/transform_broadcaster.h>
#include <cmath>

// Create odometry data publishers
ros::Publisher odom_data_pub;
ros::Publisher odom_data_pub_quat;
nav_msgs::Odometry odomNew;
nav_msgs::Odometry odomOld;

// Initial pose
const double initialX = 0.0;
const double initialY = 0.0;
const double initialTheta = 0.00000000001;
const double PI = 3.141592;

// Robot physical constants
const double TICKS_PER_REVOLUTION = 224;
const double WHEEL_RADIUS = 0.0325;
const double WHEEL_BASE = 0.23;
const double TICKS_PER_METER = 1096;

// Distance both wheels have traveled
double distanceLeft = 0;
double distanceRight = 0;

// Flag to see if initial pose has been received
bool initialPoseRecieved = false;

// Handle initial pose from RViz or a manual publisher
void set_initial_2d(const geometry_msgs::PoseStamped &rvizClick) {
  odomOld.pose.pose.position.x = rvizClick.pose.position.x;
  odomOld.pose.pose.position.y = rvizClick.pose.position.y;
  odomOld.pose.pose.orientation.z = rvizClick.pose.orientation.z;
  initialPoseRecieved = true;
}

// Calculate the distance the left wheel has traveled
void Calc_Left(const std_msgs::Int16& leftCount) {
  static int lastCountL = 0;
  if (leftCount.data != 0 && lastCountL != 0) {
    int leftTicks = (leftCount.data - lastCountL);
    if (leftTicks > 10000) {
      leftTicks = 0 - (65535 - leftTicks);
    } else if (leftTicks < -10000) {
      leftTicks = 65535 - leftTicks;
    }
    distanceLeft = leftTicks / TICKS_PER_METER;
  }
  lastCountL = leftCount.data;
}

// Calculate the distance the right wheel has traveled
void Calc_Right(const std_msgs::Int16& rightCount) {
  static int lastCountR = 0;
  if (rightCount.data != 0 && lastCountR != 0) {
    int rightTicks = rightCount.data - lastCountR;
    if (rightTicks > 10000) {
      distanceRight = (0 - (65535 - distanceRight)) / TICKS_PER_METER;
    } else if (rightTicks < -10000) {
      rightTicks = 65535 - rightTicks;
    }
    distanceRight = rightTicks / TICKS_PER_METER;
  }
  lastCountR = rightCount.data;
}

// Publish odometry in quaternion format
void publish_quat() {
  tf2::Quaternion q;
  q.setRPY(0, 0, odomNew.pose.pose.orientation.z);

  nav_msgs::Odometry quatOdom;
  quatOdom.header.stamp = odomNew.header.stamp;
  quatOdom.header.frame_id = "odom";
  quatOdom.child_frame_id = "base_footprint";
  quatOdom.pose.pose.position = odomNew.pose.pose.position;
  quatOdom.pose.pose.orientation.x = q.x();
  quatOdom.pose.pose.orientation.y = q.y();
  quatOdom.pose.pose.orientation.z = q.z();
  quatOdom.pose.pose.orientation.w = q.w();
  quatOdom.twist.twist = odomNew.twist.twist;

  for (int i = 0; i < 36; i++) {
    if (i == 0 || i == 7 || i == 14) {
      quatOdom.pose.covariance[i] = .01;
      quatOdom.twist.covariance[i] = .01;
    } else if (i == 21 || i == 28 || i == 35) {
      quatOdom.pose.covariance[i] = 0.1;
      quatOdom.twist.covariance[i] = 0.1;
    } else {
      quatOdom.pose.covariance[i] = 0;
      quatOdom.twist.covariance[i] = 0;
    }
  }
  odom_data_pub_quat.publish(quatOdom);
}

// Update the odometry estimate
void update_odom() {
  double cycleDistance = (distanceRight + distanceLeft) / 2;
  double cycleAngle = asin((distanceRight - distanceLeft) / WHEEL_BASE);
  double avgAngle = cycleAngle / 2 + odomOld.pose.pose.orientation.z;

  if (avgAngle > PI) avgAngle -= 2 * PI;
  else if (avgAngle < -PI) avgAngle += 2 * PI;

  odomNew.pose.pose.position.x = odomOld.pose.pose.position.x + cos(avgAngle) * cycleDistance;
  odomNew.pose.pose.position.y = odomOld.pose.pose.position.y + sin(avgAngle) * cycleDistance;
  odomNew.pose.pose.orientation.z = cycleAngle + odomOld.pose.pose.orientation.z;

  if (isnan(odomNew.pose.pose.position.x) || isnan(odomNew.pose.pose.position.y)) {
    odomNew.pose.pose.position.x = odomOld.pose.pose.position.x;
    odomNew.pose.pose.position.y = odomOld.pose.pose.position.y;
    odomNew.pose.pose.orientation.z = odomOld.pose.pose.orientation.z;
  }

  if (odomNew.pose.pose.orientation.z > PI) odomNew.pose.pose.orientation.z -= 2 * PI;
  else if (odomNew.pose.pose.orientation.z < -PI) odomNew.pose.pose.orientation.z += 2 * PI;

  odomNew.header.stamp = ros::Time::now();
  double dt = odomNew.header.stamp.toSec() - odomOld.header.stamp.toSec();
  if (dt > 0) {
    odomNew.twist.twist.linear.x = cycleDistance / dt;
    odomNew.twist.twist.angular.z = cycleAngle / dt;
  }

  odomOld = odomNew;
  odom_data_pub.publish(odomNew);
}

int main(int argc, char **argv) {
  ros::init(argc, argv, "ekf_odom_pub");
  ros::NodeHandle node;

  // Initialize odometry
  odomNew.header.frame_id = "odom";
  odomNew.pose.pose.position.z = 0;
  odomNew.pose.pose.orientation.x = 0;
  odomNew.pose.pose.orientation.y = 0;
  odomNew.twist.twist = geometry_msgs::Twist();
  odomOld.pose.pose.position.x = initialX;
  odomOld.pose.pose.position.y = initialY;
  odomOld.pose.pose.orientation.z = initialTheta;
  odomOld.header.stamp = ros::Time::now();

  // ✅ Automatically start publishing odometry
  initialPoseRecieved = false;

  // Subscribers
  ros::Subscriber subRight = node.subscribe("right_ticks", 100, Calc_Right, ros::TransportHints().tcpNoDelay());
  ros::Subscriber subLeft = node.subscribe("left_ticks", 100, Calc_Left, ros::TransportHints().tcpNoDelay());
  ros::Subscriber subInitPose = node.subscribe("initial_2d", 1, set_initial_2d);

  // Publishers
  odom_data_pub = node.advertise<nav_msgs::Odometry>("odom_data_euler", 100);
  odom_data_pub_quat = node.advertise<nav_msgs::Odometry>("odom_data_quat", 100);

  ros::Rate loop_rate(30);
  while (ros::ok()) {
    if (initialPoseRecieved) {
      update_odom();
      publish_quat();
    }
    ros::spinOnce();
    loop_rate.sleep();
  }

  return 0;
} 
