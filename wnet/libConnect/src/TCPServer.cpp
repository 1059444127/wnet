#include "stdafx.h"

#include "TCPServer.h"

#include "libConnect.h"
#include "TCPServerSession.h"

namespace WStone {

unsigned __stdcall workerLoop(void* arg)
{   
	auto pTCPServer = static_cast<TCPServer*>(arg);

	unsigned long transferedBytes = 0;
	PSocketContext  pSocketContext = nullptr;
	OVERLAPPED *pOverlapped = nullptr;
	BOOL ret = false;

	while(true) {
		ret = GetQueuedCompletionStatus(pTCPServer->getIOCP(), 
			&transferedBytes, (PULONG_PTR)&pSocketContext, 
			&pOverlapped, INFINITE);

		if(nullptr == pSocketContext){
			break;// 退出通知
		}
		
		auto pIOContext = CONTAINING_RECORD(pOverlapped, 
			IOContext, overlapped);
		if(!pTCPServer->handleIO(pSocketContext, pIOContext, ret)) {
			break;
		}
	}

	return 0;
}

TCPServer::TCPServer(void) : 
_isStarted(false),
_hIOCP(nullptr),
_pThreads(nullptr),
_fnAcceptEx(nullptr),
_fnGetAcceptExSockAddrs(nullptr),
_listenSocketContext(nullptr),
_threads(2 * getCPUNumbers())
{
	
}

TCPServer::~TCPServer(void)
{
	this->stop();
}

bool TCPServer::start(unsigned short port) 
{
	if(!_isStarted) {
		if(initIOCP()) {
			if(createListenerSocket(port)) {
				_isStarted = true;
			}
		}
	}

	return _isStarted;
}

void TCPServer::stop()
{
	if(_isStarted) {

		for(int i = 0; i < _threads; ++i) {
			PostQueuedCompletionStatus(getIOCP(), 0, (ULONG_PTR)0, nullptr);
		}

		WaitForMultipleObjects(_threads, _pThreads, true, INFINITE);

		for(int i = 0; i < _threads; ++i){
			safeRelease(_pThreads[i]);
		}

		safeDelete(_pThreads);
	}

	deInitIOCP();
	destroyListenerSocket();
	clearClients();

	_isStarted = false;
}

bool TCPServer::initIOCP()
{
	_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
	if(nullptr == _hIOCP) {
		SYSLOG("建立完成端口失败", GetLastError());
		return false;
	}

	return true;
}

void TCPServer::deInitIOCP()
{
	safeRelease(_hIOCP);
}

bool TCPServer::createListenerSocket(unsigned short port)
{
	_listenSocketContext = new SocketContext();

	_listenSocketContext->fd = WSASocket(AF_INET, 
		SOCK_STREAM, 0, 0, 0, WSA_FLAG_OVERLAPPED);
	if(INVALID_SOCKET == _listenSocketContext->fd) {
		SYSLOG("初始化Socket失败", WSAGetLastError());
		return false;
	}

	if(!bind2IOCP(_listenSocketContext)) {
		return false;
	}

	sockaddr_in netAddr;  
	memset(&netAddr, 0, sizeof(sockaddr_in));
	netAddr.sin_family = AF_INET;  
	netAddr.sin_port = htons(port);
	netAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if(SOCKET_ERROR == bind(_listenSocketContext->fd, 
		(sockaddr*)&netAddr, sizeof(netAddr))) {
		SYSLOG("bind()执行错误", WSAGetLastError());
		return false;
	}

	if(SOCKET_ERROR == listen(_listenSocketContext->fd, 128)) {
		SYSLOG("listen()执行出现错误", WSAGetLastError());
		return false;
	}

	GUID guidAcceptEx = WSAID_ACCEPTEX;  
	GUID guidAcceptExSockAddrs = WSAID_GETACCEPTEXSOCKADDRS; 
	unsigned long dwBytes = 0;

	if(SOCKET_ERROR == WSAIoctl(
		_listenSocketContext->fd, SIO_GET_EXTENSION_FUNCTION_POINTER, 
		&guidAcceptEx, sizeof(guidAcceptEx), 
		&_fnAcceptEx, sizeof(_fnAcceptEx), 
		&dwBytes, nullptr, nullptr)) {  
		SYSLOG("获取AcceptEx函数指针失败", WSAGetLastError()); 
		return false;  
	}  

	if(SOCKET_ERROR == WSAIoctl(
		_listenSocketContext->fd, SIO_GET_EXTENSION_FUNCTION_POINTER, 
		&guidAcceptExSockAddrs, sizeof(guidAcceptExSockAddrs), 
		&_fnGetAcceptExSockAddrs, sizeof(_fnGetAcceptExSockAddrs),   
		&dwBytes, nullptr, nullptr)) {  
		SYSLOG("获取GetAcceptExSockAddrs函数指针失败", WSAGetLastError());  
		return false; 
	}  

	for(int i = 0; i < s_maxPostAccept; i++) {
		auto pIOContext = _listenSocketContext->getNewIOContext();
		if(!postAccept(pIOContext)) {
			_listenSocketContext->removeIOContext(pIOContext);
		}
	}

	unsigned id = 0;
	_pThreads = new HANDLE[_threads];
	for(int i = 0; i < _threads; i++) {
		_pThreads[i] = (HANDLE)_beginthreadex(nullptr, 0, 
			&workerLoop, (void*)this, 0, &id);
	}

	LOG("IOCP初始化成功，建立[%d]个消息处理工作者", _threads);
	return true;
}

void TCPServer::destroyListenerSocket()
{
	safeDelete(_listenSocketContext);
}

void TCPServer::addClients(ISession* session)
{
	FastMutex::ScopedLock lock(_mtClients);

	assert(nullptr != session);
	_clients.push_back(session);
}

void TCPServer::removeClients(ISession* session)
{
	FastMutex::ScopedLock lock(_mtClients);

	if(nullptr == session) {
		return;
	}

	for(auto it = _clients.begin(); it != _clients.end(); ++it) {
		
		if(*it == session) {
			_clients.erase(it);
			safeDelete(session);
			break;
		}
	}
}

const std::vector<ISession*>& TCPServer::getClients() 
{
	FastMutex::ScopedLock lock(_mtClients);

	return _clients;
}

ISession* TCPServer::findClients(PSocketContext pSocketContext)
{
	FastMutex::ScopedLock lock(_mtClients);

	assert(nullptr != pSocketContext);

	for(auto& it : _clients) {
		if(it->getHandle() == pSocketContext->fd) {
			return it;
		}
	}

	return nullptr;
}

void TCPServer::clearClients()
{
	FastMutex::ScopedLock lock(_mtClients);

	for(auto& it : _clients) {
		safeDelete(it);
	}

	_clients.clear();
}

void TCPServer::mountMessage(unsigned id, messageCallBack cb)
{
	FastMutex::ScopedLock lock(_mtmsgs);

	auto itFound = _msgs.find(id);
	if(itFound == _msgs.end()) {
		_msgs.insert(std::make_pair(id, cb));
	}
}

messageCallBack TCPServer::getMessageCallBack(unsigned id)
{
	FastMutex::ScopedLock lock(_mtmsgs);

	auto itFound = _msgs.find(id);
	if(itFound != _msgs.end()) {
		return itFound->second;
	}

	LOG("发现获取未注册消息回调函数");
	return nullptr;
}

bool TCPServer::bind2IOCP(PSocketContext pSocketContext)
{
	if(nullptr == CreateIoCompletionPort((HANDLE)pSocketContext->fd, 
		_hIOCP, (ULONG_PTR)pSocketContext, 0)) {
		LOG("执行bind2IOCP错误[%d]", GetLastError());
		return false;
	}

	return true;
}

bool TCPServer::postAccept(PIOContext pAcceotIOContext)
{
	pAcceotIOContext->opType = OPT_ACCEPT;

	pAcceotIOContext->fd = WSASocket(AF_INET, SOCK_STREAM, 
		IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);  
	if(INVALID_SOCKET == pAcceotIOContext->fd) {  
		SYSLOG("创建用于Accept的Socket失败", WSAGetLastError());
		return false;
	} 

	unsigned long dwBytes = 0;
	if(!_fnAcceptEx(_listenSocketContext->fd, 
		pAcceotIOContext->fd, pAcceotIOContext->overlappedBuffer.buf, 
		0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, &dwBytes, 
		&pAcceotIOContext->overlapped)) {  

			if(WSA_IO_PENDING != WSAGetLastError()) {  
				SYSLOG("postAccept()失败", WSAGetLastError());
				return false;
			}
	}

	return true;
}

bool TCPServer::handleIO(
	PSocketContext pSocketContext,
	PIOContext pIOContext,
	BOOL status) 
{
	if(!status) {  
		if(!handleError(pSocketContext)) {
			return false;
		}

		return true;
	}

	if( (0 == pIOContext->overlapped.InternalHigh) && 
		(pIOContext->opType & (OPT_READ + OPT_WRITE))) {  
		handleClose(pSocketContext);
		return true;
	}

	switch(pIOContext->opType) {
		case OPT_ACCEPT: handleAccept(pSocketContext, pIOContext); break;
		case OPT_READ: handleRead(pSocketContext, pIOContext); break;
		case OPT_WRITE: handleWrite(pSocketContext, pIOContext); break;
		default: LOG("操作类型参数异常"); break;
	}

	return true;
}

void TCPServer::handleAccept(
	PSocketContext pSocketContext, 
	PIOContext pIOContext)
{
	setsockopt(pIOContext->fd, SOL_SOCKET, 
		SO_UPDATE_ACCEPT_CONTEXT, 
		(char*)&pSocketContext->fd, sizeof(SOCKET));

	sockaddr_in* clientAddr = nullptr;
	sockaddr_in* localAddr = nullptr;

	int clientLen = sizeof(sockaddr_in);
	int localLen = clientLen;  

	_fnGetAcceptExSockAddrs(
		pIOContext->overlappedBuffer.buf, 0, 
		sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
		(sockaddr**)&localAddr, &localLen, 
		(sockaddr**)&clientAddr, &clientLen);

	auto clientPort = ntohs(clientAddr->sin_port);
	auto wClientIP = AnsiToUnicode(inet_ntoa(clientAddr->sin_addr));
	LOG("创建新会话[%s-%d]", wClientIP.c_str(), clientPort);

	PSocketContext pClientSocketContext = new SocketContext();
	pClientSocketContext->fd = pIOContext->fd;
	pClientSocketContext->clientIP = wClientIP;
	pClientSocketContext->clientPort = clientPort;

	if(!bind2IOCP(pClientSocketContext)){
		delete pClientSocketContext;
		pClientSocketContext = nullptr;
		return;
	}

	// 立即投递接受数据请求
	auto client = new TCPServerSession(this, pClientSocketContext);
	client->postRecv(nullptr);
	addClients(client);

	// 立即投递下一个accept请求
	pIOContext->resetBuffer();
	postAccept(pIOContext);
}

void TCPServer::handleRead(
	PSocketContext pSocketContext, 
	PIOContext pIOContext)
{
	auto pClient = dynamic_cast<TCPServerSession*>(
		findClients(pSocketContext));

	if(pClient->unPacket(pIOContext)) {
		pClient->postRecv(pIOContext);
	}
}

void TCPServer::handleWrite(
	PSocketContext pSocketContext, 
	PIOContext pIOContext)
{
	pSocketContext->removeIOContext(pIOContext);
}

bool TCPServer::handleError(PSocketContext pSocketContext)
{
	FastMutex::ScopedLock lock(_mtClients);

	auto* pClient = findClients(pSocketContext);
	if(nullptr == pClient) {
		return true;// 数据包过多可能造成该会话已经被清理掉
	}

	auto errID = WSAGetLastError();
	if(WAIT_TIMEOUT == errID) {
		if(!pClient->isAlive()) {
			LOG("网络超时！客户端离线");
			removeClients(pClient);
		} else {
			LOG("网络超时！客户端在线");
		}

	} else if(ERROR_NETNAME_DELETED == errID) {
		LOG("检测到客户端非正常离线");
		removeClients(pClient);

	} else {
		SYSLOG("完成端口操作出现错误，线程退出", errID);
		return false;
	}

	return true;
}

void TCPServer::handleClose(PSocketContext pSocketContext)
{
	auto pClient = findClients(pSocketContext);
	LOG("客户端[%s-%d]正常离线", pClient->getPeerIP(),
		pClient->getPeerPort());

	removeClients(pClient);
}

}