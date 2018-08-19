#include"nanoRotator.h"




int connect_nanoRotator(char* rotSerialNo)
{
	if (TLI_BuildDeviceList() != 0)
	{
		std::cout << "error!!! no device on the desktop!!! " << std::endl;
		return -1;
	}

	// get device list size 
	short n = TLI_GetDeviceListSize();
	std::cout << n << std::endl;
	// get BBD serial numbers
	char serialNos[100];
	TLI_GetDeviceListByTypeExt(serialNos, 100, 40);

	// output list of matching devices
	char *p = strtok(serialNos, ",");
	while (p != NULL)
	{
		TLI_DeviceInfo deviceInfo;
		// get device info from device
		TLI_GetDeviceInfo(p, &deviceInfo);
		// get strings from device info structure
		char desc[65];
		strncpy(desc, deviceInfo.description, 64);
		desc[64] = '\0';
		char serialNo[9];
		strncpy(serialNo, deviceInfo.serialNo, 8);
		serialNo[8] = '\0';
		// output
		printf("Found Device %s=%s : %s\r\n", p, serialNo, desc);
		p = strtok(NULL, ",");
	}

	if (SBC_Open(rotSerialNo) == 0)
	{
		return 0;
	}
	else
	{
		std::cout << "error!!! nanoRotator cannot open!!! " << std::endl;
		return -1;
	}

}

int init_nanoRotator(char* rotSerialNo, int initPos, int initVelc, int initAcc)
{
	// start the device polling at 200ms intervals
	SBC_StartPolling(rotSerialNo, 1, 200);

	bool needHoming = SBC_CanMoveWithoutHomingFirst(rotSerialNo, 1);
	std::cout << "Can this device be moved without Homing? " << needHoming << std::endl;

	double stepsPerRev;
	double gearBoxRatio;
	double pitch;

	SBC_GetMotorParamsExt(rotSerialNo, 1, &stepsPerRev, &gearBoxRatio, &pitch);
	std::cout << "stepsPerRev, gearBoxRatio, pitch: " << stepsPerRev << " " << gearBoxRatio << " " << pitch << std::endl;

	//int initPos = SBC_GetPosition(rotSerialNo, 1);
	//std::cout << "initPos: " << initPos << std::endl;

	SBC_SetMotorParamsExt(rotSerialNo, 1, 409600.0, 66.0, 1.0);
	SBC_GetMotorParamsExt(rotSerialNo, 1, &stepsPerRev, &gearBoxRatio, &pitch);
	std::cout << "stepsPerRev, gearBoxRatio, pitch: " << stepsPerRev << " " << gearBoxRatio << " " << pitch << std::endl;

	//initPos = SBC_GetPosition(rotSerialNo, 1);
	//std::cout << "initPos: " << initPos << std::endl;

	double rv;
	int getrv = SBC_GetRealValueFromDeviceUnit(rotSerialNo, 1, 3754777, &rv, 1);
	std::cout << "real value from device unit: " << rv << std::endl;
	std::cout << "getrv: " << getrv << std::endl;

	int du;
	int getdu = SBC_GetDeviceUnitFromRealValue(rotSerialNo, 1, 1000.0, &du, 0);
	std::cout << "device unit for real value 1: " << du << std::endl;
	std::cout << "getdu: " << getdu << std::endl;

	Sleep(3000);

	// Home device
	SBC_ClearMessageQueue(rotSerialNo, 1);
	SBC_Home(rotSerialNo, 1);
	printf("Device %s homing\r\n", rotSerialNo);

	// wait for completion
	WORD messageType;
	WORD messageId;
	DWORD messageData;
	SBC_WaitForMessage(rotSerialNo, 1, &messageType, &messageId, &messageData);
	while (messageType != 3 || messageId != 1)
	{
		SBC_WaitForMessage(rotSerialNo, 1, &messageType, &messageId, &messageData);
	}

	// set velocity if desired
	if (initVelc > 0)
	{
		int currentVelocity, currentAcceleration;
		SBC_GetVelParams(rotSerialNo, 1, &currentVelocity, &currentAcceleration);
		std::cout << "velocity and acceleration: " << currentVelocity << " " << currentAcceleration << std::endl;
		SBC_SetVelParams(rotSerialNo, 1, initVelc, initAcc);
		SBC_GetVelParams(rotSerialNo, 1, &currentVelocity, &currentAcceleration);
		std::cout << "velocity and acceleration: " << currentVelocity << " " << currentAcceleration << std::endl;
	}

	// move to position (channel 1)
	SBC_ClearMessageQueue(rotSerialNo, 1);
	//SBC_RequestPosition(testSerialNo, 1);
	//Sleep(100);
	int currPos;
	currPos = SBC_GetPosition(rotSerialNo, 1);
	std::cout << currPos << std::endl;

	//GoToPosition(testSerialNo, 1, position);
	bool suc = SBC_MoveToPosition(rotSerialNo, 1, initPos);

	//SBC_RequestPosition(testSerialNo, 1);
	//Sleep(100);
	currPos = SBC_GetPosition(rotSerialNo, 1);
	std::cout << currPos << std::endl;
	printf("Device %s moving\r\n", rotSerialNo);


	// wait for completion
	SBC_WaitForMessage(rotSerialNo, 1, &messageType, &messageId, &messageData);
	while (messageType != 3 || messageId != 1)
	{
		SBC_WaitForMessage(rotSerialNo, 1, &messageType, &messageId, &messageData);
	}

	// get actual poaition
	currPos = SBC_GetPosition(rotSerialNo, 1);
	printf("Device %s moved to %d\r\n", rotSerialNo, currPos);

	printf("Device %s init Done!!!\n", rotSerialNo);
	return 0;

}



int rotate_nanoRotator(char* rotSerialNo, int ang)
{
	WORD messageType;
	WORD messageId;
	DWORD messageData;

	int currPos;

	int add_pos = ang / 360 * 66 * 409600;

	currPos = SBC_GetPosition(rotSerialNo, 1);
	std::cout << currPos << std::endl;

	bool suc = SBC_MoveToPosition(rotSerialNo, 1, currPos + ang);

	SBC_WaitForMessage(rotSerialNo, 1, &messageType, &messageId, &messageData);
	while (messageType != 3 || messageId != 1)
	{
		SBC_WaitForMessage(rotSerialNo, 1, &messageType, &messageId, &messageData);
	}

	// get actual poaition
	currPos = SBC_GetPosition(rotSerialNo, 1);
	printf("Device %s moved to %d\r\n", rotSerialNo, currPos);

	printf("Device %s Move Done!!!\n", rotSerialNo);
	return 0;
}

int stop_nanoRotator(char* rotSerialNo)
{
	// stop polling
	SBC_StopPolling(rotSerialNo, 1);
	// close device
	SBC_Close(rotSerialNo);

	return 0;
}

