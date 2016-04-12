#ifndef TCPSession_2016_4_1_46_H_
#define TCPSession_2016_4_1_46_H_

#include "include/ISession.h"

namespace WStone {

class TCPServer;

class TCPServerSession : public ISession
{
public:
	TCPServerSession(TCPServer* pServer, PIOCPSocketContext data);
	~TCPServerSession();

public:
	virtual bool isAlive() override;
	virtual unsigned int getHandle() override;
	virtual const wchar_t* getPeerIP() override;
	virtual unsigned short getPeerPort() override;
	virtual bool send(unsigned int, const char8*, unsigned int) override;
	virtual bool send(unsigned int, const char8*) override;
	virtual unsigned int recv(char8*, unsigned int) override { return 0; };

public:
	bool postSend(unsigned int, const char8*, unsigned int);
	bool postRecv(PIOCPIOContext pIOContext);
	bool setPacketHeader();
	void resetPacketHeader();
	bool isValidPacket(char8** retPacket);
	PPacketHeader getPacketHeader() { return _pHeader; }
	bool unPacket(PIOCPIOContext pIOContext);

private:
	TCPServer* _pServer;
	LoopBuffer _recvBuffer;
	PPacketHeader _pHeader;
	PIOCPSocketContext _socketContext;
};

}

#endif //TCPSession_2016_4_1_46_H_