/*
 * ControllerApplication.cpp
 *
 *  Created on: 2016年10月15日
 *      Author: seeing
 */

#include "ControllerApplication.h"
#include <Service/ServiceType/RequestOdometry.h>
#include <Service/ServiceType/ResponseOdometry.h>
#include <Service/ServiceType/RequestTransform.h>
#include <Service/ServiceType/ResponseTransform.h>
#include <DataSet/Dispitcher.h>
#include <Console/Console.h>
#include <Service/Service.h>
#include <Transform/LinearMath/Quaternion.h>
#include <Transform/LinearMath/Vector3.h>
#include <Time/Rate.h>
#include <DataSet/DataType/PoseStamped.h>
#include <Time/Utils.h>

namespace NS_Controller
{
  
  ControllerApplication::ControllerApplication ()
  {
    memset (&current_pose, 0, sizeof(current_pose));
    memset (&current_odometry, 0, sizeof(current_odometry));
  }
  
  ControllerApplication::~ControllerApplication ()
  {
    // TODO Auto-generated destructor stub
    if (comm)
      delete comm;
  }
  
  void
  ControllerApplication::odomService (NS_ServiceType::RequestBase* request,
                                      NS_ServiceType::ResponseBase* response)
  {
    NS_ServiceType::RequestOdometry* req =
        (NS_ServiceType::RequestOdometry*) request;
    NS_ServiceType::ResponseOdometry* rep =
        (NS_ServiceType::ResponseOdometry*) response;
    
    boost::mutex::scoped_lock locker_ (odom_lock);
    
    double x = comm->getFloat64Value (BASE_REG_ODOM_X);
    double y = comm->getFloat64Value (BASE_REG_ODOM_Y);
    double theta = comm->getFloat64Value (BASE_REG_ODOM_THETA);
    
    current_odometry.pose.position.x = x;
    current_odometry.pose.position.y = y;
    current_odometry.pose.orientation.x = 0.0f;
    current_odometry.pose.orientation.y = 0.0f;
    current_odometry.pose.orientation.z = sin (theta / 2.0);
    current_odometry.pose.orientation.w = cos (theta / 2.0);
    
    rep->odom = current_odometry;
    
  }
  
  void
  ControllerApplication::odomTransformService (
      NS_ServiceType::RequestBase* request,
      NS_ServiceType::ResponseBase* response)
  {
    NS_ServiceType::RequestTransform* req =
        (NS_ServiceType::RequestTransform*) request;
    NS_ServiceType::ResponseTransform* rep =
        (NS_ServiceType::ResponseTransform*) response;
    
    boost::mutex::scoped_lock locker_ (odom_lock);

    current_pose.x = comm->getFloat64Value(BASE_REG_ODOM_X);
    current_pose.y = comm->getFloat64Value(BASE_REG_ODOM_Y);
    current_pose.theta = comm->getFloat64Value(BASE_REG_ODOM_THETA);
    
    /*
    NS_NaviCommon::console.debug ("odometry pose: x:%f, y:%f, theta:%f.",
                                  current_pose.x, current_pose.y,
                                  current_pose.theta);
    */

    rep->transform.translation.x = current_pose.x;
    rep->transform.translation.y = current_pose.y;
    rep->transform.translation.z = 0.0;
    
    rep->transform.rotation.x = 0.0;
    rep->transform.rotation.y = 0.0;
    rep->transform.rotation.z = sin (current_pose.theta / 2.0);
    rep->transform.rotation.w = cos (current_pose.theta / 2.0);

    /*
    NS_NaviCommon::console.debug (
        "odometry transform: x:%f, y:%f, rz:%f, rw:%f",
        rep->transform.translation.x, rep->transform.translation.y,
        rep->transform.rotation.z, rep->transform.rotation.w);
    */

  }

  void
  ControllerApplication::poseStampedCallback (NS_DataType::DataBase* pose_stamped)
  {
    NS_DataType::PoseStamped* pose = (NS_DataType::PoseStamped*) pose_stamped;

    delete pose_stamped;
  }

  void
  ControllerApplication::loadParameters ()
  {
    parameter.loadConfigurationFile ("controller.xml");
    comm_dev_name_ = parameter.getParameter ("device", "/dev/stm32");
    wheel_diameter_ = parameter.getParameter ("wheel_diameter", 0.068f);
    encoder_resolution_ = parameter.getParameter ("encoder_resolution", 16);
    gear_reduction_ = parameter.getParameter ("gear_reduction", 62);
    
    wheel_track_ = parameter.getParameter ("wheel_track", 0.265f);
    accel_limit_ = parameter.getParameter ("accel_limit", 1.0f);
    
    pid_kp_right_ = parameter.getParameter ("pid_kp_r", 20.0f);
    pid_kd_right_ = parameter.getParameter ("pid_kd_r", 12.0f);
    pid_ki_right_ = parameter.getParameter ("pid_ki_r", 0.0f);
    pid_ko_right_ = parameter.getParameter ("pid_ko_r", 50.0f);
    pid_max_right_ = parameter.getParameter ("pid_max_r", 100.0f);
    pid_min_right_ = parameter.getParameter ("pid_min_r", 0.0f);
    
    pid_kp_left_ = parameter.getParameter ("pid_kp_l", 20.0f);
    pid_kd_left_ = parameter.getParameter ("pid_kd_l", 12.0f);
    pid_ki_left_ = parameter.getParameter ("pid_ki_l", 0.0f);
    pid_ko_left_ = parameter.getParameter ("pid_ko_l", 50.0f);
    pid_max_left_ = parameter.getParameter ("pid_max_l", 100.0f);
    pid_min_left_ = parameter.getParameter ("pid_min_l", 0.0f);

    control_duration_ = parameter.getParameter ("control_duration", 100);
    control_timeout_ = parameter.getParameter ("control_timeout", 1000);
  }
  
  void
  ControllerApplication::configController ()
  {

    comm->setFloat64Value (BASE_REG_WHEEL_TRACK, wheel_track_);
    comm->setFloat64Value (BASE_REG_WHEEL_DIAMETER, wheel_diameter_);
    comm->setFloat64Value (BASE_REG_ENCODER_RESOLUTION, encoder_resolution_);
    comm->setFloat64Value (BASE_REG_GEAR_REDUCTION, gear_reduction_);
    comm->setFloat64Value (BASE_REG_ACCEL_LIMIT, accel_limit_);

    comm->setFloat64Value (BASE_REG_PID_KP_RIGHT, pid_kp_right_);
    comm->setFloat64Value (BASE_REG_PID_KI_RIGHT, pid_ki_right_);
    comm->setFloat64Value (BASE_REG_PID_KD_RIGHT, pid_kd_right_);
    comm->setFloat64Value (BASE_REG_PID_KO_RIGHT, pid_ko_right_);
    comm->setFloat64Value (BASE_REG_PID_MAX_RIGHT, pid_max_right_);
    comm->setFloat64Value (BASE_REG_PID_MIN_RIGHT, pid_min_right_);

    comm->setFloat64Value (BASE_REG_PID_KP_LEFT, pid_kp_left_);
    comm->setFloat64Value (BASE_REG_PID_KI_LEFT, pid_ki_left_);
    comm->setFloat64Value (BASE_REG_PID_KD_LEFT, pid_kd_left_);
    comm->setFloat64Value (BASE_REG_PID_KO_LEFT, pid_ko_left_);
    comm->setFloat64Value (BASE_REG_PID_MAX_LEFT, pid_max_left_);
    comm->setFloat64Value (BASE_REG_PID_MIN_LEFT, pid_min_left_);

    comm->setInt32Value (BASE_REG_CNTL_DURATION, control_duration_);
    comm->setInt32Value (BASE_REG_VEL_TIMEOUT, control_timeout_);

    comm->setInt32Value (BASE_REG_CFG_DONE, 1);

    NS_NaviCommon::console.debug ("finish config base parameter!");
  }
  
  bool
  ControllerApplication::checkDevice ()
  {
    unsigned int test_code = 1234;
    unsigned int test_val = 0;
    comm->setInt32Value (BASE_REG_TEST, test_code);
    NS_NaviCommon::delay (100);
    comm->setInt32Value (BASE_REG_TEST, test_code);
    test_val = comm->getInt32Value (BASE_REG_TEST);
    if(test_val != test_code)
    {
      NS_NaviCommon::console.debug ("test stm32 connection... check code [%d], but get [%d]!",
                                    test_code, test_val);
      return false;
    }
    NS_NaviCommon::console.debug ("test stm32 connection...ok!");

    return true;
  }

  void
  ControllerApplication::initialize ()
  {
    NS_NaviCommon::console.message ("controller is initializing!");
    loadParameters ();
    
    comm = new SpiComm (comm_dev_name_);
    if (!comm->open ())
    {
      NS_NaviCommon::console.error ("can't open base controller device!");
      return;
    }
    
    if (!checkDevice ())
    {
      NS_NaviCommon::console.error ("test base controller failure!");
      return;
    }

    configController ();
    
    dispitcher->subscribe (
        NS_NaviCommon::DATA_TYPE_POSE_STAMPED,
        boost::bind (&ControllerApplication::poseStampedCallback, this, _1));

    service->advertise (
        NS_NaviCommon::SERVICE_TYPE_RAW_ODOMETRY,
        boost::bind (&ControllerApplication::odomService, this, _1, _2));
    
    service->advertise (
        NS_NaviCommon::SERVICE_TYPE_ODOMETRY_BASE_TRANSFORM,
        boost::bind (&ControllerApplication::odomTransformService, this, _1,
                     _2));
    
    initialized = true;
  }
  
  void
  ControllerApplication::run ()
  {
    NS_NaviCommon::console.message ("controller is running!");
    
    running = true;
  }
  
  void
  ControllerApplication::quit ()
  {
    NS_NaviCommon::console.message ("controller is quitting!");

    running = false;
  }

} /* namespace NS_Controller */
