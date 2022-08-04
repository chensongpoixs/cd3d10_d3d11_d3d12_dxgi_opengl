/********************************************************************
created:	2022-05-15

author:		chensong


purpose:	cd3dxx 
*********************************************************************/


#pragma once



#ifdef __cplusplus
extern "C" {
#else
#if defined(_MSC_VER) && !defined(inline)
#define inline __inline
#endif
#endif
 
extern bool hook_d3d9(void);
extern bool hook_dxgi(void);

#ifdef __cplusplus
}
#endif