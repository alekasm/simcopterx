#include "FileVersion.h"

BOOL FileVersion::Initialize()
{
	if (hInst)
		return TRUE;

	//Version.lib does not contain GetFileVersionInfoSizeExA or GetFileVersionInfoExA, must be 
	//retrieved from Version.dll. All functions are now instead loaded from dll, including
	//VerQueryValueA
	if ((hInst = LoadLibrary("Version.dll")) == NULL)
	{
		printf("Could not load version.dll! \n");
		return FALSE;
	}

	_GetFileVersionInfoSizeExA2 = (GetFileVersionInfoSizeExA2)GetProcAddress(hInst, "GetFileVersionInfoSizeExA");
	_GetFileVersionInfoExA2 = (GetFileVersionInfoExA2)GetProcAddress(hInst, "GetFileVersionInfoExA");
	_VerQueryValueA2 = (VerQueryValueA2)GetProcAddress(hInst, "VerQueryValueA");

	return TRUE;
}

BOOL FileVersion::GetSCFileVersionInfo(LPCSTR filename, StringFileInfoMap& out)
{
	if (!Initialize())
		return FALSE;

	DWORD flags = FILE_VER_GET_NEUTRAL | FILE_VER_GET_LOCALISED;
	DWORD size = _GetFileVersionInfoSizeExA2(flags, filename, NULL);
	if (size == 0)
	{
		printf("Failed to get the file version info size! Error Code: %d\n", GetLastError());
		return FALSE;
	}

	LPVOID pVersionInfo = new BYTE[size];
	BOOL result = _GetFileVersionInfoExA2(flags, filename, NULL, size, pVersionInfo);
	if (!result)
	{
		printf("Failed to get the file version info! Error Code: %d\n", GetLastError());
		return FALSE;
	}

	UINT puLen2;
	struct LANGANDCODEPAGE {
		WORD wLanguage;
		WORD wCodePage;
	} *lpTranslate;

	BOOL t_result = _VerQueryValueA2(pVersionInfo, "\\VarFileInfo\\Translation", (LPVOID*)&lpTranslate, &puLen2);
	if (!t_result)
	{
		printf("Failed to get the file translation! Error Code: %d\n", GetLastError());
		return FALSE;
	}
	printf("0x%x, 0x%x\n", lpTranslate->wLanguage, lpTranslate->wCodePage);

	std::map<std::string, std::string> stringFileInfo =
	{
		{"FileVersion", ""},
		{"CompanyName", ""},
		{"FileDescription", ""},
		{"OriginalFilename", ""}
	};

	for (auto it = stringFileInfo.begin(); it != stringFileInfo.end(); it++)
	{
		UINT puLen;
		char buffer[256];
		sprintf_s(buffer, sizeof(buffer), "\\StringFileInfo\\%04lx%04lx\\%s", lpTranslate->wLanguage, lpTranslate->wCodePage, it->first.c_str());
		printf("Quering Value: %s\n", buffer);

		LPCSTR lpBuffer;
		BOOL result3 = _VerQueryValueA2(pVersionInfo, buffer, (LPVOID*)&lpBuffer, &puLen);
		if (!result3)
		{
			printf("Failed to get the file file info! Error Code: %d\n", GetLastError());
			return FALSE;
		}
		stringFileInfo[it->first] = lpBuffer;
		printf("%s: %s\n", it->first.c_str(), it->second.c_str());
	}

	out = stringFileInfo;
	return TRUE;
}