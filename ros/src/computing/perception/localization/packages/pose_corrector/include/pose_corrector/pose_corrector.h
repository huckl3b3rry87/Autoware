/*
 *  Copyright (c) 2017, Tier IV, Inc.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither the name of Autoware nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 *  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#ifndef POSE_CORRECTOR_H
#define POSE_CORRECTOR_H

#include <cmath>
#include <vector>
#include <deque>
#include <chrono>

#include <ros/ros.h>
#include <tf/tf.h>
#include <geometry_msgs/TwistStamped.h>
#include <sensor_msgs/PointCloud2.h>

#include <boost/shared_ptr.hpp>

#include "pose_corrector_msgs/Request.h"
#include "pose_corrector_msgs/Response.h"
#include "pose_corrector_msgs/Service.h"

//TODO: if not publish sensor topics -> segmentation falut
//TODO: request.pose.pose.orientation == all 0 -> warning
//TODO: dont describe the same codes at base class, use base class instance
//TODO: use forward declaration, reduce include files

class CombineSubBase;

class PoseCorrector
{
  public:
    PoseCorrector(const ros::NodeHandle& nh, const ros::NodeHandle& private_nh, const boost::shared_ptr<const CombineSubBase>& combine_sub_base_ptr);
    virtual ~PoseCorrector();
    geometry_msgs::PoseStamped calc(const geometry_msgs::PoseStamped& begin_pose, const ros::Time& begin_time, const ros::Time& end_time);

    void subCallback(const pose_corrector_msgs::Request::ConstPtr& req);
    bool srvCallback(pose_corrector_msgs::Service::Request& req, pose_corrector_msgs::Service::Response& res);

  private:
    ros::NodeHandle nh_;
    ros::NodeHandle private_nh_;
    
    ros::Subscriber sub_;
    ros::Publisher pub_;
    ros::ServiceServer srv_;

    boost::shared_ptr<const CombineSubBase> combine_sub_base_ptr_;
    
};

#endif