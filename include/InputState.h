#pragma once


#include "Common.h"
#include "KeyCodes.h"

namespace InputSpace
{
	struct TrackedButtons {
		bool upArrow;
		bool downArrow;
		bool leftArrow;
		bool rightArrow;
		bool w;
		bool s;
		bool a;
		bool d;
		bool mouse_L;
	};

	struct CameraInfo {
		XMFLOAT3 position;
		XMFLOAT3 lookVector;
	};

	enum TrackedInput { UpDown, LeftRight };

	class InputState
	{
	public:

		static bool GetVsync();
		static void SetVsync(bool state);

		static void SetTearingSupport(bool state);

		static float GetZoom();
		static void SetZoom(float zoom);

		static CameraInfo GetCamera(double deltaTime);
		static void SetCamera(CameraInfo newInfo, float cam_X, float cam_Y);

		static void SetKey(KeyCode::Key key, bool value);

		static void SetMouseEvent(WPARAM wParam, LPARAM lParam);

		static int GetKey(TrackedInput option);

		static bool GetScriptedCam();

		static void Reset();
	};

	static TrackedButtons buttons;
	static bool vsync;
	static bool tearingSupported;
	static float zoomLevel;
	static CameraInfo camera;
	static float mouse_X;
	static float mouse_Y;
	static float old_mouse_X;
	static float old_mouse_Y;
	static float camera_X;
	static float camera_Y;
	static bool scripted_cam;
}
