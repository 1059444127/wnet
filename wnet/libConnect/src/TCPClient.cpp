#include "stdafx.h"

#include "TCPClient.h"

namespace WStone {

unsigned __stdcall workEventLoop(void* arg)
{
	auto pTCPClient = static_cast<WStone::TCPClient*>(arg);

	HANDLE hEvents[] = { pTCPClient->getSocketEvent() };

	static unsigned s_countRead = 0;
	static unsigned s_countUnknown = 0;

 	while(true) {

		auto ret = WSAWaitForMultipleEvents(1, hEvents, false, 
			WSA_INFINITE, false);

		if(WSA_WAIT_FAILED == ret) {
			SYSLOG("网络发生错误", WSAGetLastError());
			break;
		}

		WSANETWORKEVENTS events;
		ret = WSAEnumNetworkEvents(pTCPClient->getHandle(), 
			pTCPClient->getSocketEvent(), &events);
		if(SOCKET_ERROR == ret) {
			SYSLOG("获取网络事件失败", WSAGetLastError());
			break;
		}

		if(events.lNetworkEvents & FD_CONNECT) {
			if(0 == events.iErrorCode[FD_CONNECT_BIT]) {
				LOG("网络服务启动成功");
				pTCPClient->setConneted(true);
			} else {
				SYSLOG("服务连接失败", events.iErrorCode[FD_CONNECT_BIT]);
				break;
			}

		} else if(events.lNetworkEvents & FD_CLOSE) {
			pTCPClient->disconnect();
			pTCPClient->setConneted(false);
			LOG("关闭网络服务");
			break;

		} else if(events.lNetworkEvents & FD_READ) {
			LOG("收到读数据消息 - %u", ++s_countRead);
			if(0 == events.iErrorCode[FD_READ_BIT]) {
				if(!pTCPClient->onRecv()) {
					break;
				}
			} else {
				SYSLOG("收到错误的读数据消息", 
					events.iErrorCode[FD_WRITE_BIT]);
			}

		} else if(events.lNetworkEvents & FD_WRITE) {
			if(0 == events.iErrorCode[FD_CONNECT_BIT]) {
				if(!pTCPClient->onSend()) {
					break;
				}
			} else {
				SYSLOG("收到错误的写数据消息", 
					events.iErrorCode[FD_WRITE_BIT]);
			}

		} else {
			LOG("发现未知事件 - %u", ++s_countUnknown);
			SYSLOG("发现未知事件", WSAGetLastError());
		}
	}

	return 0;
}


TCPClient::TCPClient(void) : 
_session(nullptr),
_fd(INVALID_SOCKET),
_thread(nullptr),
_evtSocket(nullptr),
_isAsync(true),
_sendBuffer(s_bufferlength),
_recvBuffer(s_bufferlength),
_pPacketHeader(nullptr),
_isConnected(false)
{

}


TCPClient::~TCPClient(void)
{
	disconnect();

	safeDeleteArray(_pPacketHeader);
}

void TCPClient::mountMessage(
	unsigned id, 
	messageCallBack cb)
{
	FastMutex::ScopedLock lock(_mtMsgIDCB);

	auto itFound = _mapMsgIDCB.find(id);
	if(itFound == _mapMsgIDCB.end()) {
		_mapMsgIDCB.insert(std::make_pair(id, cb));
	}
}

messageCallBack TCPClient::getMessageCallBack(unsigned id)
{
	FastMutex::ScopedLock lock(_mtMsgIDCB);

	auto itFound = _mapMsgIDCB.find(id);
	if(itFound != _mapMsgIDCB.end()) {
		return itFound->second;
	}

	LOG("发现获取未注册消息回调函数");
	return nullptr;
}

void TCPClient::setTimeOut(
	unsigned recvs, 
	unsigned sends)
{
	if(INVALID_SOCKET != _fd || _isAsync) {
		return;
	}

	setOption(SO_SNDTIMEO, sends, sizeof(unsigned));
	setOption(SO_RCVTIMEO, recvs, sizeof(unsigned));
}

ISession* TCPClient::getSession()
{
	if(!isValidSession()) {
		return nullptr;
	}

	return this;
}

bool TCPClient::connect(
	const wchar_t* ip, 
	unsigned short port, 
	bool async /* = true */)
{
	if(nullptr == ip || 0 == port) {
		return false;
	}

	if(INVALID_SOCKET != _fd) {
		return true;
	}

	auto nIP = inet_addr(UnicodeToAnsi(ip).c_str());
	if(INADDR_NONE == nIP) {
		return false;
	}

	_ip = ip;
	_port = port;

	sockaddr_in netAddr;  
	memset(&netAddr, 0, sizeof(sockaddr_in));
	netAddr.sin_family = AF_INET;  
	netAddr.sin_port = htons(port);
	netAddr.sin_addr.s_addr = nIP;

	_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(INVALID_SOCKET == _fd) {
		LOG("创建client套接字失败[%u]", WSAGetLastError());
		return false;
	}

	setOption(SO_REUSEADDR, true, sizeof(bool));
	setOption(SO_SNDBUF, 1 * 1024, sizeof(int));
	setOption(SO_RCVBUF, 1 * 1024, sizeof(int));

	if(async) {
		_isAsync = true;
		_evtSocket = ::WSACreateEvent();
		::WSAEventSelect(_fd, _evtSocket, 
			FD_CONNECT | FD_READ | FD_WRITE | FD_CLOSE);
		_thread = (HANDLE)_beginthreadex(nullptr, 0, 
			&workEventLoop, this, 0, 0);
		if(INVALID_HANDLE_VALUE == _thread) {
			LOG("创建线程失败[%u]", GetLastError());
			return false;
		}

	} else {
		_isAsync = false;
	}

	if(NO_ERROR != ::connect(_fd, (sockaddr*)&netAddr, sizeof(netAddr))) {
		if(WSAEWOULDBLOCK != WSAGetLastError()) {
			SYSLOG("连接服务失败", WSAGetLastError());
			return false;
		}
	}

	return true;
}

void TCPClient::disconnect()
{
	if(INVALID_SOCKET != _fd) {
		closesocket(_fd);
		_fd = INVALID_SOCKET;
	}
	WaitForSingleObject(_thread, INFINITE);

	if(nullptr != _evtSocket){
		::WSACloseEvent(_evtSocket);
		_evtSocket = nullptr;
	}
}

bool TCPClient::isAlive()
{
	auto ret = ::send(getHandle(), "", 0, 0);
	if(SOCKET_ERROR == ret && 
		WSA_IO_PENDING != WSAGetLastError()) {
			return false;
	}

	return true;
}

bool TCPClient::isValidSession()
{
	if(INVALID_SOCKET != _fd && _isConnected) {
		return true;
	}

	return false;
}

bool TCPClient::setOption(int opname, int value, int sizes)
{
	if(INVALID_SOCKET == _fd) {
		return false;
	}

	if(0 == setsockopt(_fd, SOL_SOCKET, opname, 
		(char8*)&value, sizes)) {
		return true;
	}

	return false;
}

bool TCPClient::send(
	unsigned msgid, 
	const char8* data, 
	unsigned leng)
{
	if(nullptr == data || 0 == leng || !isValidSession()) {
		return false;
	}

	auto sendBuffer = PacketHeader::packet(msgid, data, leng);
	auto sendBytes = 0;

	sendBytes = ::send(_fd, sendBuffer, leng, 0);
	if(SOCKET_ERROR == sendBytes && 
		WSA_IO_PENDING != WSAGetLastError()) {
			SYSLOG("数据发送失败", WSAGetLastError());
	}

	if(_isAsync && sendBytes != leng) {
		sendBytes = _sendBuffer.write(sendBuffer + sendBytes, 
			leng - sendBytes);
	}

	safeDeleteArray(sendBuffer);
	return true;
}

unsigned TCPClient::recv(
	char8* buffer,
	unsigned lens)
{
	if(!isValidSession() || nullptr == buffer || !_isAsync) {
		return 0;
	}

	return ::recv(_fd, buffer, lens, 0);																					
}

bool TCPClient::unPacket()
{
	auto headerSizes = sizeof(PacketHeader);

	if(!setPacketHeader()) {
		return true; // 不完整的包头信息
	}

	if(s_signature != _pPacketHeader->signature) {
		LOG("获取到非法的数据包[%s]，关闭会话", getPeerIP());
		return false;
	}

	auto bodyLengths = _pPacketHeader->packetLength - headerSizes;
	if(_recvBuffer.dataLength() < bodyLengths) {
		return true;// 包不完整，等待完整的包
	}

	char8* packet = nullptr;
	if(!isValidPacket(&packet)) {
		return false;

	} else {
		auto msgCB = getMessageCallBack(_pPacketHeader->msgid);
		if(nullptr != msgCB) {// 调用注册的回调信息
			msgCB(
				dynamic_cast<ISession*>(this), 
				packet + headerSizes,
				bodyLengths);
			delete[] packet;
		}

		resetPacketHeader();
	}

	return true;
}

bool TCPClient::isValidPacket(char8** retPacket)
{
	auto oldCrc = _pPacketHeader->crc;
	_pPacketHeader->crc = 0;

	auto bodyBytes = (_pPacketHeader->packetLength) - sizeof(PacketHeader);
	char8* body = _recvBuffer.read(bodyBytes);

	char8* packet = new char8[_pPacketHeader->packetLength];
	memcpy(packet, _pPacketHeader, sizeof(PacketHeader));
	memcpy(packet + sizeof(PacketHeader), body, bodyBytes);

	auto newCrc = Crc::crc32(packet, _pPacketHeader->packetLength);
	if(oldCrc != newCrc) {
		LOG("收到被破坏的包[%s]", getPeerIP());
		delete[] body;
		delete[] packet;
		retPacket = nullptr;
		return false;
	}

	*retPacket = packet;
	return true;
}

bool TCPClient::setPacketHeader()
{
	if(nullptr == _pPacketHeader) {

		auto headerSizes = sizeof(PacketHeader);
		if(headerSizes > _recvBuffer.dataLength()) {
			return false;
		}

		char8* retBuffer = _recvBuffer.read(headerSizes);
		_pPacketHeader = reinterpret_cast<PPacketHeader>(retBuffer);
		_pPacketHeader->signature = ntohl(_pPacketHeader->signature);
		_pPacketHeader->packetLength = ntohl(_pPacketHeader->packetLength);
		_pPacketHeader->msgid = ntohl(_pPacketHeader->msgid);
		_pPacketHeader->reserved = ntohl(_pPacketHeader->reserved);
		_pPacketHeader->crc = ntohl(_pPacketHeader->crc);
	}

	return true;
}

void TCPClient::resetPacketHeader()
{
	safeDeleteArray(_pPacketHeader);
}

bool TCPClient::onRecv()
{
	int recvBytes = 0;

	char8 tempBuffer[s_bufferlength] = {0};

	while(true) {
		memset(tempBuffer, 0, s_bufferlength);
		recvBytes = ::recv(_fd, tempBuffer, s_bufferlength, 0);

		if(0 < recvBytes) {
			_recvBuffer.write(tempBuffer, recvBytes);
			if(!unPacket()) {
				return false;
			}

		} else if(0 == recvBytes) {
			LOG("接受数据时，发现网络会话已经关闭");
			return false;

		} else if(SOCKET_ERROR ==  recvBytes) {
			if(WSAEWOULDBLOCK != WSAGetLastError()) {
				SYSLOG("接受数据错误", WSAGetLastError());
				return false;
			} else {
				break;
			}
		}
	}

	LOG("收到读数据消息 - %u", _recvBuffer.dataLength());
	return true;
}

bool TCPClient::onSend()
{
	while(_sendBuffer.dataLength()) {

		unsigned readBytes = _sendBuffer.dataLength();
		auto sends = _sendBuffer.read(readBytes);
		auto retBytes = ::send(_fd, sends, readBytes, 0);

		if(retBytes != SOCKET_ERROR) {
			if(retBytes != readBytes) {// 数据没有发送完成，缓冲区填满了
				_sendBuffer.setPos(-retBytes);
				break;
			}

		} else {
			if(WSAGetLastError() == WSAEWOULDBLOCK) {
				// 发送缓冲区已经满了, 退出循环.
				break;
			} else {
				//TODO：应该调用onError函数
				return false;
			}
		}
	}

	return true;
}

}