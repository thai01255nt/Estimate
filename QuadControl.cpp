#include "Common.h"
#include "QuadControl.h"

#include "Utility/SimpleConfig.h"

#include "Utility/StringUtils.h"
#include "Trajectory.h"
#include "BaseController.h"
#include "Math/Mat3x3F.h"

#ifdef __PX4_NUTTX
#include <systemlib/param/param.h>
#endif

void QuadControl::Init()
{
  BaseController::Init();

  // variables needed for integral control
  integratedAltitudeError = 0;
    
#ifndef __PX4_NUTTX
  // Load params from simulator parameter system
  ParamsHandle config = SimpleConfig::GetInstance();
   
  // Load parameters (default to 0)
  kpPosXY = config->Get(_config+".kpPosXY", 0);
  kpPosZ = config->Get(_config + ".kpPosZ", 0);
  KiPosZ = config->Get(_config + ".KiPosZ", 0);
     
  kpVelXY = config->Get(_config + ".kpVelXY", 0);
  kpVelZ = config->Get(_config + ".kpVelZ", 0);

  kpBank = config->Get(_config + ".kpBank", 0);
  kpYaw = config->Get(_config + ".kpYaw", 0);

  kpPQR = config->Get(_config + ".kpPQR", V3F());

  maxDescentRate = config->Get(_config + ".maxDescentRate", 100);
  maxAscentRate = config->Get(_config + ".maxAscentRate", 100);
  maxSpeedXY = config->Get(_config + ".maxSpeedXY", 100);
  maxAccelXY = config->Get(_config + ".maxHorizAccel", 100);
  
  maxTiltAngle = config->Get(_config + ".maxTiltAngle", 100);

  minMotorThrust = config->Get(_config + ".minMotorThrust", 0);
  maxMotorThrust = config->Get(_config + ".maxMotorThrust", 100);
#else
  // load params from PX4 parameter system
  //TODO
  param_get(param_find("MC_PITCH_P"), &Kp_bank);
  param_get(param_find("MC_YAW_P"), &Kp_yaw);
#endif
}

VehicleCommand QuadControl::GenerateMotorCommands(float collThrustCmd, V3F momentCmd)
{
  // Convert a desired 3-axis moment and collective thrust command to 
  //   individual motor thrust commands
  // INPUTS: 
  //   desCollectiveThrust: desired collective thrust [N]
  //   desMoment: desired rotation moment about each axis [N m]
  // OUTPUT:
  //   set class member variable cmd (class variable for graphing) where
  //   cmd.desiredThrustsN[0..3]: motor commands, in [N]

  // HINTS: 
  // - you can access parts of desMoment via e.g. desMoment.x
  // You'll need the arm length parameter L, and the drag/thrust ratio kappa

  // w1**2 w2**2 w3**2 w4**2 = c/kf
  // w1**2 - w2**2 - w3**2 w4**2 = Mx/(l*kf)
  // w1**2 w2**2 - w3**2 - w4**2 = My/(l*kf)
  // w1**2 - w2**2 w3**2 - w4**2 = Mz/km
  ////////////////////////////// BEGIN STUDENT CODE ///////////////////////////
  V3F M = momentCmd;
  float l = L / ( 2*sqrt(2.f));
  float c_bar = collThrustCmd ;
  float p_bar = M.x / l;
  float q_bar = M.y / l;
  float r_bar = -M.z / kappa;
  cmd.desiredThrustsN[0] = (c_bar + p_bar + q_bar + r_bar) / 4.f; // front left
  cmd.desiredThrustsN[1] = (c_bar - p_bar + q_bar - r_bar) / 4.f; // front right
  cmd.desiredThrustsN[2] = (c_bar + p_bar - q_bar - r_bar) / 4.f; // rear left
  cmd.desiredThrustsN[3] = (c_bar - p_bar - q_bar + r_bar) / 4.f; // rear right

  float a = cmd.desiredThrustsN[0];
  float b = cmd.desiredThrustsN[1];
  float c = cmd.desiredThrustsN[2];
  float d = cmd.desiredThrustsN[3];

  /////////////////////////////// END STUDENT CODE ////////////////////////////
  return cmd;
}

V3F QuadControl::BodyRateControl(V3F pqrCmd, V3F pqr)
{
  // Calculate a desired 3-axis moment given a desired and current body rate
  // INPUTS: 
  //   pqrCmd: desired body rates [rad/s]
  //   pqr: current or estimated body rates [rad/s]
  // OUTPUT:
  //   return a V3F containing the desired moments for each of the 3 axes

  // HINTS: 
  //  - you can use V3Fs just like scalars: V3F a(1,1,1), b(2,3,4), c; c=a-b;
  //  - you'll need parameters for moments of inertia Ixx, Iyy, Izz
  //  - you'll also need the gain parameter kpPQR (it's a V3F)

  V3F momentCmd;

  ////////////////////////////// BEGIN STUDENT CODE ///////////////////////////
  float p_rate_cmd = (pqrCmd.x - pqr.x)*kpPQR.x;
  float q_rate_cmd = (pqrCmd.y - pqr.y)*kpPQR.y;
  float r_rate_cmd = (pqrCmd.z - pqr.z)*kpPQR.z;
  float Mx = p_rate_cmd * Ixx;
  float My = q_rate_cmd * Iyy;
  float Mz = r_rate_cmd * Izz;

  momentCmd.x = Mx;
  momentCmd.y = My;
  momentCmd.z = Mz;

  /////////////////////////////// END STUDENT CODE ////////////////////////////

  return momentCmd;
}

// returns a desired roll and pitch rate 
V3F QuadControl::RollPitchControl(V3F accelCmd, Quaternion<float> attitude, float collThrustCmd)
{
  // Calculate a desired pitch and roll angle rates based on a desired global
  //   lateral acceleration, the current attitude of the quad, and desired
  //   collective thrust command
  // INPUTS: 
  //   accelCmd: desired acceleration in global XY coordinates [m/s2]
  //   attitude: current or estimated attitude of the vehicle
  //   collThrustCmd: desired collective thrust of the quad [N]
  // OUTPUT:
  //   return a V3F containing the desired pitch and roll rates. The Z
  //     element of the V3F should be left at its default value (0)

  // HINTS: 
  //  - we already provide rotation matrix R: to get element R[1,2] (python) use R(1,2) (C++)
  //  - you'll need the roll/pitch gain kpBank
  //  - collThrustCmd is a force in Newtons! You'll likely want to convert it to acceleration first

  V3F pqrCmd;
  Mat3x3F R = attitude.RotationMatrix_IwrtB();
  
  ////////////////////////////// BEGIN STUDENT CODE ///////////////////////////
  float u = collThrustCmd/mass;
  float b_x_cmd = CONSTRAIN(accelCmd.x / (-u), -1, 1);
  float b_y_cmd = CONSTRAIN(accelCmd.y / (-u), -1, 1);
  float b_x_dot = (b_x_cmd - R[2])*kpBank;
  float b_y_dot = (b_y_cmd - R[5])*kpBank;
  float p_target = b_x_dot * R[3] - b_y_dot * R[0];
  float q_target = b_x_dot * R[4] - b_y_dot * R[1];
  /////////////////////////////// END STUDENT CODE ////////////////////////////
  if (collThrustCmd == 0) {
	  p_target = 0;
	  q_target = 0;
  }
  pqrCmd.x = p_target;
  pqrCmd.y = q_target;
  pqrCmd.z = 0;
  return pqrCmd;
}

float QuadControl::AltitudeControl(float posZCmd, float velZCmd, float posZ, float velZ, Quaternion<float> attitude, float accelZCmd, float dt)
{
  // Calculate desired quad thrust based on altitude setpoint, actual altitude,
  //   vertical velocity setpoint, actual vertical velocity, and a vertical 
  //   acceleration feed-forward command
  // INPUTS: 
  //   posZCmd, velZCmd: desired vertical position and velocity in NED [m]
  //   posZ, velZ: current vertical position and velocity in NED [m]
  //   accelZCmd: feed-forward vertical acceleration in NED [m/s2]
  //   dt: the time step of the measurements [seconds]
  // OUTPUT:
  //   return a collective thrust command in [N]

  // HINTS: 
  //  - we already provide rotation matrix R: to get element R[1,2] (python) use R(1,2) (C++)
  //  - you'll need the gain parameters kpPosZ and kpVelZ
  //  - maxAscentRate and maxDescentRate are maximum vertical speeds. Note they're both >=0!
  //  - make sure to return a force, not an acceleration
  //  - remember that for an upright quad in NED, thrust should be HIGHER if the desired Z acceleration is LOWER
  float err = posZCmd - posZ;

  float err_dot = velZCmd - velZ;
  integratedAltitudeError = integratedAltitudeError + err * dt;
  float u = err * kpPosZ + err_dot * kpVelZ + integratedAltitudeError*KiPosZ + accelZCmd;
  Mat3x3F R = attitude.RotationMatrix_IwrtB();
  float R33 = R[8];
  float u_bar = (-u+9.81f)/R33;
  float thrust = u_bar*mass;
  thrust = CONSTRAIN(thrust, -maxDescentRate / dt, maxAscentRate / dt);
  ////////////////////////////// BEGIN STUDENT CODE ///////////////////////////



  /////////////////////////////// END STUDENT CODE ////////////////////////////
  
  return thrust;
}

// returns a desired acceleration in global frame
V3F QuadControl::LateralPositionControl(V3F posCmd, V3F velCmd, V3F pos, V3F vel, V3F accelCmdFF)
{
  // Calculate a desired horizontal acceleration based on 
  //  desired lateral position/velocity/acceleration and current pose
  // INPUTS: 
  //   posCmd: desired position, in NED [m]
  //   velCmd: desired velocity, in NED [m/s]
  //   pos: current position, NED [m]
  //   vel: current velocity, NED [m/s]
  //   accelCmdFF: feed-forward acceleration, NED [m/s2]
  // OUTPUT:
  //   return a V3F with desired horizontal accelerations. 
  //     the Z component should be 0
  // HINTS: 
  //  - use the gain parameters kpPosXY and kpVelXY
  //  - make sure you limit the maximum horizontal velocity and acceleration
  //    to maxSpeedXY and maxAccelXY

  // make sure we don't have any incoming z-component
  velCmd.x = CONSTRAIN(velCmd.x, -maxSpeedXY, maxSpeedXY);
  velCmd.y = CONSTRAIN(velCmd.y, -maxSpeedXY, maxSpeedXY);
  float err_x = posCmd.x - pos.x;
  float err_dot_x = velCmd.x - vel.x;

  float err_y = posCmd.y - pos.y;
  float err_dot_y = velCmd.y - vel.y;

  float u_x = err_x * kpPosXY + err_dot_x * kpVelXY + accelCmdFF.x;
  float u_y = err_y * kpPosXY + err_dot_y * kpVelXY + accelCmdFF.y;
  accelCmdFF.z = 0;
  velCmd.z = 0;
  posCmd.z = pos.z;

  accelCmdFF.x = CONSTRAIN(u_x,-maxAccelXY,maxAccelXY);
  accelCmdFF.y = CONSTRAIN(u_y,-maxAccelXY,maxAccelXY);
  // we initialize the returned desired acceleration to the feed-forward value.
  // Make sure to _add_, not simply replace, the result of your controller
  // to this variable
  V3F accelCmd = accelCmdFF;

  ////////////////////////////// BEGIN STUDENT CODE ///////////////////////////

  

  /////////////////////////////// END STUDENT CODE ////////////////////////////

  return accelCmd;
}

// returns desired yaw rate
float QuadControl::YawControl(float yawCmd, float yaw)
{
  // Calculate a desired yaw rate to control yaw to yawCmd
  // INPUTS: 
  //   yawCmd: commanded yaw [rad]
  //   yaw: current yaw [rad]
  // OUTPUT:
  //   return a desired yaw rate [rad/s]
  // HINTS: 
  //  - use fmodf(foo,b) to unwrap a radian angle measure float foo to range [0,b]. 
  //  - use the yaw control gain parameter kpYaw
  float err = yawCmd - yaw;
  if (yawCmd > 0) {
	  yawCmd = fmodf(yawCmd, 2 * M_PI);
  }
  if (yawCmd <= 0) {
	  yawCmd = -fmodf(-yawCmd, 2 * M_PI);
  }
  if (err < -M_PI) {
	  err = err + 2.f * M_PI;
  }
  if (err > M_PI) {
	  err = err - 2.f * M_PI;
  }

  float yawRateCmd= err*kpYaw;
  ////////////////////////////// BEGIN STUDENT CODE ///////////////////////////


  /////////////////////////////// END STUDENT CODE ////////////////////////////

  return yawRateCmd;

}

VehicleCommand QuadControl::RunControl(float dt, float simTime)
{
  curTrajPoint = GetNextTrajectoryPoint(simTime);

  float collThrustCmd = AltitudeControl(curTrajPoint.position.z, curTrajPoint.velocity.z, estPos.z, estVel.z, estAtt, curTrajPoint.accel.z, dt);

  // reserve some thrust margin for angle control
  float thrustMargin = .1f*(maxMotorThrust - minMotorThrust);
  collThrustCmd = CONSTRAIN(collThrustCmd, (minMotorThrust+ thrustMargin)*4.f, (maxMotorThrust-thrustMargin)*4.f);
  
  V3F desAcc = LateralPositionControl(curTrajPoint.position, curTrajPoint.velocity, estPos, estVel, curTrajPoint.accel);
  
  V3F desOmega = RollPitchControl(desAcc, estAtt, collThrustCmd);
  desOmega.z = YawControl(curTrajPoint.attitude.Yaw(), estAtt.Yaw());

  V3F desMoment = BodyRateControl(desOmega, estOmega);

  return GenerateMotorCommands(collThrustCmd, desMoment);
}