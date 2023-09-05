
#ifndef _C_STREAM_RTC_C_H_
#define _C_STREAM_RTC_C_H_
 
//#include <string>



#include <stdint.h>
#include <stdbool.h>
#include <Windows.h>

#define DLLIMPORT   __declspec(dllimport)
 
#ifdef __cplusplus



/// ////////////////////////////////////////////////////////////////////////////
 
//bool DLLIMPORT  cpp_init();
  
extern "C" {

#endif

	/////////////////////////input device/////////////////////////////////////

	enum EMouseButtonsType
	{
		EMouseLeft = 0,
		EMouseMiddle,
		EMouseRight,
		EMouseThumb01,
		EMouseThumb02,

		EMouseInvalid,
	};






	/** The types of event which can be processed by the device. */
	enum   CEEventType
	{
		EEventUNDEFINED = 0,		 /** No value. */
		EEventKEY_DOWN,		 /** A key has been pushed down. */
		EEventKEY_UP,			 /** A key has been released. */
		EEventKEY_PRESS,		 /** A key has been pressed and a character has been input. */
		EEventMOUSE_ENTER,	 /** The mouse has entered canvas. */
		EEventMOUSE_LEAVE,	 /** The mouse has left the canvas. */
		EEventMOUSE_MOVE,		 /** The mouse has been moved. */
		EEventMOUSE_DOWN,		 /** A mouse button has been clicked. */
		EEventMOUSE_UP,		 /** A mouse button has been released. */
		EEventMOUSE_WHEEL,	 /** The mouse wheel was scrolled. */
		EEventTOUCH_START,	 /** A finger is put down onto the canvas. */
		EEventTOUCH_END,		 /** A finger is lifted from the canvas. */
		EEventTOUCH_MOVE,		 /** A finger is being dragged along the surface of the canvas. */
		EEventGAMEPAD_PRESS,	 /** A gamepad button has been pressed. */
		EEventGAMEPAD_RELEASE, /** A gamepad button has been released. */
		EEventGAMEPAD_ANALOG,	 /** A gamepad analog stick has been moved. */
	};

	/** A general input event. */
	struct CInputEvent
	{
		/** The type of the general event. */
		uint32_t Event; //CEEventType

		/** A generic piece of data which is used to hold information about the
		* event, specialized by making a union with an appropriate struct. */
		union
		{
			uint64_t Word;

			struct   /** KEY_DOWN */
			{
				uint8_t KeyCode;
				bool bIsRepeat;
			} KeyDown;

			struct   /* KEY_UP */
			{
				uint8_t KeyCode;
			} KeyUp;

			struct   /** KEY_PRESSED */
			{
				TCHAR Character;
			} Character;

			struct   /** MOUSE_MOVE */
			{
				int16_t DeltaX;
				int16_t DeltaY;
				uint16_t PosX;
				uint16_t PosY;
			} MouseMove;

			struct   /** MOUSE_DOWN, MOUSE_UP */
			{
				uint8_t Button;
				uint16_t PosX;
				uint16_t PosY;
			} MouseButton;

			struct   /** MOUSE_WHEEL */
			{
				int16_t Delta;
				uint16_t PosX;
				uint16_t PosY;
			} MouseWheel;

			struct   /** TOUCH_START, TOUCH_END, TOUCH_MOVE */
			{
				uint8_t TouchIndex;
				uint16_t PosX;
				uint16_t PosY;
				uint8_t Force;
			} Touch;

			struct   /** GAMEPAD_PRESSED, GAMEPAD_RELEASED */
			{
				uint8_t ControllerIndex;
				uint8_t ButtonIndex;
				bool bIsRepeat;

			} GamepadButton;

			struct   /** GAMEPAD_PRESSED, GAMEPAD_RELEASED */
			{
				uint8_t ControllerIndex;
				float AnalogValue;
				uint8_t AxisIndex;
			} GamepadAnalog;
		} Data;

		////////////////////////////////////////////////


	} ;

	typedef void(*set_inputdevice_callback)(struct CInputEvent* e);


	
 

	extern void* g_shared_handler;
	extern void* g_shared_width;
	extern void* g_shared_height;

	bool DLLIMPORT gl_shared_init(uint32_t width, uint32_t height);
	void DLLIMPORT gl_shared_capture(uint32_t width, uint32_t height);
	void DLLIMPORT gl_shared_destroy();

	bool DLLIMPORT  c_init();
	void DLLIMPORT  c_set_input_device_callback(set_inputdevice_callback  callback);
	bool DLLIMPORT  c_startup(const char * ip, uint32_t port, const char * roomname, const char * username);
	void DLLIMPORT  c_destroy();

#ifdef __cplusplus

}

#endif

#endif

