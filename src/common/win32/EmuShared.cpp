// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
// ******************************************************************
// *
// *  This file is part of the Cxbx project.
// *
// *  Cxbx and Cxbe are free software; you can redistribute them
// *  and/or modify them under the terms of the GNU General Public
// *  License as published by the Free Software Foundation; either
// *  version 2 of the license, or (at your option) any later version.
// *
// *  This program is distributed in the hope that it will be useful,
// *  but WITHOUT ANY WARRANTY; without even the implied warranty of
// *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// *  GNU General Public License for more details.
// *
// *  You should have recieved a copy of the GNU General Public License
// *  along with this program; see the file COPYING.
// *  If not, write to the Free Software Foundation, Inc.,
// *  59 Temple Place - Suite 330, Bostom, MA 02111-1307, USA.
// *
// *  (c) 2002-2003 Aaron Robinson <caustik@caustik.com>
// *
// *  All rights reserved
// *
// ******************************************************************

#include "core\kernel\init\CxbxKrnl.h"
#include "core\kernel\support\Emu.h"
#include "EmuShared.h"

#include <windows.h>
#include <cstdio>

// ******************************************************************
// * exported globals
// ******************************************************************
EmuShared *g_EmuShared = nullptr;

// ******************************************************************
// * static/global
// ******************************************************************
HANDLE hMapObject = NULL;

HMODULE hActiveModule = NULL;

// ******************************************************************
// * func: EmuShared::EmuSharedInit
// ******************************************************************
bool EmuShared::Init(long long sessionID)
{
    // ******************************************************************
    // * Ensure initialization only occurs once
    // ******************************************************************
    bool bRequireConstruction = true;

    // ******************************************************************
    // * Prevent multiple initializations
    // ******************************************************************
    if (hMapObject != NULL) {
        return true;
    }

    // ******************************************************************
    // * Prevent invalid session process id
    // ******************************************************************
    if (sessionID == 0) {
        return false;
    }

    // ******************************************************************
    // * Create the shared memory "file"
    // ******************************************************************
    {
        std::string emuSharedStr = "Local\\EmuShared-s" + std::to_string(sessionID);
        hMapObject = CreateFileMapping
        (
            INVALID_HANDLE_VALUE,   // Paging file
            nullptr,                // default security attributes
            PAGE_READWRITE,         // read/write access
            0,                      // size: high 32 bits
            sizeof(EmuShared),      // size: low 32 bits
            emuSharedStr.c_str()    // name of map object
        );

        if(hMapObject == NULL)
            return false; // CxbxrKrnlAbortEx(CXBXR_MODULE::INIT, "Could not map shared memory!");

        if(GetLastError() == ERROR_ALREADY_EXISTS)
            bRequireConstruction = false;
    }

    // ******************************************************************
    // * Memory map this file
    // ******************************************************************
    {
        g_EmuShared = (EmuShared*)MapViewOfFile
        (
            hMapObject,     // object to map view of
            FILE_MAP_WRITE, // read/write access
            0,              // high offset:  map from
            0,              // low offset:   beginning
            0               // default: map entire file
        );

        if (g_EmuShared == nullptr) {
            CloseHandle(hMapObject);
            return false; // CxbxrKrnlAbortEx(CXBXR_MODULE::INIT, "Could not map view of shared memory!");
        }
    }

    // ******************************************************************
    // * Executed only on first initialization of shared memory
    // ******************************************************************
    if (bRequireConstruction) {
        g_EmuShared->EmuShared::EmuShared();
    }

    g_EmuShared->m_RefCount++;

    if (g_EmuShared->m_size != sizeof(EmuShared)) {
        EmuShared::Cleanup();
        return false;
    }

	return true;
}

// ******************************************************************
// * func: EmuSharedCleanup
// ******************************************************************
void EmuShared::Cleanup()
{
	if (g_EmuShared != nullptr) {
		if (--(g_EmuShared->m_RefCount) <= 0)
			g_EmuShared->EmuShared::~EmuShared();

		UnmapViewOfFile(g_EmuShared);
		g_EmuShared = nullptr;
	}
}

// ******************************************************************
// * Constructor
// ******************************************************************
EmuShared::EmuShared()
{
	m_size = sizeof(EmuShared);
//	m_bMultiXbe = false;
//	m_LaunchDataPAddress = NULL;
	m_bDebugging = false;
	m_bEmulating_status = false;
	m_bFirstLaunch = false;
	m_bClipCursor = false;

	std::memset(m_DeviceControlNames, '\0', sizeof(m_DeviceControlNames));
	std::memset(m_DeviceName, '\0', sizeof(m_DeviceName));
	m_imgui_general.ini_size = IMGUI_INI_SIZE_MAX;
	std::memset(&m_imgui_general.ini_settings, 0, sizeof(m_imgui_general.ini_settings));
	std::memset(&m_imgui_audio_windows, 0, sizeof(m_imgui_audio_windows));
	std::memset(&m_imgui_video_windows, 0, sizeof(m_imgui_video_windows));
	for (auto& i : m_DeviceType) {
		i = to_underlying(XBOX_INPUT_DEVICE::DEVICE_INVALID);
	}
    std::strncpy(m_git_version, GetGitVersionStr(), GetGitVersionLength());
}

// ******************************************************************
// * Deconstructor
// ******************************************************************
EmuShared::~EmuShared()
{
}
