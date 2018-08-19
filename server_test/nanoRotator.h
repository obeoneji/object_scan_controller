#pragma once
#include <iostream>
#include "Thorlabs.MotionControl.Benchtop.StepperMotor.h"



int connect_nanoRotator(char* rotSerialNo);
int init_nanoRotator(char* rotSerialNo, int initPos, int initVelc, int initAcc);
int rotate_nanoRotator(char* rotSerialNo, int ang);
int stop_nanoRotator(char* rotSerialNo);