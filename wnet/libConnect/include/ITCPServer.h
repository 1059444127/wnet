#ifndef ITCPServer_2016_3_28_42_H_
#define ITCPServer_2016_3_28_42_H_

#include <vector>

namespace WStone {

class __declspec(novtable) ITCPServer
{
public:
	virtual void stop() = 0;
	virtual bool start(unsigned short port) = 0;
	virtual const std::vector<ISession*>& getClients() = 0;
	virtual void mountMessage(unsigned int id, messageCallBack cb) = 0;
};

}

#endif //ITCPServer_2016_3_28_42_H_