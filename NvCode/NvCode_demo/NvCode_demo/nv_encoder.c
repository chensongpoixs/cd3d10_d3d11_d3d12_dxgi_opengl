//#include <cstdio>
//#include <thread>
//#include <mutex>
#include "include/nvEncodeAPI.h"
//#include <iostream>
#include <typeinfo.h>
#include <windows.h>
#include <mmsystem.h>
#include <shellapi.h>
#include <shlobj.h>
#include <intrin.h>
#include <psapi.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <intrin.h>
#include "clog.h"

//#define INITGUID
#include <dxgi.h>
#include <d3d11.h>
#include <d3d11_1.h>







/*
 * Defines to make dynamic arrays more type-safe.
 * Note: Still not 100% type-safe but much better than using darray directly
 *       Makes it a little easier to use as well.
 *
 *       I did -not- want to use a gigantic macro to generate a crapload of
 *       typesafe inline functions per type.  It just feels like a mess to me.
 */

#define DARRAY(type)                     \
	union {                          \
		struct darray da;        \
		struct {                 \
			type *array;     \
			size_t num;      \
			size_t capacity; \
		};                       \
	}
/* Main Implementation Structure                                             */

struct nvenc_data {
	  
	void *session;
	NV_ENC_INITIALIZE_PARAMS params;
	NV_ENC_CONFIG config;
	int rc_lookahead;
	int buf_count;
	int output_delay;
	int buffers_queued;
	size_t next_bitstream;
	size_t cur_bitstream;
	bool encode_started;
	bool first_packet;
	bool can_change_bitrate;
	int32_t bframes;

	/*DARRAY(struct nv_bitstream) bitstreams;
	DARRAY(struct nv_texture) textures;
	DARRAY(struct handle_tex) input_textures;
	struct circlebuf dts_list;*/

	//DARRAY(uint8_t) packet_data;
	int64_t packet_pts;
	bool packet_keyframe;

	ID3D11Device *device;
	ID3D11DeviceContext *context;

	uint32_t cx;
	uint32_t cy;

	uint8_t *header;
	size_t header_size;

	uint8_t *sei;
	size_t sei_size;
};

 
typedef NVENCSTATUS(NVENCAPI *NV_MAX_VER_FUNC)(uint32_t *);
NV_ENCODE_API_FUNCTION_LIST nv = { NV_ENCODE_API_FUNCTION_LIST_VER };
typedef NVENCSTATUS(NVENCAPI *NV_CREATE_INSTANCE_FUNC)(
	NV_ENCODE_API_FUNCTION_LIST *);

NV_CREATE_INSTANCE_FUNC nv_create_instance = NULL;
static void *nvenc_lib = NULL;

 
//////////////////////


bool nv_failed(NVENCSTATUS err, const char *func, const char *call)
{
	struct dstr error_message = { 0 };

	switch (err) {
	case NV_ENC_SUCCESS:
		return false;

	case NV_ENC_ERR_OUT_OF_MEMORY:
		ERROR_EX_LOG( "NVENC.TooManySessions");
		break;

	case NV_ENC_ERR_UNSUPPORTED_DEVICE:
		ERROR_EX_LOG( "NVENC.UnsupportedDevice");
		break;

	case NV_ENC_ERR_INVALID_VERSION:
		ERROR_EX_LOG( "NVENC.OutdatedDriver");
		break;

	default:
		ERROR_EX_LOG("");
		/*ERROR_EX_LOG( 
			"NVENC Error: %s: %s failed: %d (%s)", func, call,
			(int)err, nv_error_name(err));*/
		 
		break;
	}

	 
	return true;
}

#define NV_FAILED(x) nv_failed( x, __FUNCTION__, #x)
struct dstr {
	char *array;
	size_t len; /* number of characters, excluding null terminator */
	size_t capacity;
};

void *os_dlopen(const char *path)
{
	struct dstr dll_name;
	wchar_t *wpath;
	wchar_t *wpath_slash;
	HMODULE h_library = NULL;

	//if (!path)
	//	return NULL;

	//dstr_init_copy(&dll_name, path);
	//dstr_replace(&dll_name, "\\", "/");
	//if (!dstr_find(&dll_name, ".dll"))
	//	dstr_cat(&dll_name, ".dll");
	//os_utf8_to_wcs_ptr(dll_name.array, 0, &wpath);

	//std::free(&dll_name);

	///* to make module dependency issues easier to deal with, allow
	// * dynamically loaded libraries on windows to search for dependent
	// * libraries that are within the library's own directory */
	//wpath_slash = wcsrchr(wpath, L'/');
	//if (wpath_slash) {
	//	*wpath_slash = 0;
	//	SetDllDirectoryW(wpath);
	//	*wpath_slash = L'/';
	//}

	h_library = LoadLibraryW(wpath);

	//std::free(wpath);

	//if (wpath_slash)
	//	SetDllDirectoryW(NULL);

	//if (!h_library) {
	//	DWORD error = GetLastError();

	//	/* don't print error for libraries that aren't meant to be
	//	 * dynamically linked */
	//	if (error == ERROR_PROC_NOT_FOUND)
	//		return NULL;

	//	char *message = NULL;

	//	FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM |
	//		FORMAT_MESSAGE_IGNORE_INSERTS |
	//		FORMAT_MESSAGE_ALLOCATE_BUFFER,
	//		NULL, error,
	//		MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
	//		(LPSTR)&message, 0, NULL);

	//	DEBUG_EX_LOG("LoadLibrary failed for '%s': %s (%lu)", path,
	//		message, error);

	//	if (message)
	//		LocalFree(message);
	//}

	return h_library;
}

bool load_nvenc_lib(void)
{
	if (sizeof(void *) == 8) {
		nvenc_lib = os_dlopen("nvEncodeAPI64.dll");
	}
	else {
		nvenc_lib = os_dlopen("nvEncodeAPI.dll");
	}

	return !!nvenc_lib;
}
static HMODULE get_lib(struct nvenc_data *enc, const char *lib)
{
	HMODULE mod = GetModuleHandleA(lib);
	if (mod)
	{
		return mod;
	}

	mod = LoadLibraryA(lib);
	if (!mod)
	{
		ERROR_EX_LOG("Failed to load %s", lib);
	}
	return mod;
}
void *os_dlsym(void *module, const char *func)
{
	void *handle;

	handle = (void *)GetProcAddress((HMODULE)module, func);

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

static inline bool init_nvenc_internal( )
{
	static bool initialized = false;
	static bool success = false;

	if (initialized)
		return success;
	initialized = true;

	NV_MAX_VER_FUNC nv_max_ver = (NV_MAX_VER_FUNC)load_nv_func(
		"NvEncodeAPIGetMaxSupportedVersion");
	if (!nv_max_ver) {
		ERROR_EX_LOG( 
			"Missing NvEncodeAPIGetMaxSupportedVersion, check "
			"your video card drivers are up to date.");
		return false;
	}

	uint32_t ver = 0;
	if (NV_FAILED( nv_max_ver(&ver))) {
		return false;
	}

	uint32_t cur_ver = (NVENCAPI_MAJOR_VERSION << 4) |
		NVENCAPI_MINOR_VERSION;
	if (cur_ver > ver) 
	{
		ERROR_EX_LOG( "NVENC.OutdatedDriver");

		ERROR_EX_LOG("Current driver version does not support this NVENC "
			"version, please upgrade your driver");
		return false;
	}

	nv_create_instance = (NV_CREATE_INSTANCE_FUNC)load_nv_func(
		"NvEncodeAPICreateInstance");
	if (!nv_create_instance) {
		ERROR_EX_LOG(  "Missing NvEncodeAPICreateInstance, check "
			"your video card drivers are up to date.");
		return false;
	}

	if (NV_FAILED( nv_create_instance(&nv))) 
	{
		return false;
	}

	success = true;
	return true;
}

bool init_nvenc( )
{
	bool success;

	clock_guard lock(g_mutex);
	success = init_nvenc_internal( );
	 

	return success;
}

typedef HRESULT(WINAPI *CREATEDXGIFACTORY1PROC)(REFIID, void **);


static bool init_d3d11(struct nvenc_data *enc )
{
	HMODULE dxgi = get_lib(enc, "DXGI.dll");
	HMODULE d3d11 = get_lib(enc, "D3D11.dll");
	CREATEDXGIFACTORY1PROC create_dxgi;
	PFN_D3D11_CREATE_DEVICE create_device;
	IDXGIFactory1 *factory;
	IDXGIAdapter *adapter;
	ID3D11Device *device;
	ID3D11DeviceContext *context;
	HRESULT hr;

	if (!dxgi || !d3d11) {
		return false;
	}

	create_dxgi = (CREATEDXGIFACTORY1PROC)GetProcAddress(
		dxgi, "CreateDXGIFactory1");
	create_device = (PFN_D3D11_CREATE_DEVICE)GetProcAddress(
		d3d11, "D3D11CreateDevice");

	if (!create_dxgi || !create_device)
	{
		ERROR_EX_LOG("Failed to load D3D11/DXGI procedures");
		return false;
	}

	hr = create_dxgi(__uuidof(IDXGIFactory1), (void **)&factory);
	if (FAILED(hr)) 
	{
		ERROR_EX_LOG("CreateDXGIFactory1 failed");
		return false;
	}

	hr = factory-> EnumAdapters(  0, &adapter);
	factory-> Release();
	if (FAILED(hr)) {
		ERROR_EX_LOG("EnumAdapters failed");
		return false;
	}

	hr = create_device(adapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, 0, NULL, 0,
		D3D11_SDK_VERSION, &device, NULL, &context);
	adapter-> Release();
	if (FAILED(hr)) {
		ERROR_EX_LOG("D3D11CreateDevice failed");
		return false;
	}

	enc->device = device;
	enc->context = context;
	return true;
}

static bool init_session(struct nvenc_data *enc)
{
	NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS params = {
		NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS_VER };
	params.device = enc->device;
	params.deviceType = NV_ENC_DEVICE_TYPE_DIRECTX;
	params.apiVersion = NVENCAPI_VERSION;

	if (NV_FAILED(nv.nvEncOpenEncodeSessionEx(&params, &enc->session))) {
		return false;
	}
	return true;
}

static bool init_encoder(struct nvenc_data *enc, obs_data_t *settings,
	bool psycho_aq)
{
	const char *rc = obs_data_get_string(settings, "rate_control");
	int bitrate = (int)obs_data_get_int(settings, "bitrate");
	int max_bitrate = (int)obs_data_get_int(settings, "max_bitrate");
	int cqp = (int)obs_data_get_int(settings, "cqp");
	int keyint_sec = (int)obs_data_get_int(settings, "keyint_sec");
	const char *preset = obs_data_get_string(settings, "preset");
	const char *profile = obs_data_get_string(settings, "profile");
	bool lookahead = obs_data_get_bool(settings, "lookahead");
	int bf = (int)obs_data_get_int(settings, "bf");
	bool vbr = astrcmpi(rc, "VBR") == 0;
	NVENCSTATUS err;

	video_t *video = obs_encoder_video(enc->encoder);
	const struct video_output_info *voi = video_output_get_info(video);

	enc->cx = voi->width;
	enc->cy = voi->height;

	/* -------------------------- */
	/* get preset                 */

	GUID nv_preset = NV_ENC_PRESET_DEFAULT_GUID;
	bool twopass = false;
	bool hp = false;
	bool ll = false;

	if (astrcmpi(preset, "hq") == 0) {
		nv_preset = NV_ENC_PRESET_HQ_GUID;

	}
	else if (astrcmpi(preset, "mq") == 0) {
		nv_preset = NV_ENC_PRESET_HQ_GUID;
		twopass = true;

	}
	else if (astrcmpi(preset, "hp") == 0) {
		nv_preset = NV_ENC_PRESET_HP_GUID;
		hp = true;

	}
	else if (astrcmpi(preset, "ll") == 0) {
		nv_preset = NV_ENC_PRESET_LOW_LATENCY_DEFAULT_GUID;
		ll = true;

	}
	else if (astrcmpi(preset, "llhq") == 0) {
		nv_preset = NV_ENC_PRESET_LOW_LATENCY_HQ_GUID;
		ll = true;

	}
	else if (astrcmpi(preset, "llhp") == 0) {
		nv_preset = NV_ENC_PRESET_LOW_LATENCY_HP_GUID;
		hp = true;
		ll = true;
	}

	const bool rc_lossless = astrcmpi(rc, "lossless") == 0;
	bool lossless = rc_lossless;
	if (rc_lossless) {
		lossless = nv_get_cap(enc, NV_ENC_CAPS_SUPPORT_LOSSLESS_ENCODE);
		if (lossless) {
			nv_preset = hp ? NV_ENC_PRESET_LOSSLESS_HP_GUID
				: NV_ENC_PRESET_LOSSLESS_DEFAULT_GUID;
		}
		else {
			warn("lossless encode is not supported, ignoring");
		}
	}

	/* -------------------------- */
	/* get preset default config  */

	NV_ENC_PRESET_CONFIG preset_config = { NV_ENC_PRESET_CONFIG_VER,
						  {NV_ENC_CONFIG_VER} };

	err = nv.nvEncGetEncodePresetConfig(enc->session,
		NV_ENC_CODEC_H264_GUID, nv_preset,
		&preset_config);
	if (nv_failed(enc->encoder, err, __FUNCTION__,
		"nvEncGetEncodePresetConfig")) {
		return false;
	}

	/* -------------------------- */
	/* main configuration         */

	enc->config = preset_config.presetCfg;

	uint32_t gop_size =
		(keyint_sec) ? keyint_sec * voi->fps_num / voi->fps_den : 250;

	NV_ENC_INITIALIZE_PARAMS *params = &enc->params;
	NV_ENC_CONFIG *config = &enc->config;
	NV_ENC_CONFIG_H264 *h264_config = &config->encodeCodecConfig.h264Config;
	NV_ENC_CONFIG_H264_VUI_PARAMETERS *vui_params =
		&h264_config->h264VUIParameters;

	int darWidth, darHeight;
	av_reduce(&darWidth, &darHeight, voi->width, voi->height, 1024 * 1024);

	memset(params, 0, sizeof(*params));
	params->version = NV_ENC_INITIALIZE_PARAMS_VER;
	params->encodeGUID = NV_ENC_CODEC_H264_GUID;
	params->presetGUID = nv_preset;
	params->encodeWidth = voi->width;
	params->encodeHeight = voi->height;
	params->darWidth = darWidth;
	params->darHeight = darHeight;
	params->frameRateNum = voi->fps_num;
	params->frameRateDen = voi->fps_den;
	params->enableEncodeAsync = 1;
	params->enablePTD = 1;
	params->encodeConfig = &enc->config;
	config->gopLength = gop_size;
	config->frameIntervalP = 1 + bf;
	h264_config->idrPeriod = gop_size;

	bool repeat_headers = obs_data_get_bool(settings, "repeat_headers");
	if (repeat_headers) {
		h264_config->repeatSPSPPS = 1;
		h264_config->disableSPSPPS = 0;
		h264_config->outputAUD = 1;
	}

	h264_config->sliceMode = 3;
	h264_config->sliceModeData = 1;

	h264_config->useBFramesAsRef = NV_ENC_BFRAME_REF_MODE_DISABLED;

	vui_params->videoSignalTypePresentFlag = 1;
	vui_params->videoFullRangeFlag = (voi->range == VIDEO_RANGE_FULL);
	vui_params->colourDescriptionPresentFlag = 1;

	switch (voi->colorspace) {
	case VIDEO_CS_601:
		vui_params->colourPrimaries = 6;
		vui_params->transferCharacteristics = 6;
		vui_params->colourMatrix = 6;
		break;
	case VIDEO_CS_DEFAULT:
	case VIDEO_CS_709:
		vui_params->colourPrimaries = 1;
		vui_params->transferCharacteristics = 1;
		vui_params->colourMatrix = 1;
		break;
	case VIDEO_CS_SRGB:
		vui_params->colourPrimaries = 1;
		vui_params->transferCharacteristics = 13;
		vui_params->colourMatrix = 1;
		break;
	}

	enc->bframes = bf;

	/* lookahead */
	const bool use_profile_lookahead = config->rcParams.enableLookahead;
	lookahead = nv_get_cap(enc, NV_ENC_CAPS_SUPPORT_LOOKAHEAD) &&
		(lookahead || use_profile_lookahead);
	if (lookahead) {
		enc->rc_lookahead = use_profile_lookahead
			? config->rcParams.lookaheadDepth
			: 8;
	}

	int buf_count = max(4, config->frameIntervalP * 2 * 2);
	if (lookahead) {
		buf_count = max(buf_count, config->frameIntervalP +
			enc->rc_lookahead +
			EXTRA_BUFFERS);
	}

	buf_count = min(64, buf_count);
	enc->buf_count = buf_count;

	const int output_delay = buf_count - 1;
	enc->output_delay = output_delay;

	if (lookahead) {
		const int lkd_bound = output_delay - config->frameIntervalP - 4;
		if (lkd_bound >= 0) {
			config->rcParams.enableLookahead = 1;
			config->rcParams.lookaheadDepth =
				max(enc->rc_lookahead, lkd_bound);
			config->rcParams.disableIadapt = 0;
			config->rcParams.disableBadapt = 0;
		}
		else {
			lookahead = false;
		}
	}

	/* psycho aq */
	if (nv_get_cap(enc, NV_ENC_CAPS_SUPPORT_TEMPORAL_AQ)) {
		config->rcParams.enableAQ = psycho_aq;
		config->rcParams.aqStrength = 8;
		config->rcParams.enableTemporalAQ = psycho_aq;
	}
	else if (psycho_aq) {
		WARNING_EX_LOG("Ignoring Psycho Visual Tuning request since GPU is not capable");
	}

	/* -------------------------- */
	/* rate control               */

	enc->can_change_bitrate =
		nv_get_cap(enc, NV_ENC_CAPS_SUPPORT_DYN_BITRATE_CHANGE) &&
		!lookahead;

	config->rcParams.rateControlMode = twopass ? NV_ENC_PARAMS_RC_VBR_HQ
		: NV_ENC_PARAMS_RC_VBR;

	if (astrcmpi(rc, "cqp") == 0 || rc_lossless) {
		if (lossless) {
			h264_config->qpPrimeYZeroTransformBypassFlag = 1;
			cqp = 0;
		}

		config->rcParams.rateControlMode = NV_ENC_PARAMS_RC_CONSTQP;
		config->rcParams.constQP.qpInterP = cqp;
		config->rcParams.constQP.qpInterB = cqp;
		config->rcParams.constQP.qpIntra = cqp;
		enc->can_change_bitrate = false;

		bitrate = 0;
		max_bitrate = 0;

	}
	else if (astrcmpi(rc, "vbr") != 0) { /* CBR by default */
		h264_config->outputBufferingPeriodSEI = 1;
		config->rcParams.rateControlMode =
			twopass ? NV_ENC_PARAMS_RC_2_PASS_QUALITY
			: NV_ENC_PARAMS_RC_CBR;
	}

	h264_config->outputPictureTimingSEI = 1;
	config->rcParams.averageBitRate = bitrate * 1000;
	config->rcParams.maxBitRate = vbr ? max_bitrate * 1000 : bitrate * 1000;
	config->rcParams.vbvBufferSize = bitrate * 1000;

	/* -------------------------- */
	/* profile                    */

	if (astrcmpi(profile, "main") == 0) {
		config->profileGUID = NV_ENC_H264_PROFILE_MAIN_GUID;
	}
	else if (astrcmpi(profile, "baseline") == 0) {
		config->profileGUID = NV_ENC_H264_PROFILE_BASELINE_GUID;
	}
	else if (!lossless) {
		config->profileGUID = NV_ENC_H264_PROFILE_HIGH_GUID;
	}

	/* -------------------------- */
	/* initialize                 */

	if (NV_FAILED(nv.nvEncInitializeEncoder(enc->session, params))) {
		return false;
	}

	DEBUG_EX_LOG("settings:\n"
		"\trate_control: %s\n"
		"\tbitrate:      %d\n"
		"\tcqp:          %d\n"
		"\tkeyint:       %d\n"
		"\tpreset:       %s\n"
		"\tprofile:      %s\n"
		"\twidth:        %d\n"
		"\theight:       %d\n"
		"\t2-pass:       %s\n"
		"\tb-frames:     %d\n"
		"\tlookahead:    %s\n"
		"\tpsycho_aq:    %s\n",
		rc, bitrate, cqp, gop_size, preset, profile, enc->cx, enc->cy,
		twopass ? "true" : "false", bf, lookahead ? "true" : "false",
		psycho_aq ? "true" : "false");

	return true;
}


static void *nvenc_create_internal( )
{
	NV_ENCODE_API_FUNCTION_LIST init = { NV_ENCODE_API_FUNCTION_LIST_VER };
	struct nvenc_data *enc = new struct nvenc_data;
	 
	enc->first_packet = true;

	if (!init_nvenc())
	{
		goto fail;
	}
	if (NV_FAILED(nv_create_instance(&init))) {
		goto fail;
	}
	if (!init_d3d11(enc)) {
		goto fail;
	}
	if (!init_session(enc)) {
		goto fail;
	}
	if (!init_encoder(enc, settings, psycho_aq)) {
		goto fail;
	}
	if (!init_bitstreams(enc)) {
		goto fail;
	}
	if (!init_textures(enc)) {
		goto fail;
	}

	return enc;

fail:
	nvenc_destroy(enc);
	return NULL;
}
