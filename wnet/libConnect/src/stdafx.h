#ifndef stdafx_2016_3_25_25_H_
#define stdafx_2016_3_25_25_H_

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT _WIN32_WINNT_WINXP
#include <SDKDDKVer.h>
#include <windows.h>
#include <WinSock2.h>
#include <Mswsock.h>
#include <process.h>
#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "Mswsock.lib")
#endif

#include <locale>
#include <assert.h>
#include <string>
#include <iostream>
#include <functional>
#include <map>
#include <vector>

#include <vld.h>

#include <helpers\StringCovert.h>
#include <helpers\LoopBuffer.h>
#include <helpers\MacroMember.h>
#include <helpers\FastMutex.h>
#include <helpers\Singleton.h>
#include <helpers\Crc.h>

#include "Common.h"

#endif //stdafx_2016_3_25_25_H_