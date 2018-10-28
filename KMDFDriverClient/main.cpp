#include "stdafx.h"

#include <cfgmgr32.h>
#include <hidsdi.h>
#include <cmath>

#pragma comment(lib, "hid.lib")

#define REPORT_ID_MOUSE 0x02

#pragma pack(1)
typedef struct _HID_MOUSE_INPUT_REPORT {
	BYTE				reportId;
	BYTE				buttons;
	CHAR				x;
	CHAR				y;
	BYTE				code;
	unsigned __int64	data;
} HID_MOUSE_INPUT_REPORT, *PHID_MOUSE_INPUT_REPORT;
#pragma pack()

HANDLE openDevice() {
	GUID guid;
	HidD_GetHidGuid(&guid);

	ULONG deviceInterfaceListLength = 0;
	CONFIGRET configret = CM_Get_Device_Interface_List_Size(
		&deviceInterfaceListLength,
		&guid,
		NULL,
		CM_GET_DEVICE_INTERFACE_LIST_PRESENT
	);
	if (CR_SUCCESS != configret) {
		return NULL;
	}

	if (deviceInterfaceListLength <= 1) {
		return NULL;
	}

	PWSTR deviceInterfaceList = (PWSTR)malloc(deviceInterfaceListLength * sizeof(WCHAR));
	if (NULL == deviceInterfaceList) {
		return NULL;
	}
	ZeroMemory(deviceInterfaceList, deviceInterfaceListLength * sizeof(WCHAR));

	configret = CM_Get_Device_Interface_List(
		&guid,
		NULL,
		deviceInterfaceList,
		deviceInterfaceListLength,
		CM_GET_DEVICE_INTERFACE_LIST_PRESENT
	);
	if (CR_SUCCESS != configret) {
		free(deviceInterfaceList);
		return NULL;
	}

	HANDLE file = INVALID_HANDLE_VALUE;
	for (PWSTR currentInterface = deviceInterfaceList; *currentInterface; currentInterface += wcslen(currentInterface) + 1) {
		HIDD_ATTRIBUTES hidAttributes;

		file = CreateFile(currentInterface, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
		if (INVALID_HANDLE_VALUE == file) {
			continue;
		}

		if (!HidD_GetAttributes(file, &hidAttributes)) {
			CloseHandle(file);
			continue;
		}

		if (0x00 != hidAttributes.VendorID || 0x00 != hidAttributes.ProductID) {
			CloseHandle(file);
			continue;
		}
		break;
	}
	free(deviceInterfaceList);
	if (INVALID_HANDLE_VALUE == file) {
		return NULL;
	}

	return file;
}

BOOL
sendMouseReport(
	HANDLE handle,
	CHAR xSpeed,
	CHAR ySpeed)
{
	HID_MOUSE_INPUT_REPORT report;
	report.reportId = REPORT_ID_MOUSE;
	report.buttons = 0x00;
	report.x = xSpeed;
	report.y = ySpeed;
	report.code = 0x01;
	report.data = 0x0011223344556677;

	DWORD bytesWritten = 0;
	BOOL isWritten = WriteFile(handle, &report, sizeof(HID_MOUSE_INPUT_REPORT), &bytesWritten, NULL);
	return isWritten && sizeof(HID_MOUSE_INPUT_REPORT) == bytesWritten;
}

BOOL
moveCursor(
	HANDLE file,
	LONG x,
	LONG y)
{
	while (true) {
		POINT currentPoint;
		BOOL getCursorPosResult = GetCursorPos(&currentPoint);
		if (!getCursorPosResult) {
			return FALSE;
		}

		LONG distance = (LONG)sqrt(pow(x - currentPoint.x, 2) + pow(y - currentPoint.y, 2));
		if (distance <= 2) {
			return TRUE;
		}

		LONG xSpeed = (LONG)sqrt(abs(x - currentPoint.x));
		xSpeed = xSpeed > 127 ? 127 : xSpeed;
		xSpeed = (x > currentPoint.x ? xSpeed : -xSpeed);

		LONG ySpeed = (LONG)sqrt(abs(y - currentPoint.y));
		ySpeed = ySpeed > 127 ? 127 : ySpeed;
		ySpeed = (y > currentPoint.y ? ySpeed : -ySpeed);

		if (!sendMouseReport(file, (CHAR) xSpeed, (CHAR) ySpeed)) {
			printf("ERROR!\n");
			return FALSE;
		}
		Sleep(1);
	}
}

POINT getCurrentCursorPosition() {
	POINT currentPoint;
	BOOL getCursorPosResult = GetCursorPos(&currentPoint);
	return currentPoint;
}

enum ExitCodes {
	DEVICE_OPENING_ERROR = 1
};

int main(int argc, char **argv) {
	HANDLE file = openDevice();
	if (NULL == file) {
		return DEVICE_OPENING_ERROR;
	}

	POINT startingPoint = getCurrentCursorPosition();
	for (int V = 0; V <= 127; ++V) {
		sendMouseReport(file, V * (CHAR) pow(-1, V), 0);
		Sleep(100);
		POINT currentPoint = getCurrentCursorPosition();
		int S = abs(currentPoint.x - startingPoint.x);
		printf("%03d => %03d\n", V, S);
		startingPoint = currentPoint;
	}

	system("pause");
	return EXIT_SUCCESS;
}