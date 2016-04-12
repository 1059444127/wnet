#ifndef TCPServer_2016_3_28_06_H_
#define TCPServer_2016_3_28_06_H_

#include "include/ITCPServer.h"

namespace WStone {

class TCPServer : 
	public ITCPServer,
	public SingletonI<TCPServer>
{
	friend class SingletonI<TCPServer>;

public:
	TCPServer(void);

protected:
	~TCPServer(void);

public:
	HANDLE getIOCP() { return _iocp; }

public:
	virtual bool start(unsigned short port) override;
	virtual void stop() override;
	virtual const std::vector<ISession*>& getClients() override;
	virtual void mountMessage(unsigned int id, messageCallBack cb) override;

public:
	void addClients(ISession*);
	void removeClients(ISession*);
	ISession* findClients(PIOCPSocketContext);
	messageCallBack getMessageCallBack(unsigned int id);
	void doAccept(PIOCPSocketContext, PIOCPIOContext);
	void doRead(PIOCPSocketContext, PIOCPIOContext);
	void doWrite(PIOCPSocketContext, PIOCPIOContext);
	bool doError(PIOCPSocketContext, unsigned long errID);
	void doClose(PIOCPSocketContext);

private:
	bool initIOCP();
	void deInitIOCP();
	bool createListenerSocket(unsigned short post);
	void destroyListenerSocket();
	void clearClients();
	bool bind2IOCP(PIOCPSocketContext);
	bool postAccept(PIOCPIOContext);

private:
	HANDLE _iocp;
	int _nThreads;
	HANDLE* _pThreads;
	bool _isStarted;
	LPFN_ACCEPTEX _fnAcceptEx;
	PIOCPSocketContext _listenSocketContext;
	const static int s_maxPostAccept = 10;
	FastMutex _mtMsgs;
	FastMutex _mtClients;
	std::vector<ISession*> _clients;
	LPFN_GETACCEPTEXSOCKADDRS _fnGetAcceptExSockAddrs;
	std::map<unsigned int, messageCallBack> _msgs;
};

}
#endif //TCPServer_2016_3_28_06_H_


