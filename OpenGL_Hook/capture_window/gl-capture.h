#pragma once



#ifdef __cplusplus
extern "C" {
#else
#if defined(_MSC_VER) && !defined(inline)
#define inline __inline
#endif
#endif
	bool hook_gl();
	 void* gl_shared_init_d3d11(void);
	  void* get_d3d11_device_context(void* cur_d3d11);
	  bool hook_captuer_ok(void);
	  HANDLE get_shared();
	  void g_send_video_callback();
	  void send_video_data(ID3D11Device* cur_d3d11, ID3D11Texture2D* cur_d3d11_texture);
#ifdef __cplusplus
}
#endif