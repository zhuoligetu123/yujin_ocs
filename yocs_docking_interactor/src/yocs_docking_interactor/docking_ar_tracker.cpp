/*  
 *  docking_ar_tracker.cpp
 *  LICENSE : BSD - https://raw.github.com/yujinrobot/yujin_ocs/license/LICENSE
 */

#include "yocs_docking_interactor/ar_tracker.hpp"

namespace yocs_docking_interactor {

DockingARTracker::DockingARTracker(ros::NodeHandle& n): nh_(n) {
  init();
}

DockingARTracker::~DockingARTracker() {
}

bool DockingARTracker::init()
{
  ros::NodeHandle pnh("~");
  pnh.param("docking_ar_min_confidence", min_confidence_, 0.3);

  dock_marker_registered_ = false;

  // ar track alvar tracker handler
  srv_tracker_params_ = nh_.serviceClient<dynamic_reconfigure::Reconfigure>(DockingARTrackerDefaultParam::AR_TRACKER_SET_PARAM);
  tracker_enabled_ = false;

  global_marker_received_ = false;
  sub_global_markers_ = nh_.subscribe(DockingARTrackerDefaultParam::SUB_GLOBAL_MARKERS, 1, &DockingARTracker::processGlobalMarkers, this);
}

bool DockingARTracker::isReady()
{
  return global_marker_received_;
}

bool DockingARTracker::reset()
{
  dock_marker_registered_ = false;
  return true;
}

void DockingARTracker::processGlobalMarkers(const ar_track_alvar_msgs::AlvarMarkers::ConstPtr& msg)
{
  global_markers_ = *msg;

  for(unsigned int i=0; i < global_markers_.markers.size(); i++)
  {
    ar_track_alvar_msgs::AlvarMarker m = global_markers_.markers[i];
    m.id = m.id - 3;
    global_markers_.markers.push_back(m);
  }

  global_marker_received_ = true;
}

void DockingARTracker::customCB(const ar_track_alvar_msgs::AlvarMarkers& spotted_markers, const std::vector<yocs::TrackedMarker> &tracked_markers)
{
}

bool DockingARTracker::setClosestAsDockingMarker()
{
  // the closest ar marker is the docking marker.
  return ARMarkerTracking::closest(1.0, min_confidence_, global_markers_,  docking_marker_in_robot_frame_);
}

bool DockingARTracker::registerDockingOnGlobalFrame(const std::string global_frame, std::string& message)
{
  ar_track_alvar_msgs::AlvarMarkers spotted_markers;
  ar_track_alvar_msgs::AlvarMarker current_dock_marker;

  if(spotted(1.0, min_confidence_, global_markers_, spotted_markers) == false)
  {
    message = "failed to spot dock marker";
    return false;
  }

  if(included(docking_marker_in_robot_frame_.id, spotted_markers, &current_dock_marker) == false)
  {
    message = "failed to spot dock marker";
    return false;
  }

  docking_marker_in_robot_frame_ = current_dock_marker;
  geometry_msgs::PoseStamped before = docking_marker_in_robot_frame_.pose;
  geometry_msgs::PoseStamped after;

  try
  {
    tf_listener_.transformPose(global_frame, before, after);
    docking_marker_in_global_frame_ = docking_marker_in_robot_frame_;
    docking_marker_in_global_frame_.header = after.header;
    docking_marker_in_global_frame_.pose = after;
  }catch(tf::TransformException& e)
  {
    ROS_ERROR("Cannot get tf %s -> %s : %s", docking_marker_in_robot_frame_.pose.header.frame_id.c_str(), global_frame.c_str(), e.what());
    message = "cannot get tranform to global frame";
    return false;
  }

  dock_marker_registered_ = true;
  message = "success";
  return true;
}
}
