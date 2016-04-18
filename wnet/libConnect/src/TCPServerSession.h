#ifndef TCPSession_2016_4_1_46_H_
#define TCPSession_2016_4_1_46_H_

#include "include/ISession.h"

namespace WStone {

class TCPServer;

class TCPServerSession : public ISession
{
public:
	TCPServerSession(TCPServer* pServer, PSocketContext data);
	~TCPServerSession();

public:
	virtual bool isAlive() override;
	virtual unsigned getHandle() override;
	virtual const wchar_t* getPeerIP() override;
	virtual unsigned short getPeerPort() override;
	virtual bool send(unsigned, const char8*, unsigned) override;
	virtual unsigned recv(char8*, unsigned) override { return 0; }

public:
	bool setPacketHeader();
	void resetPacketHeader();
	bool unPacket(PIOContext pIOContext);
	bool postRecv(PIOContext pIOContext);
	bool isValidPacket(char8** retPacket);
	bool postSend(unsigned, const char8*, unsigned);
	PPacketHeader getPacketHeader() { return _pHeader; }

private:
	TCPServer* _pServer;
	LoopBuffer _recvBuffer;
	LoopBuffer _sendBuffer;
	PPacketHeader _pHeader;
	PSocketContext _socketContext;
};

}

#endif //TCPSession_2016_4_1_46_H_