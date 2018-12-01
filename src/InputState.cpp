#include "InputState.h"
using namespace InputSpace;

bool InputState::GetVsync() {
	return !(tearingSupported && !vsync);
}

void InputState::SetVsync(bool state) {
	vsync = state;
}

void InputState::SetTearingSupport(bool state) {
	tearingSupported = state;
}

float InputState::GetZoom() {
	return zoomLevel;
}

void InputState::SetZoom(float zoom) {
	zoomLevel = zoom;
}

CameraInfo InputState::GetCamera(double deltaTime) {
	POINT cursorPos;
	GetCursorPos(&cursorPos);
	mouse_X = cursorPos.x;
	mouse_Y = cursorPos.y;
	float mouseSensitivity = 15;
	if (buttons.mouse_L) {
		camera_X += mouseSensitivity * (mouse_X - old_mouse_X) * deltaTime;
		camera_X = fmodf(camera_X, 360);
		if (camera_X < 0)
			camera_X += 360;
		camera_Y += mouseSensitivity * (old_mouse_Y - mouse_Y) * deltaTime;
		if (camera_Y > 89) {
			camera_Y = 89;
		}
		else if (camera_Y < -89) {
			camera_Y = -89;
		}
	}
	old_mouse_X = mouse_X;
	old_mouse_Y = mouse_Y;


	int up_down = InputState::GetKey(TrackedInput::UpDown);
	int left_right = InputState::GetKey(TrackedInput::LeftRight);
	XMVECTOR pos = XMLoadFloat3(&(camera.position));
	XMVECTOR look = XMLoadFloat3(&(camera.lookVector));
	XMVECTOR up = XMVectorSet(0, 1, 0, 1);
	XMVECTOR right = XMVector3Cross(look, up);

	look = look * up_down * 5 * deltaTime;
	right = right * -left_right * 5 * deltaTime;

	
	XMStoreFloat3(&(camera.position), pos + look + right);
	look = XMVectorSet(sinf(XMConvertToRadians(camera_X)) * cosf(XMConvertToRadians(camera_Y)), sinf(XMConvertToRadians(camera_Y)), cosf(XMConvertToRadians(camera_X)) * cosf(XMConvertToRadians(camera_Y)), 1);
	XMStoreFloat3(&(camera.lookVector), look);

	return camera;
}

void InputState::SetCamera(CameraInfo newInfo, float cam_X, float cam_Y) {
	camera = newInfo;
	camera_X = cam_X;
	camera_Y = cam_Y;
}

bool InputState::GetScriptedCam() {
	return scripted_cam;
}

void InputState::SetKey(KeyCode::Key key, bool value) {
	switch (key)
	{
	case KeyCode::W:
		buttons.w = value;
		break;
	case KeyCode::Up:
		buttons.upArrow = value;
		break;
	case KeyCode::S:
		buttons.s = value;
		break;
	case KeyCode::Down:
		buttons.downArrow = value;
		break;
	case KeyCode::A:
		buttons.a = value;
		break;
	case KeyCode::Left:
		buttons.leftArrow = value;
		break;
	case KeyCode::D:
		buttons.d = value;
		break;
	case KeyCode::Right:
		buttons.rightArrow = value;
		break;
	case KeyCode::R:
		if (value)
			Reset();
		break;
	case KeyCode::V:
		if(value)
			vsync = !vsync;
		break;
	case KeyCode::C:
		if (value)
			scripted_cam = !scripted_cam;
		if (!scripted_cam)
			Reset();
	default:
		break;
	}
}

void InputState::SetMouseEvent(WPARAM wParam, LPARAM lParam) {
	buttons.mouse_L = (wParam & MK_LBUTTON) != 0;
}

int InputState::GetKey(TrackedInput option) {
	int returnValue = 0;
	switch (option) {
	case TrackedInput::UpDown:
		if (buttons.upArrow || buttons.w) {
			returnValue += 1;
		}
		if (buttons.downArrow || buttons.s) {
			returnValue -= 1;
		}
		break;
	case TrackedInput::LeftRight:
		if (buttons.rightArrow || buttons.d) {
			returnValue += 1;
		}
		if (buttons.leftArrow || buttons.a) {
			returnValue -= 1;
		}
		break;
	}
	return returnValue;
}

void InputState::Reset() {
	SetCamera({ XMFLOAT3(0,0,0),XMFLOAT3(0,0,-1) }, 180, 0);
	vsync = false;
	scripted_cam = false;
}