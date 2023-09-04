#ifndef _C_SHARE_HANDLER_H_
#define _C_SHARE_HANDLER_H_

#include <stdint.h>
#include <stdbool.h>


 
#ifdef __cplusplus
extern "C" {
#else
#if defined(_MSC_VER) && !defined(inline)
#define inline __inline
#endif
#endif
 
	bool __declspec(dllimport) gl_shared_init(uint32_t width, uint32_t height);
	void __declspec(dllimport) gl_shared_capture(uint32_t width, uint32_t height);
	void __declspec(dllimport) gl_shared_destroy();
#ifdef __cplusplus
}
#endif






#endif // _C_SHARE_HANDLER_H_