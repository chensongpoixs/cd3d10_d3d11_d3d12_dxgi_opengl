#include "nv_encoder_helper.h"
#include "clog.h"


//////////////////////////////////////////////////////////////////
static void *nvenc_lib = NULL;
NV_CREATE_INSTANCE_FUNC nv_create_instance = NULL;

NV_ENCODE_API_FUNCTION_LIST nv = { NV_ENCODE_API_FUNCTION_LIST_VER };


/////////////////////////////


void *os_dlopen(const char *path)
{


	HMODULE h_library = NULL;


	h_library = LoadLibrary(TEXT(path));
	//h_library = LoadLibraryW((LPCWSTR)path);



	return h_library;
}

bool load_nvenc_lib(void)
{
	if (sizeof(void *) == 8)
	{
		DEBUG_EX_LOG("");
		nvenc_lib = os_dlopen("nvEncodeAPI64.dll");
	}
	else {
		nvenc_lib = os_dlopen("nvEncodeAPI.dll");
	}

	return !!nvenc_lib;
}



const char *nv_error_name(NVENCSTATUS err)
{
#define RETURN_CASE(x) \
	case x:        \
		return #x

	switch (err) {
		RETURN_CASE(NV_ENC_SUCCESS);
		RETURN_CASE(NV_ENC_ERR_NO_ENCODE_DEVICE);
		RETURN_CASE(NV_ENC_ERR_UNSUPPORTED_DEVICE);
		RETURN_CASE(NV_ENC_ERR_INVALID_ENCODERDEVICE);
		RETURN_CASE(NV_ENC_ERR_INVALID_DEVICE);
		RETURN_CASE(NV_ENC_ERR_DEVICE_NOT_EXIST);
		RETURN_CASE(NV_ENC_ERR_INVALID_PTR);
		RETURN_CASE(NV_ENC_ERR_INVALID_EVENT);
		RETURN_CASE(NV_ENC_ERR_INVALID_PARAM);
		RETURN_CASE(NV_ENC_ERR_INVALID_CALL);
		RETURN_CASE(NV_ENC_ERR_OUT_OF_MEMORY);
		RETURN_CASE(NV_ENC_ERR_ENCODER_NOT_INITIALIZED);
		RETURN_CASE(NV_ENC_ERR_UNSUPPORTED_PARAM);
		RETURN_CASE(NV_ENC_ERR_LOCK_BUSY);
		RETURN_CASE(NV_ENC_ERR_NOT_ENOUGH_BUFFER);
		RETURN_CASE(NV_ENC_ERR_INVALID_VERSION);
		RETURN_CASE(NV_ENC_ERR_MAP_FAILED);
		RETURN_CASE(NV_ENC_ERR_NEED_MORE_INPUT);
		RETURN_CASE(NV_ENC_ERR_ENCODER_BUSY);
		RETURN_CASE(NV_ENC_ERR_EVENT_NOT_REGISTERD);
		RETURN_CASE(NV_ENC_ERR_GENERIC);
		RETURN_CASE(NV_ENC_ERR_INCOMPATIBLE_CLIENT_KEY);
		RETURN_CASE(NV_ENC_ERR_UNIMPLEMENTED);
		RETURN_CASE(NV_ENC_ERR_RESOURCE_REGISTER_FAILED);
		RETURN_CASE(NV_ENC_ERR_RESOURCE_NOT_REGISTERED);
		RETURN_CASE(NV_ENC_ERR_RESOURCE_NOT_MAPPED);
	}
#undef RETURN_CASE

	return "Unknown Error";
}
bool nv_failed(  NVENCSTATUS err, const char *func,
	const char *call)
{
//	struct dstr error_message = { 0 };

	switch (err) {
	case NV_ENC_SUCCESS:
		return false;

	case NV_ENC_ERR_OUT_OF_MEMORY:
		ERROR_EX_LOG(  "NVENC.TooManySessions") ;
		break;

	case NV_ENC_ERR_UNSUPPORTED_DEVICE:
		ERROR_EX_LOG("NVENC.UnsupportedDevice") ;
		break;

	case NV_ENC_ERR_INVALID_VERSION:
		ERROR_EX_LOG("NVENC.OutdatedDriver") ;
		break;

	default:
		ERROR_EX_LOG( "NVENC Error: %s: %s failed: %d (%s)", func, call,
			(int)err, nv_error_name(err));
		 
	 
		break;
	}

	ERROR_EX_LOG("%s: %s failed: %d (%s)", func, call, (int)err,
		nv_error_name(err));
	return true;
}

//#define NV_FAILED(e, x) nv_failed(e, x, __FUNCTION__, #x)


void *os_dlsym(void *module, const char *func)
{
	void *handle;

	handle = (void *)GetProcAddress(module, func);

	return handle;
}
static void *load_nv_func(const char *func)
{
	void *func_ptr = os_dlsym(nvenc_lib, func);
	if (!func_ptr)
	{
		ERROR_EX_LOG("Could not load function: %s", func);
	}
	return func_ptr;
}

static inline bool init_nvenc_internal()
{
	static bool initialized = false;
	static bool success = false;

	if (initialized)
		return success;
	initialized = true;

	NV_MAX_VER_FUNC nv_max_ver = (NV_MAX_VER_FUNC)load_nv_func("NvEncodeAPIGetMaxSupportedVersion");
	if (!nv_max_ver)
	{
		ERROR_EX_LOG("Missing NvEncodeAPIGetMaxSupportedVersion, check "
			"your video card drivers are up to date.");
		return false;
	}

	uint32_t ver = 0;
	if (NV_FAILED(nv_max_ver(&ver)))
	{
		return false;
	}

	DEBUG_EX_LOG("nvencoder version = %u", ver);

	uint32_t cur_ver = (NVENCAPI_MAJOR_VERSION << 4) |
		NVENCAPI_MINOR_VERSION;
	if (cur_ver > ver)
	{
		ERROR_EX_LOG("NVENC.OutdatedDriver");

		ERROR_EX_LOG("Current driver version does not support this NVENC "
			"version, please upgrade your driver");
		return false;
	}

	nv_create_instance = (NV_CREATE_INSTANCE_FUNC)load_nv_func(
		"NvEncodeAPICreateInstance");
	if (!nv_create_instance)
	{
		ERROR_EX_LOG("Missing NvEncodeAPICreateInstance, check "
			"your video card drivers are up to date.");
		return false;
	}

	if (NV_FAILED(nv_create_instance(&nv)))
	{
		ERROR_EX_LOG("nv_create_instance failed !!!");
		return false;
	}

	success = true;
	return true;
}

bool init_nvenc()
{
	bool success;

	//pthread_mutex_lock(&init_mutex);
	success = init_nvenc_internal();
	//pthread_mutex_unlock(&init_mutex);

	return success;
}