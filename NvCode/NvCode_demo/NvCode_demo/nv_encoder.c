 

 
#include <string.h>
#include <stdarg.h>
#define INITGUID
#include <dxgi.h>
#include <d3d11.h>
#include <d3d11_1.h>
#include "clog.h"
#include "nv_encoder_helper.h"

//////////////////////////////////
/* ------------------------------------------------------------------------- */
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
	struct circlebuf dts_list;

	DARRAY(uint8_t) packet_data;*/
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


//////////////////

static HANDLE get_lib(struct nvenc_data *enc, const char *lib)
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

	if (!dxgi || !d3d11) 
	{
		ERROR_EX_LOG("load dxgi = %p, load d3d11 = %p", dxgi, d3d11);
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

	hr = create_dxgi(&IID_IDXGIFactory1, &factory);
	if (FAILED(hr)) 
	{
		ERROR_EX_LOG("CreateDXGIFactory1 failed");
		return false;
	}

	hr = factory->lpVtbl->EnumAdapters(factory, 0, &adapter);
	factory->lpVtbl->Release(factory);
	if (FAILED(hr))
	{
		ERROR_EX_LOG("EnumAdapters failed");
		return false;
	}

	hr = create_device(adapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, 0, NULL, 0,
		D3D11_SDK_VERSION, &device, NULL, &context);
	adapter->lpVtbl->Release(adapter);
	if (FAILED(hr)) 
	{
		ERROR_EX_LOG("D3D11CreateDevice failed");
		return false;
	}

	enc->device = device;
	enc->context = context;
	return true;
}


static bool init_session(struct nvenc_data *enc)
{
	NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS params = {NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS_VER };
	params.device = enc->device;
	params.deviceType = NV_ENC_DEVICE_TYPE_DIRECTX;
	params.apiVersion = NVENCAPI_VERSION;
	void * session = NULL;
	if (NV_FAILED(nv.nvEncOpenEncodeSessionEx(&params, &session)))
	{
		ERROR_EX_LOG("nvEncOpenEncodeSessionEx failed !!!");
		return false;
	}
	enc->session = session;
	return true;
}



static bool init_encoder(struct nvenc_data *enc)
{
	enc->cx = 1920;
	enc->cy = 1080;

	GUID nv_preset = NV_ENC_PRESET_LOW_LATENCY_DEFAULT_GUID;


	/* -------------------------- */
	/* get preset default config  */

	NV_ENC_PRESET_CONFIG preset_config = { NV_ENC_PRESET_CONFIG_VER,
						  {NV_ENC_CONFIG_VER} };

	NVENCSTATUS err = nv.nvEncGetEncodePresetConfig(enc->session,
		NV_ENC_CODEC_H264_GUID, nv_preset,
		&preset_config);
	if (nv_failed(err, __FUNCTION__,
		"nvEncGetEncodePresetConfig"))
	{
		ERROR_EX_LOG("nvEncGetEncodePresetConfig failed !!!");
		return false;
	}



	/* -------------------------- */
	/* main configuration         */

	enc->config = preset_config.presetCfg;

	uint32_t gop_size = 60;
		/*(keyint_sec) ? keyint_sec * voi->fps_num / voi->fps_den : 250;*/

	NV_ENC_INITIALIZE_PARAMS *params = &enc->params;
	NV_ENC_CONFIG *config = &enc->config;
	NV_ENC_CONFIG_H264 *h264_config = &config->encodeCodecConfig.h264Config;
	NV_ENC_CONFIG_H264_VUI_PARAMETERS *vui_params =
		&h264_config->h264VUIParameters;

	int darWidth, darHeight;
//	av_reduce(&darWidth, &darHeight, voi->width, voi->height, 1024 * 1024);

	memset(params, 0, sizeof(*params));
	params->version = NV_ENC_INITIALIZE_PARAMS_VER;
	params->encodeGUID = NV_ENC_CODEC_H264_GUID;
	params->presetGUID = nv_preset;
	params->encodeWidth = enc->cx;
	params->encodeHeight = enc->cy;
	params->darWidth = enc->cx;
	params->darHeight = enc->cy;
	params->frameRateNum = 60;
	params->frameRateDen = 1;
	params->enableEncodeAsync = 1;
	params->enablePTD = 1;
	params->encodeConfig = &enc->config;
	config->gopLength = gop_size;
	config->frameIntervalP = 1 + 2/*bf*/;
	h264_config->idrPeriod = gop_size;

	/*bool repeat_headers = obs_data_get_bool(settings, "repeat_headers");
	if (repeat_headers) */
	{
		h264_config->repeatSPSPPS = 1;
		h264_config->disableSPSPPS = 0;
		h264_config->outputAUD = 1;
	}

	h264_config->sliceMode = 3;
	h264_config->sliceModeData = 1;

	h264_config->useBFramesAsRef = NV_ENC_BFRAME_REF_MODE_DISABLED;

	vui_params->videoSignalTypePresentFlag = 1;
	//vui_params->videoFullRangeFlag = (voi->range == VIDEO_RANGE_FULL);
	vui_params->colourDescriptionPresentFlag = 1;

	/*switch (voi->colorspace) {
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
	}*/

	enc->bframes = 2;

	/* lookahead */
	/*const bool use_profile_lookahead = config->rcParams.enableLookahead;
	lookahead = nv_get_cap(enc, NV_ENC_CAPS_SUPPORT_LOOKAHEAD) &&
		(lookahead || use_profile_lookahead);
	if (lookahead) {
		enc->rc_lookahead = use_profile_lookahead
			? config->rcParams.lookaheadDepth
			: 8;
	}*/

	/*int buf_count = max(4, config->frameIntervalP * 2 * 2);
	if (lookahead) {
		buf_count = max(buf_count, config->frameIntervalP +
			enc->rc_lookahead +
			EXTRA_BUFFERS);
	}*/

	//buf_count = 4; // min(64, buf_count);
	enc->buf_count = 4; // buf_count; */

	/*const int output_delay = buf_count - 1;
	enc->output_delay = output_delay;*/

	//if (lookahead) {
	//	const int lkd_bound = output_delay - config->frameIntervalP - 4;
	//	if (lkd_bound >= 0) {
	//		config->rcParams.enableLookahead = 1;
	//		config->rcParams.lookaheadDepth =
	//			max(enc->rc_lookahead, lkd_bound);
	//		config->rcParams.disableIadapt = 0;
	//		config->rcParams.disableBadapt = 0;
	//	}
	//	else {
	//		lookahead = false;
	//	}
	//}

	/* psycho aq */
	//if (nv_get_cap(enc, NV_ENC_CAPS_SUPPORT_TEMPORAL_AQ)) {
	//	config->rcParams.enableAQ = psycho_aq;
	//	config->rcParams.aqStrength = 8;
	//	config->rcParams.enableTemporalAQ = psycho_aq;
	//}
	//else if (psycho_aq) {
	//	warn("Ignoring Psycho Visual Tuning request since GPU is not capable");
	//}


	/* -------------------------- */
	/* rate control               */

	/*enc->can_change_bitrate =
		nv_get_cap(enc, NV_ENC_CAPS_SUPPORT_DYN_BITRATE_CHANGE) &&
		!lookahead;*/

	config->rcParams.rateControlMode = NV_ENC_PARAMS_RC_CBR_LOWDELAY_HQ;

	/*if (astrcmpi(rc, "cqp") == 0 || rc_lossless) {
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

	}*/
	//else if (astrcmpi(rc, "vbr") != 0) { /* CBR by default */
	//	h264_config->outputBufferingPeriodSEI = 1;
	//	config->rcParams.rateControlMode =
	//		twopass ? NV_ENC_PARAMS_RC_2_PASS_QUALITY
	//		: NV_ENC_PARAMS_RC_CBR;
	//}

	h264_config->outputPictureTimingSEI = 1;
	config->rcParams.averageBitRate = 8000 * 1000;
	config->rcParams.maxBitRate = 10000 * 1000;
	//config->rcParams.vbvBufferSize = bitrate * 1000;



	/* -------------------------- */
	/* profile                    */
	config->profileGUID = NV_ENC_H264_PROFILE_BASELINE_GUID;
	/*if (astrcmpi(profile, "main") == 0) {
		config->profileGUID = NV_ENC_H264_PROFILE_MAIN_GUID;
	}
	else if (astrcmpi(profile, "baseline") == 0) {
		config->profileGUID = NV_ENC_H264_PROFILE_BASELINE_GUID;
	}
	else if (!lossless) {
		config->profileGUID = NV_ENC_H264_PROFILE_HIGH_GUID;
	}*/

	/* -------------------------- */
	/* initialize                 */

	if (NV_FAILED(nv.nvEncInitializeEncoder(enc->session, params))) {
		return false;
	}

	/*info("settings:\n"
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
		psycho_aq ? "true" : "false");*/

	return true;
		return true;
}

/* ------------------------------------------------------------------------- */
/* Texture Resource                                                          */

struct nv_texture {
	void *res;
	ID3D11Texture2D *tex;
	void *mapped_res;
};

static bool nv_texture_init(struct nvenc_data *enc, struct nv_texture *nvtex)
{
	ID3D11Device *device = enc->device;
	ID3D11Texture2D *tex;
	HRESULT hr;

	D3D11_TEXTURE2D_DESC desc = { 0 };
	desc.Width = enc->cx;
	desc.Height = enc->cy;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_NV12;
	desc.SampleDesc.Count = 1;
	desc.BindFlags = D3D11_BIND_RENDER_TARGET;

	hr = device->lpVtbl->CreateTexture2D(device, &desc, NULL, &tex);
	if (FAILED(hr)) 
	{
		ERROR_EX_LOG("Failed to create texture");
		return false;
	}

	tex->lpVtbl->SetEvictionPriority(tex, DXGI_RESOURCE_PRIORITY_MAXIMUM);

	NV_ENC_REGISTER_RESOURCE res = { NV_ENC_REGISTER_RESOURCE_VER };
	res.resourceType = NV_ENC_INPUT_RESOURCE_TYPE_DIRECTX;
	res.resourceToRegister = tex;
	res.width = enc->cx;
	res.height = enc->cy;
	res.bufferFormat = NV_ENC_BUFFER_FORMAT_NV12;

	if (NV_FAILED(nv.nvEncRegisterResource(enc->session, &res)))
	{
		ERROR_EX_LOG("nvEncRegisterResource failed !!!");
		tex->lpVtbl->Release(tex);
		return false;
	}
	DEBUG_EX_LOG("nvEncRegisterResource ok !!!");
	nvtex->res = res.registeredResource;
	nvtex->tex = tex;
	return true;
}

static bool init_textures(struct nvenc_data *enc)
{
	//da_reserve(enc->bitstreams, enc->buf_count);
	for (int i = 0; i < enc->buf_count; i++) 
	{
		struct nv_texture texture;
		if (!nv_texture_init(enc, &texture)) 
		{
			ERROR_EX_LOG("nv_texture_init filaed i = %u", i);
			return false;
		}
		DEBUG_EX_LOG("nv_texture_init (i = %u) ok !!!", i);
		//da_push_back(enc->textures, &texture);
	}

	return true;
}

int main(int argc, char *argv[])
{
	DEBUG_EX_LOG("");
	NV_ENCODE_API_FUNCTION_LIST init = { NV_ENCODE_API_FUNCTION_LIST_VER };
	struct nvenc_data * enc = malloc(sizeof(struct nvenc_data));
	if (load_nvenc_lib())
	{
		DEBUG_EX_LOG("load nvencodeAPI ok !!!");

		// 1. init nvenc --> 
		if (!init_nvenc()) 
		{
			ERROR_EX_LOG("init evenc failed !!!");
			goto fail;
		}
		if (NV_FAILED(nv_create_instance(&init)))
		{
			ERROR_EX_LOG("nv_create_instance failed !!!");
			goto fail;
		}
		DEBUG_EX_LOG("nv_create_instance ok @!!!");
		if (!init_d3d11(enc))
		{
			ERROR_EX_LOG("init_d3d11 failed !!!");
			goto fail;
		}
		DEBUG_EX_LOG("init d3d11 ok !!!");

		if (!init_session(enc)) 
		{
			ERROR_EX_LOG("init_session failed !!!");
			goto fail;
		}
		DEBUG_EX_LOG("init session ok !!!");

		if (!init_encoder(enc)) 
		{
			ERROR_EX_LOG("init_encoder failed !!!");
			goto fail;
		}
		
		DEBUG_EX_LOG("init_encoder ok !!!");

		if (!init_textures(enc)) 
		{
			ERROR_EX_LOG("init_textures failed !!!");
			goto fail;
		}
		DEBUG_EX_LOG("init_textures ok !!!");
	}
	else
	{
		ERROR_EX_LOG("load nvencodeAPI failed !!!");
	}
fail:

	return 0;
}