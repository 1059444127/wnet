#include "stdafx.h"

#include "TCPServerSession.h"

#include "TCPServer.h"
#include "libConnect.h"

namespace WStone {

TCPServerSession::TCPServerSession(
	TCPServer* pServer, 
	PSocketContext data) : 
_pServer(pServer),
_socketContext(data),
_recvBuffer(s_bufferlength),
_sendBuffer(s_bufferlength),
_pHeader(nullptr)
{

}

TCPServerSession::~TCPServerSession(void)
{
	safeDelete(_socketContext);
	safeDelete(_pHeader);
}

unsigned TCPServerSession::getHandle()
{
	if(nullptr != _socketContext) {
		return _socketContext->fd;
	}

	return INVALID_SOCKET;
}

const wchar_t* TCPServerSession::getPeerIP()
{
	if(nullptr != _socketContext) {
		return _socketContext->clientIP.c_str();
	}

	return nullptr;
}

unsigned short TCPServerSession::getPeerPort()
{
	if(nullptr != _socketContext) {
		return _socketContext->clientPort;
	}

	return 0;
}

bool TCPServerSession::send(
	unsigned msgid, 
	const char8* pdata, 
	unsigned length)
{
	return this->postSend(msgid, pdata, length);
}

bool TCPServerSession::postSend(
	unsigned msgid, 
	const char8* pdata, 
	unsigned length)
{
	auto sendBuffer = PacketHeader::packet(msgid, pdata, length);
	if(nullptr == sendBuffer) {
		return false;
	}

	auto netManager = LibConnect::getInstance();
	if(nullptr != netManager->getEncryptCallback()) {
		auto cryptSendBuffer = 
			netManager->getEncryptCallback()(sendBuffer, length);
		assert(nullptr != cryptSendBuffer);
		safeDeleteArray(sendBuffer);
		sendBuffer = cryptSendBuffer;
	}
	
	unsigned sendedBytes = 0;
	do{
		auto ioContext = _socketContext->getNewIOContext();
		ioContext->fd = getHandle();
		ioContext->opType = OPT_WRITE;

		auto minlen = min(length - sendedBytes, 
			ioContext->overlappedBuffer.len);
		memcpy(ioContext->buffer, sendBuffer + sendedBytes, minlen);
		sendedBytes += minlen;

		auto result = ::WSASend(getHandle(), 
			&ioContext->overlappedBuffer, 1, nullptr,
			0, &ioContext->overlapped, nullptr);

		if(SOCKET_ERROR == result && WSA_IO_PENDING != WSAGetLastError()) {
			LOG("发送数据错误[%s-%u]", getPeerIP(), msgid);
			SYSLOG("错误信息", WSAGetLastError());
			_socketContext->removeIOContext(ioContext);
			safeDeleteArray(sendBuffer);
			return false;
		}
	}while(sendedBytes < length);

	safeDeleteArray(sendBuffer);
	return true;
}

bool TCPServerSession::postRecv(PIOContext pIOContext)
{
	if(nullptr == pIOContext) {
		pIOContext = _socketContext->getNewIOContext();
		pIOContext->fd = getHandle();
		pIOContext->opType = OPT_READ;
	}

	pIOContext->resetBuffer();

	unsigned long dwFlags = 0;
	unsigned long dwBytes = 0;
	auto nBytes = WSARecv(getHandle(), 
		&pIOContext->overlappedBuffer, 1, &dwBytes, &dwFlags, 
		&pIOContext->overlapped, nullptr);

	if((SOCKET_ERROR == nBytes) && 
		(WSA_IO_PENDING != WSAGetLastError())) {
		SYSLOG("发送数据失败", WSAGetLastError());
		return false;
	}

	return true;
}

bool TCPServerSession::isAlive()
{
	auto ret = ::send(getHandle(), "", 0, 0);
	if(SOCKET_ERROR == ret && 
		WSA_IO_PENDING != WSAGetLastError()) {
		return false;
	}

	return true;
}

bool TCPServerSession::setPacketHeader()
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

void TCPServerSession::resetPacketHeader()
{
	safeDeleteArray(_pHeader);
}

bool TCPServerSession::isValidPacket(char8** retPacket)
{
	auto oldCrc = _pHeader->crc;
	_pHeader->crc = 0;

	unsigned retBytes = _pHeader->packetLength - sizeof(PacketHeader);
	char8* data = _recvBuffer.read(retBytes);

	char8* packet = new char8[_pHeader->packetLength];
	memcpy(packet, _pHeader, sizeof(PacketHeader));
	memcpy(packet + sizeof(PacketHeader), data, retBytes);

	auto newCrc = Crc::crc32(packet, _pHeader->packetLength);
	if(oldCrc != newCrc) {
		LOG("收到被破坏的包[%s]", getPeerIP());
		delete[] data;
		delete[] packet;
		*retPacket = nullptr;
		return false;
	}

	*retPacket = packet;
	return true;
}

bool TCPServerSession::unPacket(PIOContext pIOContext)
{
	auto netManager = LibConnect::getInstance();
	if(nullptr != netManager->getDecryptCallback()) {
		unsigned recvBytes = pIOContext->overlapped.InternalHigh;
		auto recvOldBuffer = netManager->getDecryptCallback()(
			pIOContext->buffer, recvBytes);
		assert(nullptr != recvOldBuffer);
		_recvBuffer.write(recvOldBuffer, recvBytes);
		safeDeleteArray(recvOldBuffer);

	} else {
		_recvBuffer.write(pIOContext->buffer, 
			pIOContext->overlapped.InternalHigh);
	}

	if(!setPacketHeader()) {
		return true;
	}

	if(s_signature != _pHeader->signature) {
		LOG("获取到非法的数据包[%s]，关闭会话", getPeerIP());
		return false;
	}

	if(_recvBuffer.dataLength() < 
		_pHeader->packetLength - sizeof(PacketHeader)) {
		return true;// 包不完整，等待完整的包
	}

	// 处理完整的包信息
	char8* packet = nullptr;
	if(!isValidPacket(&packet)) {
		return false;

	} else {
		auto msgCB = _pServer->getMessageCallBack(_pHeader->msgid);
		if(nullptr != msgCB) {// 调用注册的回调信息
			msgCB(this, packet + sizeof(PacketHeader),
			_pHeader->packetLength - sizeof(PacketHeader));
			delete[] packet;
		}
		resetPacketHeader();
	}

	return true;
}

}