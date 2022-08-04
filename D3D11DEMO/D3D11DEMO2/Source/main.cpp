/***************************************************************************
					选择GPU的技术验证demo

******************************************************************************/


////////////////////////////////////////////////////////////////////////////////
// Filename: main.cpp
////////////////////////////////////////////////////////////////////////////////
#include "Systemclass.h"

//int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nShowCmd)
int main(int argc, char *argv[])
{

	if (argc > 1)
	{
		g_gpu_index = std::atoi( argv[1]);
		if (argc > 2)
		{
			g_monitor_index = std::atoi(argv[1]);
		}
	}
	SystemClass* System;
	bool result;

	// Create the system object
	System = new SystemClass;
	if (!System)
	{
		return 0;
	}

	// Initialize and run the system object.
	result = System->Initialize();
	if (result)
	{
		System->Run();
	}

	// Shutdown and release the system object.
	System->Shutdown();
	delete System;
	System = nullptr;
	
	return 0;
}