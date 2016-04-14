#ifndef TCPServer_2016_3_28_06_H_
#define TCPServer_2016_3_28_06_H_

#include "include/ITCPServer.h"

namespace WStone {

class TCPServer : 
	public ITCPServer,
	public Singleton<TCPServer>
{
	friend class Singleton<TCPServer>;

public:
	TCPServer(void);

protected:
	~TCPServer(void);

public:
	virtual bool start(unsigned short port) override;
	virtual void stop() override;
	virtual const std::vector<ISession*>& getClients() override;
	virtual void mountMessage(unsigned int id, messageCallBack cb) override;

public:
	HANDLE getIOCP() { return _hIOCP; }
	void addClients(ISession*);
	void removeClients(ISession*);
	ISession* findClients(PSocketContext);
	messageCallBack getMessageCallBack(unsigned int id);
	bool handleIO(PSocketContext pSocketContext, 
		PIOContext pIOCntext,BOOL status);

private:
	bool initIOCP();
	void deInitIOCP();
	bool createListenerSocket(unsigned short post);
	void destroyListenerSocket();
	void clearClients();
	bool bind2IOCP(PSocketContext);
	bool postAccept(PIOContext);
	void handleAccept(PSocketContext, PIOContext);
	void handleRead(PSocketContext, PIOContext);
	void handleWrite(PSocketContext, PIOContext);
	bool handleError(PSocketContext);
	void handleClose(PSocketContext);

private:
	int _threads;
	HANDLE _hIOCP;
	bool _isStarted;
	FastMutex _mtmsgs;
	HANDLE* _pThreads;
	FastMutex _mtClients;
	LPFN_ACCEPTEX _fnAcceptEx;
	std::vector<ISession*> _clients;
	PSocketContext _listenSocketContext;
	const static int s_maxPostAccept = 10;
	std::map<unsigned int, messageCallBack> _msgs;
	LPFN_GETACCEPTEXSOCKADDRS _fnGetAcceptExSockAddrs;
};

}
#endif //TCPServer_2016_3_28_06_H_


