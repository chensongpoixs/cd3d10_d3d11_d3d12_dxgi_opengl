#pragma once
 
	

	void LOG(const char* format, ...);

#define WARNING_EX_LOG(format, ...)	LOG("[%s][%d][warn]" format, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define DEBUG_EX_LOG(format, ...)   LOG("[%s][%d][debug]" format, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define ERROR_EX_LOG(format, ...)   LOG("[%s][%d][error]" format, __FUNCTION__, __LINE__, ##__VA_ARGS__)
 

