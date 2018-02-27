// =============================================================================
// PROJECT CHRONO - http://projectchrono.org
//
// Copyright (c) 2014 projectchrono.org
// All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file at the top level of the distribution and at
// http://projectchrono.org/license-chrono.txt.
//
// =============================================================================
// Authors: Radu Serban
// =============================================================================
//
// Demonstration of a steering path-follower PID controller.
//
// The vehicle reference frame has Z up, X towards the front of the vehicle, and
// Y pointing to the left.
// TO DO:
// Find proper yaw rate calculation
// Subsscribe to msg from obstacle_avoidance_chrono, callback to that to calculate ChBezierCurve
// =============================================================================

#include <fstream>
#include<iostream>
#include "boost/bind.hpp"
#include "chrono/core/ChFileutils.h"
#include "chrono/core/ChRealtimeStep.h"
#include "chrono/utils/ChFilters.h"
#include <ros/console.h>
#include "ros/ros.h"
#include "std_msgs/String.h"
#include "ros_chrono_msgs/veh_status.h"
#include "traj_gen/Control.h"
#include <sstream>
#include "chrono_vehicle/ChVehicleModelData.h"
#include "chrono_vehicle/terrain/RigidTerrain.h"
#include "chrono_vehicle/driver/ChIrrGuiDriver.h"
#include "chrono_vehicle/driver/ChPathFollowerDriver.h"
#include "chrono_vehicle/wheeled_vehicle/utils/ChWheeledVehicleIrrApp.h"
#include <math.h>
#include "chrono_models/vehicle/hmmwv/HMMWV.h"

#define PI 3.1415926535
//using veh_status.msg
using namespace chrono;
using namespace chrono::geometry;
using namespace chrono::vehicle;
using namespace chrono::vehicle::hmmwv;


// =============================================================================
// Problem parameters
// Main Data Path
//std::string data_path("/home/shreyas/.julia/v0.6/MAVs/catkin_ws/data/vehicle/");
std::string data_path("data/vehicle/");
// Contact method type
ChMaterialSurface::ContactMethod contact_method = ChMaterialSurface::SMC;

// Type of tire model (RIGID, LUGRE, FIALA, or PACEJKA)
TireModelType tire_model = TireModelType::RIGID;

// Input file name for PACEJKA tires if they are selected
std::string pacejka_tire_file(data_path+"hmmwv/tire/HMMWV_pacejka.tir");
//std::string pacejka_tire_file("hmmwv/tire/HMMWV_pacejka.tir");

// Type of powertrain model (SHAFTS or SIMPLE)
PowertrainModelType powertrain_model = PowertrainModelType::SIMPLE;

// Drive type (FWD, RWD, or AWD)
DrivelineType drive_type = DrivelineType::RWD;

// Visualization type for vehicle parts (PRIMITIVES, MESH, or NONE)
VisualizationType chassis_vis_type = VisualizationType::PRIMITIVES;
VisualizationType suspension_vis_type = VisualizationType::PRIMITIVES;
VisualizationType steering_vis_type = VisualizationType::PRIMITIVES;
VisualizationType wheel_vis_type = VisualizationType::NONE;
VisualizationType tire_vis_type = VisualizationType::PRIMITIVES;

// Input file names for the path-follower driver model
std::string steering_controller_file(data_path+"generic/driver/SteeringController.json");
//std::string steering_controller_file("generic/driver/SteeringController.json");
std::string speed_controller_file(data_path+"generic/driver/SpeedController.json");
//std::string speed_controller_file("generic/driver/SpeedController.json");
// std::string path_file("paths/straight.txt");
std::string path_file(data_path+"paths/my_path.txt");

// std::string path_file("paths/curve.txt");
// std::string path_file("paths/NATO_double_lane_change.txt");
//std::string path_file(data_path+"paths/ISO_double_lane_change.txt");
//std::string path_file("paths/ISO_double_lane_change.txt");
// Initial vehicle location and orientation
ChVector<> initLoc(200, 0, 0.5);
ChQuaternion<> initRot(cos(PI/4), 0, 0, sin(PI/4)); //initial yaw of pi/2
// Desired vehicle speed (m/s)
double target_speed = 12;

// Rigid terrain dimensions
double terrainHeight = 0;
double terrainLength = 1000.0;  // size in X direction
double terrainWidth = 1000.0;   // size in Y direction

// Point on chassis tracked by the chase camera
ChVector<> trackPoint(0.0, 0.0, 1.75);

// Simulation step size
double step_size = 1e-2;
double tire_step_size = step_size;

// Simulation end time
double t_end = 100;

// Render FPS
double fps = 60;

// Debug logging
bool debug_output = false;
double debug_fps = 10;

// Output directories
const std::string out_dir = GetChronoOutputPath() + "STEERING_CONTROLLER";
const std::string pov_dir = out_dir + "/POVRAY";

// POV-Ray output
bool povray_output = false;

// Vehicle state output (forced to true if povray output enabled)
bool state_output = false;
int filter_window_size = 20;


// =============================================================================

// Custom Irrlicht event receiver for selecting current driver model.
class ChDriverSelector : public irr::IEventReceiver {
  public:
    ChDriverSelector(const ChVehicle& vehicle, ChPathFollowerDriver* driver_follower, ChIrrGuiDriver* driver_gui)
        : m_vehicle(vehicle),
          m_driver_follower(driver_follower),
          m_driver_gui(driver_gui),
          m_driver(m_driver_follower),
          m_using_gui(false) {}

    ChDriver* GetDriver() { return m_driver; }
    bool UsingGUI() const { return m_using_gui; }

    virtual bool OnEvent(const irr::SEvent& event) {
        // Only interpret keyboard inputs.
        if (event.EventType != irr::EET_KEY_INPUT_EVENT)
            return false;

        // Disregard key pressed
        if (event.KeyInput.PressedDown)
            return false;

        switch (event.KeyInput.Key) {
            case irr::KEY_COMMA:
                if (m_using_gui) {
                    m_driver = m_driver_follower;
                    m_using_gui = false;
                }
                return true;
            case irr::KEY_PERIOD:
                if (!m_using_gui) {
                    m_driver_gui->SetThrottle(m_driver_follower->GetThrottle());
                    m_driver_gui->SetSteering(m_driver_follower->GetSteering());
                    m_driver_gui->SetBraking(m_driver_follower->GetBraking());
                    m_driver = m_driver_gui;
                    m_using_gui = true;
                }
                return true;
            case irr::KEY_HOME:
                if (!m_using_gui && !m_driver_follower->GetSteeringController().IsDataCollectionEnabled()) {
                    std::cout << "Data collection started at t = " << m_vehicle.GetChTime() << std::endl;
                    m_driver_follower->GetSteeringController().StartDataCollection();
                }
                return true;
            case irr::KEY_END:
                if (!m_using_gui && m_driver_follower->GetSteeringController().IsDataCollectionEnabled()) {
                    std::cout << "Data collection stopped at t = " << m_vehicle.GetChTime() << std::endl;
                    m_driver_follower->GetSteeringController().StopDataCollection();
                }
                return true;
            case irr::KEY_INSERT:
                if (!m_using_gui && m_driver_follower->GetSteeringController().IsDataAvailable()) {
                    char filename[100];
                    sprintf(filename, "controller_%.2f.out", m_vehicle.GetChTime());
                    std::cout << "Data written to file " << filename << std::endl;
                    m_driver_follower->GetSteeringController().WriteOutputFile(std::string(filename));
                }
                return true;
            default:
                break;
        }

        return false;
    }

  private:
    bool m_using_gui;
    const ChVehicle& m_vehicle;
    ChPathFollowerDriver* m_driver_follower;
    ChIrrGuiDriver* m_driver_gui;
    ChDriver* m_driver;
};
//----------------------callback
//void controlCallback(const mavs_control::Control::ConstPtr &msg)

void controlCallback(const traj_gen::Control::ConstPtr &msg,HMMWV_Reduced &my_hmmwv, ChVehicleIrrApp &app,ChIrrGuiDriver &driver_gui)
{

  app.SetPaused(1);

  std::vector<double> x_vec=msg->x;
  std::vector<double> y_vec=msg->y;
//  ROS_INFO("I heard: [%f]", x_vec[0]);
  //ROS_INFO_STREAM(msg);
  double num_pts = x_vec.size();
  double num_cols = 3;
  double z_val = 0.5;
  std::ofstream myfile;
  myfile.open(path_file,std::ofstream::out | std::ofstream::trunc);

  myfile << ' ' << num_pts << ' '<< num_cols << '\n';
  for (int pt_cnt=0; pt_cnt<num_pts;pt_cnt=pt_cnt+1){
  myfile << ' ' << x_vec[pt_cnt] << ' '<< y_vec[pt_cnt] <<' ' << z_val << '\n';
}
  myfile.close();

  auto path = ChBezierCurve::read(path_file);

  // Initialize driver follower
  //driver_follower.Reset();
  ChPathFollowerDriver driver_follower(my_hmmwv.GetVehicle(), steering_controller_file,
                                       speed_controller_file, path, "my_path", target_speed);
  driver_follower.Initialize();

  // Create and register a custom Irrlicht event receiver to allow selecting the
  // current driver model.
  ChDriverSelector selector(my_hmmwv.GetVehicle(), &driver_follower, &driver_gui);
  app.SetUserEventReceiver(&selector);

  // Finalize construction of visualization assets
  app.AssetBindAll();
  app.AssetUpdateAll();

  // -----------------
  // Initialize output
  // -----------------

  state_output = state_output || povray_output;

  if (state_output) {
      if (ChFileutils::MakeDirectory(out_dir.c_str()) < 0) {
          std::cout << "Error creating directory " << out_dir << std::endl;
          //return 1;
      }
  }

  if (povray_output) {
      if (ChFileutils::MakeDirectory(pov_dir.c_str()) < 0) {
          std::cout << "Error creating directory " << pov_dir << std::endl;
        //  return 1;
      }
      driver_follower.ExportPathPovray(out_dir);
  }

  app.SetPaused(0);


}
// =============================================================================

int main(int argc, char* argv[]) {
//while (ros::ok())
//{


    GetLog() << "Copyright (c) 2017 projectchrono.org\nChrono version: " << CHRONO_VERSION << "\n\n";

    SetChronoDataPath(CHRONO_DATA_DIR);
    vehicle::SetDataPath("opt/chrono/chrono/data/vehicle");
    // std::cout << GetChronoDataPath() << "\n"; check path of chrono data folder
    // Initialize ROS Chrono node and set node handle to n

    ros::init(argc, argv, "Chronode");
    ros::NodeHandle n;
    ros::Publisher vehicleinfo_pub = n.advertise<ros_chrono_msgs::veh_status>("vehicleinfo", 1);
    //ros::Rate loop_rate(5);

    // ------------------------------
    // Create the vehicle and terrain
    // ------------------------------

    // Create the HMMWV vehicle, set parameters, and initialize
    HMMWV_Reduced my_hmmwv;
    my_hmmwv.SetContactMethod(contact_method);
    my_hmmwv.SetChassisFixed(false);
    my_hmmwv.SetInitPosition(ChCoordsys<>(initLoc, initRot));
    my_hmmwv.SetPowertrainType(powertrain_model);
    my_hmmwv.SetDriveType(drive_type);
    my_hmmwv.SetTireType(tire_model);
    my_hmmwv.SetTireStepSize(tire_step_size);
    my_hmmwv.SetPacejkaParamfile(pacejka_tire_file);
    my_hmmwv.Initialize();

    my_hmmwv.SetChassisVisualizationType(chassis_vis_type);
    my_hmmwv.SetSuspensionVisualizationType(suspension_vis_type);
    my_hmmwv.SetSteeringVisualizationType(steering_vis_type);
    my_hmmwv.SetWheelVisualizationType(wheel_vis_type);
    my_hmmwv.SetTireVisualizationType(tire_vis_type);

    // Create the terrain
    RigidTerrain terrain(my_hmmwv.GetSystem());
    terrain.SetContactFrictionCoefficient(0.9f);
    terrain.SetContactRestitutionCoefficient(0.01f);
    terrain.SetContactMaterialProperties(2e7f, 0.3f);
    terrain.SetColor(ChColor(1, 1, 1));
    //terrain.SetTexture(chrono::vehicle::GetDataFile("terrain/textures/tile4.jpg"), 200, 200);
    terrain.SetTexture(data_path+"terrain/textures/tile4.jpg", 200, 200);
    terrain.Initialize(terrainHeight, terrainLength, terrainWidth);

    // ----------------------
    // Create the Bezier path
    // ----------------------


    auto path = ChBezierCurve::read(path_file);
//  auto path = ChBezierCurve::read(chrono::vehicle::GetDataFile(path_file));
//z= 0.1;
//std::cout << path;
//    lol->write(data_path+"paths/my_path.txt");

    // ---------------------------------------
    // Create the vehicle Irrlicht application
    // ---------------------------------------

    ChVehicleIrrApp app(&my_hmmwv.GetVehicle(), &my_hmmwv.GetPowertrain(), L"Steering Controller Demo",
                        irr::core::dimension2d<irr::u32>(800, 640));

    app.SetHUDLocation(500, 20);
    app.SetSkyBox();
    app.AddTypicalLogo();
    app.AddTypicalLights(irr::core::vector3df(-150.f, -150.f, 200.f), irr::core::vector3df(-150.f, 150.f, 200.f), 100,
                         100);
    app.AddTypicalLights(irr::core::vector3df(150.f, -150.f, 200.f), irr::core::vector3df(150.0f, 150.f, 200.f), 100,
                         100);
    app.EnableGrid(false);
    app.SetChaseCamera(trackPoint, 6.0, 0.5);

    app.SetTimestep(step_size);

    // Visualization of controller points (sentinel & target)
    irr::scene::IMeshSceneNode* ballS = app.GetSceneManager()->addSphereSceneNode(0.1f);
    irr::scene::IMeshSceneNode* ballT = app.GetSceneManager()->addSphereSceneNode(0.1f);
    ballS->getMaterial(0).EmissiveColor = irr::video::SColor(0, 255, 0, 0);
    ballT->getMaterial(0).EmissiveColor = irr::video::SColor(0, 0, 255, 0);

    // -------------------------
    // Create the driver systems
    // -------------------------

    // Create both a GUI driver and a path-follower and allow switching between them
    ChIrrGuiDriver driver_gui(app);
    driver_gui.Initialize();

    /*
    ChPathFollowerDriver driver_follower(my_hmmwv.GetVehicle(), path, "my_path", target_speed);
    driver_follower.GetSteeringController().SetLookAheadDistance(5);
    driver_follower.GetSteeringController().SetGains(0.5, 0, 0);
    driver_follower.GetSpeedController().SetGains(0.4, 0, 0);
    */

    ChPathFollowerDriver driver_follower(my_hmmwv.GetVehicle(), steering_controller_file,
                                         speed_controller_file, path, "my_path", target_speed);
    driver_follower.Initialize();

    // Create and register a custom Irrlicht event receiver to allow selecting the
    // current driver model.
    ChDriverSelector selector(my_hmmwv.GetVehicle(), &driver_follower, &driver_gui);
    app.SetUserEventReceiver(&selector);

    // Finalize construction of visualization assets
    app.AssetBindAll();
    app.AssetUpdateAll();

    // -----------------
    // Initialize output
    // -----------------

    state_output = state_output || povray_output;

    if (state_output) {
        if (ChFileutils::MakeDirectory(out_dir.c_str()) < 0) {
            std::cout << "Error creating directory " << out_dir << std::endl;
            return 1;
        }
    }

    if (povray_output) {
        if (ChFileutils::MakeDirectory(pov_dir.c_str()) < 0) {
            std::cout << "Error creating directory " << pov_dir << std::endl;
            return 1;
        }
        driver_follower.ExportPathPovray(out_dir);
    }

    utils::CSV_writer csv("\t");
    csv.stream().setf(std::ios::scientific | std::ios::showpos);
    csv.stream().precision(6);

    utils::ChRunningAverage fwd_acc_GC_filter(filter_window_size);
    utils::ChRunningAverage lat_acc_GC_filter(filter_window_size);

    utils::ChRunningAverage fwd_acc_driver_filter(filter_window_size);
    utils::ChRunningAverage lat_acc_driver_filter(filter_window_size);

    // ---------------
    // Simulation loop
    // ---------------

    // Driver location in vehicle local frame
    ChVector<> driver_pos = my_hmmwv.GetChassis()->GetLocalDriverCoordsys().pos;
    // Number of simulation steps between miscellaneous events
    double render_step_size = 1 / fps;
    int render_steps = (int)std::ceil(render_step_size / step_size);
    double debug_step_size = 1 / debug_fps;
    int debug_steps = (int)std::ceil(debug_step_size / step_size);

    // Initialize simulation frame counter and simulation time
    ChRealtimeStepTimer realtime_timer;
    int sim_frame = 0;
    int render_frame = 0;
    double yaw_prev_val=0;
    double t_prev_val=0;
    //my_hmmwv_ptr = &my_hmmwv;
    //app_ptr = &app;
  //  int file_idx=0;
    //double xVec[3];
    std::ofstream myfile1;
    myfile1.open(data_path+"position.txt",std::ofstream::out | std::ofstream::trunc);

    while (app.GetDevice()->run()) {
        // Extract system state
        ros::Subscriber sub = n.subscribe<traj_gen::Control>("/mavs/optimal_control", 10, boost::bind(controlCallback, _1, boost::ref(my_hmmwv),boost::ref(app),boost::ref(driver_gui)));
    //   ros::Subscriber sub = n.subscribe<mavs_control::Control>("/mavs/optimal_control", 10, boost::bind(&controlCallback, _1, boost::ref(my_hmmwv),boost::ref(app)));
      //  ros::Subscriber sub = n.subscribe("/mavs/optimal_control", 10, controlCallback);
        double time = my_hmmwv.GetSystem()->GetChTime();
        ChVector<> acc_CG = my_hmmwv.GetVehicle().GetChassisBody()->GetPos_dtdt();
        ChVector<> acc_driver = my_hmmwv.GetVehicle().GetVehicleAcceleration(driver_pos);
        double fwd_acc_CG = fwd_acc_GC_filter.Add(acc_CG.x());
        double lat_acc_CG = lat_acc_GC_filter.Add(acc_CG.y());
        double fwd_acc_driver = fwd_acc_driver_filter.Add(acc_driver.x());
        double lat_acc_driver = lat_acc_driver_filter.Add(acc_driver.y());

        // End simulation
        if (time >= t_end)
            break;

        // Collect output data from modules (for inter-module communication)
        double throttle_input = selector.GetDriver()->GetThrottle();
        double steering_input = selector.GetDriver()->GetSteering();
        double braking_input = selector.GetDriver()->GetBraking();

        /*
        // Hack for acceleration-braking maneuver
        static bool braking = false;
        if (my_hmmwv.GetVehicle().GetVehicleSpeed() > target_speed)
            braking = true;
        if (braking) {
            throttle_input = 0;
            braking_input = 1;
        } else {
            throttle_input = 1;
            braking_input = 0;
        }
        */

        // Update sentinel and target location markers for the path-follower controller.
        // Note that we do this whether or not we are currently using the path-follower driver.
        const ChVector<>& pS = driver_follower.GetSteeringController().GetSentinelLocation();
        const ChVector<>& pT = driver_follower.GetSteeringController().GetTargetLocation();
        ballS->setPosition(irr::core::vector3df((irr::f32)pS.x(), (irr::f32)pS.y(), (irr::f32)pS.z()));
        ballT->setPosition(irr::core::vector3df((irr::f32)pT.x(), (irr::f32)pT.y(), (irr::f32)pT.z()));

        // Render scene and output POV-Ray data
        if (sim_frame % render_steps == 0) {
            app.BeginScene(true, true, irr::video::SColor(255, 140, 161, 192));
            app.DrawAll();
            app.EndScene();
          /*
            if (povray_output) {
                char filename[100];
                sprintf(filename, "%s/data_%03d.dat", pov_dir.c_str(), render_frame + 1);
                utils::WriteShapesPovray(my_hmmwv.GetSystem(), filename);
            }

            if (state_output) {
                csv << time << steering_input << throttle_input << braking_input;
                csv << my_hmmwv.GetVehicle().GetVehicleSpeed();
                csv << acc_CG.x() << fwd_acc_CG << acc_CG.y() << lat_acc_CG;
                csv << acc_driver.x() << fwd_acc_driver << acc_driver.y() << lat_acc_driver;
                csv << std::endl;
            }*/

            render_frame++;
        }
/*
        // Debug logging
        if (debug_output && sim_frame % debug_steps == 0) {
            GetLog() << "driver acceleration:  " << acc_driver.x() << "  " << acc_driver.y() << "  " << acc_driver.z()
                     << "\n";
            GetLog() << "CG acceleration:      " << acc_CG.x() << "  " << acc_CG.y() << "  " << acc_CG.z() << "\n";
            GetLog() << "\n";
        }*/

        // Update modules (process inputs from other modules)
        driver_follower.Synchronize(time);
        driver_gui.Synchronize(time);
        terrain.Synchronize(time);
        my_hmmwv.Synchronize(time, steering_input, braking_input, throttle_input, terrain);
        std::string msg = selector.UsingGUI() ? "GUI driver" : "Follower driver";
        app.Synchronize(msg, steering_input, throttle_input, braking_input);

        // Advance simulation for one timestep for all modules
        double step = realtime_timer.SuggestSimulationStep(step_size);
        driver_follower.Advance(step);
        driver_gui.Advance(step);
        terrain.Advance(step);
        my_hmmwv.Advance(step);
        app.Advance(step);

        // Increment simulation frame number
        sim_frame++;

        ChVector<> global_pos = my_hmmwv.GetVehicle().GetVehicleCOMPos();//global location of chassis reference frame origin
        ChQuaternion<> global_orntn = my_hmmwv.GetVehicle().GetVehicleRot();//global orientation as quaternion
        ChVector<> rot_dt = my_hmmwv.GetVehicle().GetChassisBody()->GetWvel_loc();//global orientation as quaternion
        ChVector<> global_velCOM = my_hmmwv.GetVehicle().GetChassisBody()->GetPos_dt();
        ChVector<> global_accCOM = my_hmmwv.GetVehicle().GetChassisBody()->GetPos_dtdt();
        //ChVector<> euler_ang = global_orntn.Q_to_Rotv(); //convert to euler angles
        double q0 = global_orntn[0];
        double q1 = global_orntn[1];
        double q2 = global_orntn[2];
        double q3 = global_orntn[3];
        double yaw_val=atan2(2*(q0*q3+q1*q2),1-2*(q2*q2+q3*q3));

        if (yaw_val<0){
          yaw_val=-yaw_val+PI/2;
        }
        else if (yaw_val>=0 && yaw_val<=PI/2){
          yaw_val=PI/2-yaw_val;
        }
        else if (yaw_val>PI/2 && yaw_val<=PI){
          yaw_val=5*PI/2-yaw_val;
        }
        //ChPacejkaTire<> slip_angle = GetSlipAngle()
        double slip_angle = my_hmmwv.GetTire(0)->GetLongitudinalSlip();

        ros_chrono_msgs::veh_status data_out;
        data_out.t_chrono=time; //time in chrono simulation
        data_out.x_pos= global_pos[0] ;
        data_out.y_pos=global_pos[1];
        data_out.x_v= global_velCOM[0]; //speed measured at the origin of the chassis reference frame.
        data_out.y_v= global_velCOM[1];
        data_out.x_a= global_accCOM[0];
        data_out.yaw_curr=yaw_val; //in radians
        data_out.yaw_rate=-rot_dt[2];//(yaw_val-yaw_prev_val)/(time-t_prev_val); //in radians
        data_out.sa=slip_angle;
        data_out.thrt_in=throttle_input; //throttle input in the range [0,+1]
        data_out.brk_in=braking_input; //braking input in the range [0,+1]
        data_out.str_in=steering_input; //steeering input in the range [-1,+1]

        yaw_prev_val=yaw_val; //update previous yaw and time for next iteration's yaw rate calculation
        t_prev_val=time;

        vehicleinfo_pub.publish(data_out);
      //  loop_rate.sleep();
       ros::spinOnce();

       myfile1 << ' ' << global_pos[0] << ' '<< global_pos[1]  <<' ' << global_pos[2]  << '\n';
    }

    if (state_output){
        csv.write_to_file(out_dir + "/state.out");
    }

//}

myfile1.close();

    return 0;
}
