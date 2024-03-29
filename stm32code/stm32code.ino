// Copyright (c) 2019 Relativty
// Sarahghz stm32f103c8t6 Blue Pill release
// Version: 1.7
// Date:    02/04/2019

//=============================================================================
// I2Cdev must still be installed in libraries, 
// as MPU6050 requires the unpatched version to compile.
//=============================================================================



#include "Relativ.h"

#include "I2Cdev.h"

#include "MPU6050_6Axis_MotionApps20.h"

#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
    #include "Wire.h"
#endif

MPU6050 mpu;
Relativ relativ; 

#define INTERRUPT_PIN PA1
// set these to correct for drift - try small values ~0.000001
#define ALPHA 0.f // correction for drift - tilt channel
#define BETA  0.f // correction for drift - yaw channel

#define MPU6050_ACCEL_OFFSET_X -1499
#define MPU6050_ACCEL_OFFSET_Y -511
#define MPU6050_ACCEL_OFFSET_Z  671
#define MPU6050_GYRO_OFFSET_X  -19
#define MPU6050_GYRO_OFFSET_Y  -93
#define MPU6050_GYRO_OFFSET_Z   7

// IMU status and control:
bool dmpReady = false;  // true if DMP init was successful
uint8_t mpuIntStatus;   
uint8_t devStatus;      // 0 = success, !0 = error
uint16_t packetSize;    
uint16_t fifoCount;     
uint8_t fifoBuffer[64]; 

Quaternion q;           // [w, x, y, z]

volatile bool mpuInterrupt = false;     // indicates whether MPU interrupt pin has gone high
void dmpDataReady() {
    mpuInterrupt = true;
}

void setup() {
    #if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
        //Wire.setSDA(PB9);
        //Wire.setSCL(PB8);
        Wire.begin();
    #elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
        Fastwire::setup(400, true);
    #endif

    relativ.start();
    
    mpu.initialize();
    pinMode(INTERRUPT_PIN, INPUT);
    Serial.println(mpu.testConnection() ? F("MPU6050 connection successful") : F("MPU6050 connection failed"));

    // configure the DMP
    devStatus = mpu.dmpInitialize();

    // ==================================
    // supply your own gyro offsets here:
    // ==================================
    // follow procedure here to calibrate offsets https://github.com/kkpoon/CalibrateMPU6050
    mpu.setXAccelOffset(MPU6050_ACCEL_OFFSET_X);
    mpu.setYAccelOffset(MPU6050_ACCEL_OFFSET_Y);
    mpu.setZAccelOffset(MPU6050_ACCEL_OFFSET_Z);
    mpu.setXGyroOffset(MPU6050_GYRO_OFFSET_X);
    mpu.setYGyroOffset(MPU6050_GYRO_OFFSET_Y);
    mpu.setZGyroOffset(MPU6050_GYRO_OFFSET_Z);

    // devStatus if everything worked properly
    if (devStatus == 0) {
        // turn on the DMP, now that it's ready
        mpu.setDMPEnabled(true);

        // enable Arduino interrupt detection
        attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), dmpDataReady, RISING);
        mpuIntStatus = mpu.getIntStatus();

        dmpReady = true;

        // get expected DMP packet size for later comparison
        packetSize = mpu.dmpGetFIFOPacketSize();
    } else {
        // ERROR!
    }
}

void loop() {
    // Do nothing if DMP doesn't initialize correctly
    if (!dmpReady) return;

    // wait for MPU interrupt or extra packet(s) to be available
    while (!mpuInterrupt && fifoCount < packetSize) {
    }
    // reset interrupt flag and get INT_STATUS byte
    mpuInterrupt = false;
    mpuIntStatus = mpu.getIntStatus();

    // get current FIFO count
    fifoCount = mpu.getFIFOCount();

    if ((mpuIntStatus & 0x10) || fifoCount == 1024) {
        mpu.resetFIFO();
    }

    // check for interrupt
    else if (mpuIntStatus & 0x02) {
        while (fifoCount < packetSize) fifoCount = mpu.getFIFOCount();
        mpu.getFIFOBytes(fifoBuffer, packetSize);
        fifoCount -= packetSize;

        mpu.dmpGetQuaternion(&q, fifoBuffer);


        // find direction of gravity (down in world frame)
        VectorFloat down;
        mpu.dmpGetGravity(&down, &q);
        
        // tilt angle is the angle made between world-y and the "up"-vector (just negative of "down")
        float phi = acos( -down.y / down.getMagnitude() );

        // drift due to tilt
        Quaternion d_tilt( cos(phi/2.f), -down.getNormalized().z * sin(phi/2.f), 0.f, down.getNormalized().x * sin(phi/2.f) );

        // TO DO: if magnetometer readings are available, yaw drift can be accounted for
        // int16_t ax_tmp, ay_tmp, az_tmp, gx_tmp, gy_tmp, gz_tmp, mx, my, mz;
        // mpu.getMotion9(ax_tmp, ay_tmp, az_tmp, gx_tmp, gy_tmp, gz_tmp, mx, my, mz);
        // VectorFloat16 magn(float(mx), float(my), float(mz));
        // VectorFloat16 north = magn.rotate(q);
        // float beta = acos( -north.z / north.magnitude() );
        // Quaternion d_yaw( cos(beta/2.f), 0.f, sin(beta/2.f), 0.f );
        Quaternion d_yaw; // just default to identity if no magnetometer is available.

        // COMPLEMENTARY FILTER (YAW CORRECTION * TILT CORRECTION * QUATERNION FROM ABOVE)
        double tilt_correct = -ALPHA * 2.f * acos(d_tilt.w);
        double yaw_correct  = -BETA  * 2.f * acos(d_yaw.w);
        Quaternion tilt_correction( cos(tilt_correct/2.f), -down.getNormalized().getMagnitude() * sin(tilt_correct/2.f), 0.f, down.getNormalized().x * sin(tilt_correct/2.f) );
        Quaternion yaw_correction( cos(yaw_correct/2.f), 0.f, sin(yaw_correct/2.f), 0.f);

        // qc = yc * tc * q
        Quaternion qc(tilt_correction.w, tilt_correction.x, tilt_correction.y, tilt_correction.z);
        qc.getProduct(yaw_correction);
        qc.getProduct(tilt_correction);
        qc.getProduct(q);

        // report result
        relativ.updateOrientation(q.x, q.y, q.z, q.w, 4);  
    }
}
