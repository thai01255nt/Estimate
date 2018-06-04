# Estimation Project #

## Result at each step. ##

### Step 1. Sensor Noise ###

Excute simulator in scenario '06_SensorNoise'. Then it will generate 2 files record GPS and IMU.
I used code below to process these infomations and calculate standard deviation for GPS and IMU sensors.
 [Calculate_std_IMU.ipynb](./Calculate_std_IMU.ipynb)
 
 This is result when i run code:
 
 - GPS Std: 0.71
 - IMU Std: 0.488
 
 Result when i re-run simulator in scenario '06_SensorNoise'
 ![06_SensorNoise](./images/Step1_1.png)
 
 ![output](./images/step1_2.png)
 
### Step 2. Attitude Estimation ###

This step i need complete function `UpdateFromIMU()` (in line 74-129)
We need to implement a non-linear one to get good results. I used equation below (from controller project):

![Euler Angles Equation](./images/EulerAnglesEquation.gif)

And then use knowledge from section 7.2 of [Estimation for Quadrotors](https://www.overleaf.com/read/vymfngphcccj) to complete function `UpdateFromIMU()` with non-linear version.

This is result when i run scenario `07_AttitudeEstimation`
![07_AttitudeEstimation](./images/step2_1.png)

![output](./images/step2_2.png)

### Step 3. Prediction Step ###

#### 1. Predict ####

In that step, we predict current state from previous state and acceleration measurement.
Use equation (49) in section 7.2 of [Estimation for Quadrotors](https://www.overleaf.com/read/vymfngphcccj) to complete fuction `PredictState` (in line 154-190)

Result when i run scenario `08_PredictState`

![08_PredictState](./images/step3.png)
![output](./images/step3_2.png)

#### 2. Check the magnitude of the error ####

Run scenario `09_PredictCovariance` to check the magnitude of the error.
Result

![09_PredictCovariance](./images/step3_3)

### Step 4. Magnetometer Update ###

Use EKF (section 7.3.2 of [Estimation for Quadrotors](./images/https://www.overleaf.com/read/vymfngphcccj)) to combine predict yaw (from fuction `UpdateFromIMU`) with measurement yaw (from Magnetometer). And then update this value in function `UpdateFromMag`.

Beside, tune the parameter QYawStd (QuadEstimatorEKF.txt).

Result when i run scenario `10_MagUpdate` to test this function. (in line 343-370).

![10_MagUpdate](./images/step 4_1.png)
![output](./images/step 4_2.png)

### Step 5. Closed Loop + GPS Update ###

I set SimIMU.AccelStd & SimIMU.GyroStd like below:

`#SimIMU.AccelStd = 0.1,0.1,0.1`
`#SimIMU.GyroStd = 0.1,0.1,0.1`

And then i used equation (49) in section 7.3.1  of [Estimation for Quadrotors](https://www.overleaf.com/read/vymfngphcccj) to complete fuction `UpdateFromGPS` (in line 307-341).

Result when i run scenario `11_GPSUpdate` to test this function.

![11_GPSUpdate](./images/step 5_1.png)

### Step 6. Adding My Controller and test again scenario from previous project. ###

`02_AttitudeControl`

![02_AttitudeControl](./images/02.png)
![output](./images/output_02.png)

`03_PositionControl`

![03_PositionControl](./images/03.png)
![output](./images/output_03.png)

`04_Nonidealities`

![04_Nonidealities](./images/04.png)
![output](./images/output_04.png)

`05_TrajectoryFollow`

![05_TrajectoryFollow](./images/05.png)
![output](./images/output_05.png)



