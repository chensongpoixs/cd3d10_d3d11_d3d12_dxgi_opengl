#define _CRT_SECURE_NO_WARNINGS
#include "clog.h"
//#include <iostream>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
 


	static const char* g_ccapture_hook_file_name = "./nvcodeer_demo.log";

	static FILE* out_file_log_ptr = NULL;
	static inline void SHOW(const char* format, va_list args)
	{
		if (!out_file_log_ptr)
		{
			out_file_log_ptr =  fopen(g_ccapture_hook_file_name, "wb+");
		}
		if (!out_file_log_ptr)
		{
			return;
		}
		char message[1024] = { 0 };

		int num = _vsprintf_p(message, 1024, format, args);
		if (num > 0)
		{
			 fprintf(out_file_log_ptr, "%s\n", message);
			 printf(   "%s\n", message);
			 fflush(out_file_log_ptr);
			 //fflush(stdout);
		}
	}
	void LOG(const char* format, ...)
	{


		va_list args;
		va_start(args, format);

		SHOW(format, args);
		va_end(args);

	}
 
