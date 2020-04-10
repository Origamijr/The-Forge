/*
 * Copyright (c) 2018-2020 The Forge Interactive Inc.
 *
 * This file is part of The-Forge
 * (see https://github.com/ConfettiFX/The-Forge).
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
*/

#pragma once

#if defined(_WIN32)
#include <sys/stat.h>
#include <stdlib.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#include <windows.h>
#endif

#if defined(__APPLE__)
#if !defined(TARGET_IOS)
#import <Carbon/Carbon.h>
#else
#include <stdint.h>
typedef uint64_t uint64;
#endif
#endif
#if defined(__ANDROID__)
#include <android_native_app_glue.h>
#include <android/log.h>
#elif defined(__linux__) && !defined(VK_USE_PLATFORM_GGP)
#define VK_USE_PLATFORM_XLIB_KHR
#if defined(VK_USE_PLATFORM_XLIB_KHR) || defined(VK_USE_PLATFORM_XCB_KHR)
#include <X11/Xutil.h>
// X11 defines primitive types which conflict with Forge libraries
#undef Bool
#endif
#endif

#ifdef _WIN32
#define FORGE_CALLCONV __cdecl
#else
#define FORGE_CALLCONV
#endif

#include <stdio.h>
#include <stdint.h>
#include "../../OS/Math/MathTypes.h"

// For time related functions such as getting localtime
#include <time.h>
#include <ctime>

#include <float.h>
#include <limits.h>
#include <stddef.h>

#ifndef _WIN32
#include <unistd.h>
#endif

#if !defined(_WIN32)
#define stricmp(a, b) strcasecmp(a, b)
#if !defined(ORBIS)
#define vsprintf_s vsnprintf
#define strncpy_s strncpy
#endif
#endif

#if defined(_MSC_VER) && !defined(NX64)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

#if defined(_DURANGO)
#define stricmp(a, b) _stricmp(a, b)
#endif

typedef void* IconHandle;

typedef struct WindowHandle
{
#if defined(VK_USE_PLATFORM_XLIB_KHR)
	Display*                 display;
	Window                   window;
	Atom                     xlib_wm_delete_window;
#elif defined(VK_USE_PLATFORM_XCB_KHR)
	xcb_connection_t*        connection;
	xcb_window_t             window;
	xcb_screen_t*            screen;
	xcb_intern_atom_reply_t* atom_wm_delete_window;
#else
	void*                    window;    //hWnd
#endif
} WindowHandle;

typedef struct RectDesc
{
	int32_t left;
	int32_t top;
	int32_t right;
	int32_t bottom;
} RectDesc;

inline int getRectWidth(const RectDesc& rect) { return rect.right - rect.left; }

inline int getRectHeight(const RectDesc& rect) { return rect.bottom - rect.top; }

struct WindowsDesc;

struct WindowCallbacks
{
	void    (*onResize)(WindowsDesc* window, int32_t newSizeX, int32_t newSizeY);
	int32_t (*onHandleMessage)(WindowsDesc* window, void* msg);
};

typedef struct WindowsDesc
{
	WindowHandle              handle;    //hWnd

	WindowCallbacks           callbacks;

	RectDesc                  windowedRect;
	RectDesc                  fullscreenRect;
	RectDesc                  clientRect;
	IconHandle                bigIcon;
	IconHandle                smallIcon;

	uint32_t                  windowsFlags;
	bool                      fullScreen;
	bool                      cursorTracked;
	bool                      iconified;
	bool                      maximized;
	bool                      minimized;
	bool                      hide;
} WindowsDesc;

typedef struct Resolution
{
	uint32_t mWidth;
	uint32_t mHeight;
} Resolution;

// Monitor data
//
typedef struct MonitorDesc
{
	RectDesc monitorRect;
	RectDesc workRect;
	// This size matches the static size of DISPLAY_DEVICE.DeviceName
#ifdef _WIN32
	WCHAR adapterName[32];
	WCHAR displayName[32];
	WCHAR publicAdapterName[64];
	WCHAR publicDisplayName[64];
#else
	char  adapterName[32];
	char  displayName[32];
	char  publicAdapterName[64];
	char  publicDisplayName[64];
#endif
	bool modesPruned;
	bool modeChanged;

	Resolution  defaultResolution;
	Resolution* resolutions;
	uint32_t    resolutionCount;
} MonitorDesc;

// Define some sized types
typedef uint8_t uint8;
typedef int8_t  int8;

typedef uint16_t uint16;
typedef int16_t  int16;

typedef uint32_t uint32;
typedef int32_t  int32;

typedef ptrdiff_t intptr;

#ifdef _WIN32
typedef signed __int64   int64;
typedef unsigned __int64 uint64;
#elif defined(__APPLE__)
typedef unsigned long DWORD;
typedef unsigned int UINT;
typedef long long int int64;
//typedef bool BOOL;
#elif defined(__linux__)
typedef unsigned long DWORD;
typedef unsigned int UINT;
typedef int64_t int64;
typedef uint64_t uint64;
#elif defined(NX64)
typedef unsigned long DWORD;
typedef unsigned int UINT;
typedef long long int int64;
typedef uint64_t uint64;
#else
typedef signed long long   int64;
typedef unsigned long long uint64;
#endif

typedef uint8        ubyte;
typedef uint16       ushort;
typedef unsigned int uint;
typedef const char * LPCSTR, *PCSTR;

// API functions
void requestShutdown();

// Window handling
void openWindow(const char* app_name, WindowsDesc* winDesc);
void closeWindow(const WindowsDesc* winDesc);
void setWindowRect(WindowsDesc* winDesc, const RectDesc& rect);
void setWindowSize(WindowsDesc* winDesc, unsigned width, unsigned height);
void toggleFullscreen(WindowsDesc* winDesc);
void showWindow(WindowsDesc* winDesc);
void hideWindow(WindowsDesc* winDesc);
void maximizeWindow(WindowsDesc* winDesc);
void minimizeWindow(WindowsDesc* winDesc);

void setMousePositionRelative(const WindowsDesc* winDesc, int32_t x, int32_t y);

void getRecommendedResolution(RectDesc* rect);
// Sets video mode for specified display
void setResolution(const MonitorDesc* pMonitor, const Resolution* pRes);

MonitorDesc* getMonitor(uint32_t index);
float2       getDpiScale();

bool getResolutionSupport(const MonitorDesc* pMonitor, const Resolution* pRes);

// Shell commands

typedef struct Path Path;

/// @param stdOutFile The file to which the output of the command should be written. May be NULL.
int systemRun(const char *command, const char **arguments, size_t argumentCount, const Path* stdOutFile);


//
// failure research ...
//