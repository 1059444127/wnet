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
			SYSLOG(L"���緢������", WSAGetLastError());
			break;
		}

		WSANETWORKEVENTS events;
		ret = WSAEnumNetworkEvents(pTCPClient->getHandle(), 
			pTCPClient->getSocketEvent(), &events);
		if(SOCKET_ERROR == ret) {
			SYSLOG(L"��ȡ�����¼�ʧ��", WSAGetLastError());
			break;
		}

		if(events.lNetworkEvents & FD_CONNECT) {
			if(0 == events.iErrorCode[FD_CONNECT_BIT]) {
				LOG(L"������������ɹ�");
				pTCPClient->setConneted(true);
			} else {
				SYSLOG(L"��������ʧ��", events.iErrorCode[FD_CONNECT_BIT]);
				break;
			}

		} else if(events.lNetworkEvents & FD_CLOSE) {
			pTCPClient->disconnect();
			pTCPClient->setConneted(false);
			LOG(L"�ر��������");
			break;

		} else if(events.lNetworkEvents & FD_READ) {
			LOG(L"�յ���������Ϣ - %u", ++s_countRead);
			if(0 == events.iErrorCode[FD_READ_BIT]) {
				if(!pTCPClient->onRecv()) {
					break;
				}
			} else {
				SYSLOG(L"�յ�����Ķ�������Ϣ", 
					events.iErrorCode[FD_WRITE_BIT]);
			}

		} else if(events.lNetworkEvents & FD_WRITE) {
			if(0 == events.iErrorCode[FD_CONNECT_BIT]) {
				if(!pTCPClient->onSend()) {
					break;
				}
			} else {
				SYSLOG(L"�յ������д������Ϣ", 
					events.iErrorCode[FD_WRITE_BIT]);
			}

		} else {
			LOG(L"����δ֪�¼� - %u", ++s_countUnknown);
			SYSLOG(L"����δ֪�¼�", WSAGetLastError());
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
_pHeader(nullptr),
_isConnected(false)
{

}


TCPClient::~TCPClient(void)
{
	disconnect();

	safeDeleteArray(_pHeader);
}

void TCPClient::mountMessage(
	unsigned int id, 
	messageCallBack cb)
{
	FastMutex::ScopedLock lock(_mtMsgs);

	auto itFound = _msgs.find(id);
	if(itFound == _msgs.end()) {
		_msgs.insert(std::make_pair(id, cb));
	}
}

messageCallBack TCPClient::getMessageCallBack(unsigned int id)
{
	FastMutex::ScopedLock lock(_mtMsgs);

	auto itFound = _msgs.find(id);
	if(itFound != _msgs.end()) {
		return itFound->second;
	}

	LOG(L"���ֻ�ȡδע����Ϣ�ص�����");
	return nullptr;
}

void TCPClient::setTimeOut(
	unsigned int recvs, 
	unsigned int sends)
{
	if(INVALID_SOCKET != _fd || _isAsync) {
		return;
	}

	setOption(SO_SNDTIMEO, sends, sizeof(unsigned int));
	setOption(SO_RCVTIMEO, recvs, sizeof(unsigned int));
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
		LOG(L"����client�׽���ʧ��[%u]", WSAGetLastError());
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
			LOG(L"�����߳�ʧ��[%u]", GetLastError());
			return false;
		}

	} else {
		_isAsync = false;
	}

	if(NO_ERROR != ::connect(_fd, (sockaddr*)&netAddr, sizeof(netAddr))) {
		if(WSAEWOULDBLOCK != WSAGetLastError()) {
			SYSLOG(L"���ӷ���ʧ��", WSAGetLastError());
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
	unsigned int msgid, 
	const char8* data, 
	unsigned int leng)
{
	if(nullptr == data || 0 == leng || !isValidSession()) {
		return false;
	}

	auto sendBuffer = PacketHeader::packet(msgid, data, leng);
	auto sendBytes = 0;

	sendBytes = ::send(_fd, sendBuffer, leng, 0);
	if(SOCKET_ERROR == sendBytes && 
		WSA_IO_PENDING != WSAGetLastError()) {
			SYSLOG(L"���ݷ���ʧ��", WSAGetLastError());
	}

	if(_isAsync && sendBytes != leng) {
		sendBytes = _sendBuffer.write(sendBuffer + sendBytes, 
			leng - sendBytes);
	}

	safeDeleteArray(sendBuffer);
	return true;
}

bool TCPClient::send(
	unsigned int msgid, 
	const char8* data)
{
	auto len = strlen(data);
	return TCPClient::send(msgid, data, len);
}

unsigned int TCPClient::recv(
	char8* buffer,
	unsigned int lens)
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
		return true; // �������İ�ͷ��Ϣ
	}

	if(s_signature != _pHeader->signature) {
		LOG(L"��ȡ���Ƿ������ݰ�[%s]���رջỰ", getPeerIP());
		return false;
	}

	auto bodyLengths = _pHeader->packetLength - headerSizes;
	if(_recvBuffer.dataLength() < bodyLengths) {
		return true;// �����������ȴ������İ�
	}

	char8* packet = nullptr;
	if(!isValidPacket(&packet)) {
		return false;

	} else {
		auto msgCB = getMessageCallBack(_pHeader->msgid);
		if(nullptr != msgCB) {// ����ע��Ļص���Ϣ
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
	auto oldCrc = _pHeader->crc;
	_pHeader->crc = 0;

	auto bodyBytes = (_pHeader->packetLength) - sizeof(PacketHeader);
	char8* body = _recvBuffer.read(bodyBytes);

	char8* packet = new char8[_pHeader->packetLength];
	memcpy(packet, _pHeader, sizeof(PacketHeader));
	memcpy(packet + sizeof(PacketHeader), body, bodyBytes);

	auto newCrc = Crc::crc32(packet, _pHeader->packetLength);
	if(oldCrc != newCrc) {
		LOG(L"�յ����ƻ��İ�[%s]", getPeerIP());
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
	if(nullptr == _pHeader) {

		auto headerSizes = sizeof(PacketHeader);
		if(headerSizes > _recvBuffer.dataLength()) {
			return false;
		}

		char8* retBuffer = _recvBuffer.read(headerSizes);
		_pHeader = reinterpret_cast<PPacketHeader>(retBuffer);
		_pHeader->signature = ntohl(_pHeader->signature);
		_pHeader->packetLength = ntohl(_pHeader->packetLength);
		_pHeader->msgid = ntohl(_pHeader->msgid);
		_pHeader->reserved = ntohl(_pHeader->reserved);
		_pHeader->crc = ntohl(_pHeader->crc);
	}

	return true;
}

void TCPClient::resetPacketHeader()
{
	safeDeleteArray(_pHeader);
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
			LOG(L"��������ʱ����������Ự�Ѿ��ر�");
			return false;

		} else if(SOCKET_ERROR ==  recvBytes) {
			if(WSAEWOULDBLOCK != WSAGetLastError()) {
				SYSLOG(L"�������ݴ���", WSAGetLastError());
				return false;
			} else {
				break;
			}
		}
	}

	LOG(L"�յ���������Ϣ - %u", _recvBuffer.dataLength());
	return true;
}

bool TCPClient::onSend()
{
	while(_sendBuffer.dataLength()) {

		unsigned int readBytes = _sendBuffer.dataLength();
		auto sends = _sendBuffer.read(readBytes);
		auto retBytes = ::send(_fd, sends, readBytes, 0);

		if(retBytes != SOCKET_ERROR) {
			if(retBytes != readBytes) {// ����û�з�����ɣ�������������
				_sendBuffer.setPos(-retBytes);
				break;
			}

		} else {
			if(WSAGetLastError() == WSAEWOULDBLOCK) {
				// ���ͻ������Ѿ�����, �˳�ѭ��.
				break;
			} else {
				//TODO��Ӧ�õ���onError����
				return false;
			}
		}
	}

	return true;
}

}