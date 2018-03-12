# Chrono Vehicle Model
A HMMWV vehicle model developed using Project Chrono is controlled using ROS parameters which transmit a desired path. The vehicle model
is initialized with parameters from a config yaml file, including an initial desired xy path. The vehicle can track to an updated path sent through the
ROS parameter server. This is demonstrated by traj_gen_chrono.cpp updating the ROS parameters for the desired x and y coordinates after the vehicle begins tracking
the initial desired path. The vehicle's states are published in a ROS msg and also saved as ROS parameters.

## Main Packages

 - traj_gen_chrono
 - ros_chrono
 - ros_chrono_msgs

## How to Build

```

$ cd ~/Autoware/ros
$ catkin_make

```
## How to Start

```
$ cd $HOME/Autoware/ros
$ roslaunch ros_chrono path_follower.launch

```
## Change Vehicle Initial Conditions

To change intiial conditions, initial trajectory, or turn off the GUI (work in progress), edit the parameters in the HMMWV YAML config file:

```
$ sudo gedit ros/src/system/chrono/ros_chrono/config/hmmwv_params.yaml

```

## Change Values of Updated Path

Change the values of x2, y2 in traj_gen.cpp and recompile using catkin_make.

## Monitor Vehicle State

Open another terminal and type:

```
$ rostopic echo vehicleinfo

```
This displays all states and inputs specified in the veh_status.msg file.

## Change ROS Parameter from command line

```
$ rosparam set <param_name> "param_value"

```

## Current Differences between 3DOF Vehicle model and HMMWV model:

### HMMWV Model
- mass: 2,449.55696
- Izz: 3,570.2
- la: 1.871831 (Distance from COM to front axle)
- lb: 1.871831 (Distance from COM to rear axle)

### 3DOF Vehicle Model
- mass: 2,688.7
- Izz: 4,110.1
- la: 1.5775
- lb: 1.7245  
