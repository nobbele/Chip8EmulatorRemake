#include "filedialog.h"

// TODO cross-platform

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>      // For common windows data types and function headers
#include <wchar.h>

void get_working_directory(wchar_t *const buffer)
{
	GetModuleFileNameW(NULL, buffer, FILEDIALOGBUFFERSIZE);
}

void open_file_dialog(wchar_t *const buffer)
{
	OPENFILENAMEW ofn;
	ZeroMemory(&ofn, sizeof(ofn));

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL;
	ofn.lpstrFile = buffer;
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = FILEDIALOGBUFFERSIZE;
	ofn.lpstrFilter = L"Chip8 Rom File\0*.*\0\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_READONLY;

	wchar_t working_dir[FILEDIALOGBUFFERSIZE];
	get_working_directory(working_dir);
	ofn.lpstrInitialDir = working_dir;

	GetOpenFileNameW(&ofn);
}