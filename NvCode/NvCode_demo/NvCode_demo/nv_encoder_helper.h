
#pragma once
#include "include/nvEncodeAPI.h"


#define bool int
#define true 1
#define false 0

typedef NVENCSTATUS(NVENCAPI *NV_MAX_VER_FUNC)(uint32_t *);

typedef NVENCSTATUS(NVENCAPI *NV_CREATE_INSTANCE_FUNC)(
	NV_ENCODE_API_FUNCTION_LIST *);

extern NV_CREATE_INSTANCE_FUNC nv_create_instance;
extern NV_ENCODE_API_FUNCTION_LIST nv;

bool nv_failed(  NVENCSTATUS err, const char *func,
	const char *call);



#define NV_FAILED(x) nv_failed(  x, __FUNCTION__, #x)



extern bool init_nvenc( );
extern bool load_nvenc_lib(void);
