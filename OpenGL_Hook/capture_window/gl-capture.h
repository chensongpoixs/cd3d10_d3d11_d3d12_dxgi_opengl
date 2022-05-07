#pragma once



#ifdef __cplusplus
extern "C" {
#else
#if defined(_MSC_VER) && !defined(inline)
#define inline __inline
#endif
#endif
	bool hook_gl();
#ifdef __cplusplus
}
#endif