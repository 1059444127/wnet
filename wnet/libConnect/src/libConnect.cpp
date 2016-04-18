#include "stdafx.h"

#include "libConnect.h"

#include "TCPServer.h"
#include "TCPClient.h"

namespace WStone {

void _internal_logCallback(
	const char* file, 
	unsigned line, 
	const wchar_t* msg, ...)
{
	static FastMutex s_mutexLog;
	FastMutex::ScopedLock lock(s_mutexLog);

	static const int s_maxLeng = 8192;
	static wchar_t buf[s_maxLeng] = {0};

	memset(buf, 0, s_maxLeng);

	SYSTEMTIME st;
	GetLocalTime(&st);

	_snwprintf_s(buf, s_maxLeng -1, 
		L"%u-%02d-%02d %02d:%02d:%02d.%03d [%s-%u] ", 
		st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute,
		st.wSecond, st.wMilliseconds,
		AnsiToUnicode(file).c_str(), line);
	auto sizes = wcslen(buf);

	va_list pArgs;
	va_start(pArgs, msg);
	_vsnwprintf_s(buf + sizes, s_maxLeng - sizes, 
		s_maxLeng - sizes, msg, pArgs);
	va_end(pArgs);

	if (nullptr != LibConnect::getInstance()->getLogCallback()) {
		LibConnect::getInstance()->getLogCallback()(buf);

	} else {
		std::wcout << buf << L"\n";
		OutputDebugStringW(buf);
		std::wcout << L"\n";
	}
}

void _internal_sysLog(
	const char* file, 
	unsigned line,
	const wchar_t* cErr, 
	unsigned long errID)
{
	LPWSTR errMsg = nullptr;

	FormatMessageW(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, errID, 
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
		(LPWSTR)&errMsg, 0, nullptr);

	std::wstring errInfo = errMsg;
	errInfo = errInfo.substr(0, errInfo.length() - 2);
	_internal_logCallback(file, line, L"%s : [%u - %s]", 
		cErr, errID, errInfo.c_str());
	LocalFree(errMsg);
}

int getCPUNumbers()
{
	SYSTEM_INFO si;
	GetSystemInfo(&si);

	return si.dwNumberOfProcessors;
}

_tagPERIODATA::_tagPERIODATA() : 
opType(OPT_UNKNOWN), 
fd(INVALID_SOCKET),
buffer(nullptr)
{
	buffer = new char8[s_bufferlength];
	memset(buffer, 0, s_bufferlength);

	memset(&overlapped, 0, sizeof(OVERLAPPED));
	overlappedBuffer.buf = buffer;
	overlappedBuffer.len = s_bufferlength;
}

_tagPERIODATA::~_tagPERIODATA()
{
	safeDelete(buffer);

	closesocket(fd);
}

void _tagPERIODATA::resetBuffer()
{
	memset(buffer, 0, s_bufferlength);
}

_tagPERSOCKETDATA::_tagPERSOCKETDATA() :
fd(INVALID_SOCKET), 
clientPort(0) 
{

}

_tagPERSOCKETDATA::~_tagPERSOCKETDATA()
{
	closesocket(fd);

	for(auto it : ioContexts) {
		safeDelete(it);
	}
}

PIOContext _tagPERSOCKETDATA::getNewIOContext()
{
	FastMutex::ScopedLock lock(_mtIOContexts);

	PIOContext ret = new IOContext();
	ioContexts.push_back(ret);
	return ret;
}

void _tagPERSOCKETDATA::removeIOContext(PIOContext pIOContext) 
{
	FastMutex::ScopedLock lock(_mtIOContexts);

	auto it = ioContexts.begin();
	for(; it != ioContexts.end(); ++it) {
		if(*it == pIOContext) {
			delete pIOContext;
			ioContexts.erase(it);
			break;
		}
	}
}

char8* _tagPacketHeader::packet(
	unsigned msgid, 
	const char8* data, 
	unsigned& length)
{
	if(nullptr == data || 0 == length) {
		return nullptr;
	}

	auto headerLength = sizeof(PacketHeader);

	length = headerLength + sizeof(char8) * length;
	auto pHeader = (_tagPacketHeader*)new char8[length];

	pHeader->signature = s_signature;
	pHeader->msgid = msgid;
	pHeader->packetLength = length;
	pHeader->reserved = 0;
	pHeader->crc = 0;

	memcpy(pHeader->body, data, length - headerLength);
	pHeader->crc = Crc::crc32((char8*)pHeader, length);

	pHeader->signature = htonl(pHeader->signature);
	pHeader->msgid = htonl(pHeader->msgid);
	pHeader->packetLength = htonl(pHeader->packetLength);
	pHeader->reserved = htonl(pHeader->reserved);
	pHeader->crc = htonl(pHeader->crc);

	return (char8*)pHeader;
}

extern "C" {

_LIBCONNECT_EXPORTS ILibConnect* _LIBCONNECT_CALL openNetManager()
{
	std::wcout.imbue(std::locale("chs"));
	return LibConnect::getInstance();
}

_LIBCONNECT_EXPORTS void _LIBCONNECT_CALL closeNetManager()
{
	return LibConnect::destroyInstance();
}

}

//////////////////////////////////////////////////////////////////////////
LibConnect::LibConnect() : 
_encryptCB(nullptr),
_logCB(nullptr)
{
	loadSocketLib();
}

LibConnect::~LibConnect()
{
	TCPClient::destroyInstance();
	TCPServer::destroyInstance();

	unLoadSocketLib();
}

void LibConnect::loadSocketLib()
{
	WSADATA wsaData;
	auto wNeeds = MAKEWORD(2, 2);

	auto err = WSAStartup(wNeeds, &wsaData);
	if(NO_ERROR != err) {
		LOG("³õÊ¼»¯ÍøÂç¿â´íÎó[%d]", err);
	}
}

void LibConnect::unLoadSocketLib()
{
	WSACleanup();
}

ITCPServer* LibConnect::getTCPServer()
{
	return TCPServer::getInstance();
}

ITCPClient* LibConnect::getTCPClient()
{
	return TCPClient::getInstance();
}

}