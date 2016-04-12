#include "stdafx.h"

#include "TCPServer.h"

#include "TCPServerSession.h"
#include "libConnect.h"

namespace WStone {

unsigned __stdcall workerLoop(void* arg)
{   
	auto pTCPServer = static_cast<TCPServer*>(arg);
	OVERLAPPED *pOverlapped = nullptr;
	PIOCPSocketContext  pSocketContext = nullptr;
	unsigned long transferedBytes = 0;
	BOOL ret = false;

	while(true) {
		ret = GetQueuedCompletionStatus(pTCPServer->getIOCP(), 
			&transferedBytes, (PULONG_PTR)&pSocketContext, 
			&pOverlapped, INFINITE);

		if(nullptr == pSocketContext){
			break;// 退出通知
		}

		if(!ret) {  
			if(!pTCPServer->doError(pSocketContext, GetLastError())) {
				break;
			}

			continue;
		}
			
		PIOCPIOContext pIOContext = CONTAINING_RECORD(
			pOverlapped, IOCPIOContext, overlapped);  

		if((0 == transferedBytes) && (pIOContext->opType & 
		   (OPT_READ + OPT_WRITE))) {  
			pTCPServer->doClose(pSocketContext);
			
			continue;
		}

		switch(pIOContext->opType) {
			case OPT_ACCEPT:
				pTCPServer->doAccept(pSocketContext, pIOContext); break;
			case OPT_READ:
				pTCPServer->doRead(pSocketContext, pIOContext); break;
			case OPT_WRITE:
				pTCPServer->doWrite(pSocketContext, pIOContext); break;
			default:
				LOG(L"操作类型参数异常"); break;
		}
	}

	return 0;
}

TCPServer::TCPServer(void) : 
_isStarted(false),
_iocp(nullptr),
_pThreads(nullptr),
_fnAcceptEx(nullptr),
_fnGetAcceptExSockAddrs(nullptr),
_listenSocketContext(nullptr),
_nThreads(2 * getCPUNumbers())
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

		for(int i = 0; i < _nThreads; ++i) {
			PostQueuedCompletionStatus(getIOCP(), 0, (ULONG_PTR)0, nullptr);
		}

		WaitForMultipleObjects(_nThreads, _pThreads, true, INFINITE);

		for(int i = 0; i < _nThreads; ++i){
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
	_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
	if(nullptr == _iocp) {
		SYSLOG(L"建立完成端口失败", GetLastError());
		return false;
	}

	return true;
}

void TCPServer::deInitIOCP()
{
	safeRelease(_iocp);
}

bool TCPServer::createListenerSocket(unsigned short port)
{
	_listenSocketContext = new IOCPSocketContext();

	_listenSocketContext->fd = 
		WSASocket(AF_INET, SOCK_STREAM, 0, 0, 0, WSA_FLAG_OVERLAPPED);
	if(INVALID_SOCKET == _listenSocketContext->fd) {
		LOG(L"初始化Socket失败[%d]", WSAGetLastError());
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
		LOG(L"bind函数执行错误[%d]", WSAGetLastError());
		return false;
	}

	if(SOCKET_ERROR == listen(_listenSocketContext->fd, 128)) {
		LOG(L"listen()函数执行出现错误[%d]", WSAGetLastError());
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
		SYSLOG(L"获取AcceptEx函数指针失败", WSAGetLastError()); 
		return false;  
	}  

	if(SOCKET_ERROR == WSAIoctl(
		_listenSocketContext->fd, SIO_GET_EXTENSION_FUNCTION_POINTER, 
		&guidAcceptExSockAddrs, sizeof(guidAcceptExSockAddrs), 
		&_fnGetAcceptExSockAddrs, sizeof(_fnGetAcceptExSockAddrs),   
		&dwBytes, nullptr, nullptr)) {  
		SYSLOG(L"获取GetAcceptExSockAddrs函数指针失败", WSAGetLastError());  
		return false; 
	}  

	for(int i = 0; i < s_maxPostAccept; i++) {
		auto pIOContext = _listenSocketContext->getNewIOContext();
		if(!postAccept(pIOContext)) {
			_listenSocketContext->removeIOContext(pIOContext);
		}
	}

	unsigned int id;
	_pThreads = new HANDLE[_nThreads];
	for(int i = 0; i < _nThreads; i++) {
		_pThreads[i] = (HANDLE)_beginthreadex(nullptr, 0, 
			&workerLoop, (void*)this, 0, &id);
	}

	LOG(L"IOCP初始化成功，建立[%d]个消息处理工作者", _nThreads);
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

ISession* TCPServer::findClients(PIOCPSocketContext pSocketContext)
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

void TCPServer::mountMessage(unsigned int id, messageCallBack cb)
{
	FastMutex::ScopedLock lock(_mtMsgs);

	auto itFound = _msgs.find(id);
	if(itFound == _msgs.end()) {
		_msgs.insert(std::make_pair(id, cb));
	}
}

messageCallBack TCPServer::getMessageCallBack(unsigned int id)
{
	FastMutex::ScopedLock lock(_mtMsgs);

	auto itFound = _msgs.find(id);
	if(itFound != _msgs.end()) {
		return itFound->second;
	}

	LOG(L"发现获取未注册消息回调函数");
	return nullptr;
}

bool TCPServer::bind2IOCP(PIOCPSocketContext pSocketContext)
{
	if(nullptr == CreateIoCompletionPort((HANDLE)pSocketContext->fd, 
		_iocp, (ULONG_PTR)pSocketContext, 0)) {
		LOG(L"执行bind2IOCP错误[%d]", GetLastError());
		return false;
	}

	return true;
}

bool TCPServer::postAccept(PIOCPIOContext pAcceotIOContext)
{
	pAcceotIOContext->opType = OPT_ACCEPT;

	pAcceotIOContext->fd = WSASocket(AF_INET, SOCK_STREAM, 
		IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);  
	if(INVALID_SOCKET == pAcceotIOContext->fd) {  
		SYSLOG(L"创建用于Accept的Socket失败", WSAGetLastError());
		return false;
	} 

	unsigned long dwBytes = 0;
	if(!_fnAcceptEx(_listenSocketContext->fd, 
		pAcceotIOContext->fd, pAcceotIOContext->overlappedBuffer.buf, 
		0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, &dwBytes, 
		&pAcceotIOContext->overlapped)) {  
		
		if(WSA_IO_PENDING != WSAGetLastError()) {  
			LOG(L"postAccept()失败[%d]", WSAGetLastError());
			return false;
		}
	}

	return true;
}

void TCPServer::doAccept(
	PIOCPSocketContext pSocketContext, 
	PIOCPIOContext pIOContext)
{
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

	LOG(L"创建新会话[%s-%d]", wClientIP.c_str(), clientPort);

	PIOCPSocketContext pClientSocketContext = new IOCPSocketContext();
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

void TCPServer::doRead(
	PIOCPSocketContext pSocketContext, 
	PIOCPIOContext pIOContext)
{
	auto pClient = dynamic_cast<TCPServerSession*>(
		findClients(pSocketContext));

	if(pClient->unPacket(pIOContext)) {
		pClient->postRecv(pIOContext);
	}
}

void TCPServer::doWrite(
	PIOCPSocketContext pSocketContext, 
	PIOCPIOContext pIOContext)
{
	
}

bool TCPServer::doError(
	PIOCPSocketContext pSocketContext, 
	unsigned long errID)
{
	auto* pClients = findClients(pSocketContext);

	if(WAIT_TIMEOUT == errID) {

		if(!pClients->isAlive()) {
			LOG(L"网络超时，检测到客户端异常退出");
			removeClients(pClients);
			return true;
		} else {
			LOG(L"网络超时！");
			return true;
		}

	} else if(ERROR_NETNAME_DELETED == errID) {
		LOG(L"检测到客户端异常退出");
		removeClients(pClients);
		return true;

	} else {
		LOG(L"完成端口操作出现错误，线程退出[%d]", errID);
		return false;
	}

	return true;
}

void TCPServer::doClose(PIOCPSocketContext pSocketContext)
{
	auto pClient = findClients(pSocketContext);
	LOG(L"客户端[%s-%d]退出系统", pClient->getPeerIP(),
		pClient->getPeerPort());

	removeClients(pClient);
}

}