#ifndef TCPClient_2016_3_28_50_H_
#define TCPClient_2016_3_28_50_H_

#include "include/ISession.h"
#include "include/ITCPClient.h"

namespace WStone {

class TCPClient : 
	public ITCPClient, 
	public ISession,
	public Singleton<TCPClient>
{
	friend class Singleton<TCPClient>;

public:
	TCPClient();

protected:
	~TCPClient(void);

public:
	virtual bool isAlive() override;
	virtual void disconnect() override;
	virtual bool isValidSession() override;
	virtual ISession* getSession() override;
	virtual unsigned getHandle() override { return _fd; }
	virtual const wchar_t* getPeerIP() override {return _ip.c_str(); }
	virtual unsigned short getPeerPort() override {return _port; };
	virtual void setTimeOut(unsigned, unsigned) override;
	virtual bool send(unsigned, const char8*, unsigned) override;
	virtual unsigned recv(char8* buffer, unsigned lens) override;
	virtual bool connect(const wchar_t*, unsigned short, bool = true) override;
	virtual void mountMessage(unsigned id, messageCallBack cb) override;

public:
	bool onSend();
	bool onRecv();
	HANDLE getSocketEvent() { return _evtSocket; }
	void setConneted(bool val) { _isConnected =  val; }
	
private:
	bool unPacket();
	bool setPacketHeader();
	void resetPacketHeader();
	bool isValidPacket(char8** retPacket);
	bool setOption(int opname, int value, int sizes);
	messageCallBack getMessageCallBack(unsigned id);

private:
	int _fd;
	std::wstring _ip;
	unsigned short _port;
	bool _isAsync;
	bool _isConnected;
	ISession* _session;
	HANDLE _thread;
	HANDLE _evtSocket;
	LoopBuffer _sendBuffer;
	LoopBuffer _recvBuffer;
	PPacketHeader _pPacketHeader;
	FastMutex _mtMsgIDCB;
	std::map<unsigned, messageCallBack> _mapMsgIDCB;
};

}
#endif //TCPClient_2016_3_28_50_H_