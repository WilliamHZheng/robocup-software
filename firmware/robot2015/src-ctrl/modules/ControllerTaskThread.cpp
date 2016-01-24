#include <rtos.h>
#include <RPCVariable.h>

#include <Console.hpp>
#include <logger.hpp>
#include <assert.hpp>

#include "robot-devices.hpp"
#include "task-signals.hpp"
#include "task-globals.hpp"
#include "motors.hpp"
#include "fpga.hpp"
#include "mpu-6050.hpp"

// Keep this pretty high for now. Ideally, drop it down to ~3 for production
// builds. Hopefully that'll be possible without the console
static const int CONTROL_LOOP_WAIT_MS = 5;

// Declaration for an alternative control loop thread for when the accel/gyro
// can't be used for whatever reason
void Task_Controller_Sensorless(const osThreadId*);

namespace {
// The gyro/accel values are given RPC read/write access here
float gyroVals[3] = {0};
float accelVals[3] = {0};

// RPCVariable<float> gyrox(&gyroVals[0], "gyro-x");
// RPCVariable<float> gyroy(&gyroVals[1], "gyro-y");
// RPCVariable<float> gyroz(&gyroVals[2], "gyro-z");
// RPCVariable<float> accelx(&accelVals[0], "accel-x");
// RPCVariable<float> accely(&accelVals[1], "accel-y");
// RPCVariable<float> accelz(&accelVals[2], "accel-z");

// Making a temporary variable to test out the writing side of RPC variables
// int testVar;
// RPCVariable<int> test_var(&testVar, "var1");
}

/**
 * initializes the motion controller thread
 */
void Task_Controller(void const* args) {
    const osThreadId* mainID = (const osThreadId*)args;

    // Store the thread's ID
    osThreadId threadID = Thread::gettid();
    ASSERT(threadID != nullptr);

    // Store our priority so we know what to reset it to after running a command
    osPriority threadPriority = osThreadGetPriority(threadID);

    MPU6050 imu(RJ_I2C_BUS);

    imu.setBW(MPU6050_BW_256);
    imu.setGyroRange(MPU6050_GYRO_RANGE_250);
    imu.setAcceleroRange(MPU6050_ACCELERO_RANGE_2G);
    imu.setSleepMode(false);

    char testResp;
    if ((testResp = imu.testConnection())) {
        float resultRatio[6];
        imu.selfTest(resultRatio);
        LOG(INIT,
            "IMU self test results:\r\n"
            "    Accel (X,Y,Z):\t(%2.2f%%, %2.2f%%, %2.2f%%)\r\n"
            "    Gyro  (X,Y,Z):\t(%2.2f%%, %2.2f%%, %2.2f%%)",
            resultRatio[0], resultRatio[1], resultRatio[2], resultRatio[3],
            resultRatio[4], resultRatio[5]);

        LOG(INIT,
            "Control loop ready!\r\n    Thread ID:\t%u\r\n    Priority:\t%d",
            threadID, threadPriority);

        // Set the error code's valid bit
        imu_err |= 1 << 0;

    } else {
        LOG(SEVERE,
            "MPU6050 not found!\t(response: 0x%02X)\r\n    Falling back to "
            "sensorless control loop.",
            testResp);

        // Set the error flag - bit positions are pretty arbitruary as of now
        imu_err |= 1 << 1;
        // Set the error code's valid bit
        imu_err |= 1 << 0;

        // Start a thread that can function without the IMU, terminate us if it
        // ever returns
        Task_Controller_Sensorless(mainID);

        // should never reach this point
        osThreadTerminate(threadID);

        return;
    }

    // osThreadSetPriority(threadID, osPriorityNormal);

    // signal back to main and wait until we're signaled to continue
    osSignalSet((osThreadId)mainID, MAIN_TASK_CONTINUE);
    Thread::signal_wait(SUB_TASK_CONTINUE, osWaitForever);

    std::vector<uint16_t> duty_cycles;
    duty_cycles.assign(5, 100);
    for (int i = 0; i < duty_cycles.size(); ++i)
        duty_cycles.at(i) = 100 + 206 * i;

    

    while (true) {
        imu.getGyro(gyroVals);
        imu.getAccelero(accelVals);

        // printf(
        //     "\r\n\033[K"
        //     "\t(% 1.2f, % 1.2f, % 1.2f)\r\n"
        //     "\t(% 1.2f, % 1.2f, % 1.2f)\033[F\033[F",
        //     gyroVals[0], gyroVals[1], gyroVals[2], accelVals[0],
        //     accelVals[1],
        //     accelVals[2]);
        // Console::Flush();

        // write all duty cycles
        for (size_t i = 0; i < 500; ++i) {
            duty_cycles.at(1) = 2*i;
            FPGA::Instance()->set_duty_cycles(duty_cycles.data(), duty_cycles.size());
            Thread::wait(5);
        }

        Thread::wait(10);
        duty_cycles.at(1) = 0;
        FPGA::Instance()->set_duty_cycles(duty_cycles.data(), duty_cycles.size());

        Thread::wait(1500);

        Thread::wait(CONTROL_LOOP_WAIT_MS);
        // Thread::yield();
    }

    osThreadTerminate(threadID);
}

/**
 * [Task_Controller_Sensorless]
 * @param args [description]
 */
void Task_Controller_Sensorless(const osThreadId * mainID) {
    // Store the thread's ID
    osThreadId threadID = Thread::gettid();
    ASSERT(threadID != nullptr);

    // Store our priority so we know what to reset it to after running a command
    osPriority threadPriority = osThreadGetPriority(threadID);

    LOG(INIT,
        "Sensorless control loop ready!\r\n    Thread ID:\t%u\r\n    "
        "Priority:\t%d",
        threadID, threadPriority);

    // signal back to main and wait until we're signaled to continue
    osSignalSet((osThreadId)mainID, MAIN_TASK_CONTINUE);
    Thread::signal_wait(SUB_TASK_CONTINUE, osWaitForever);

    while (true) {
        Thread::wait(CONTROL_LOOP_WAIT_MS);
        Thread::yield();
    }

    osThreadTerminate(threadID);
}
