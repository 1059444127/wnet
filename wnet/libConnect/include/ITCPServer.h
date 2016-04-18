#ifndef ITCPServer_2016_3_28_42_H_
#define ITCPServer_2016_3_28_42_H_

#include <vector>

namespace WStone {

class __declspec(novtable) ITCPServer
{
public:
	/**********************************************************
	*@brief :  停止网络服务
	***********************************************************/
	virtual void stop() = 0;

	/**********************************************************
	*@brief : 开启网络服务
	***********************************************************/
	virtual bool start(unsigned short port) = 0;

	/**********************************************************
	*@brief : 获取接入的客户端列表
	***********************************************************/
	virtual const std::vector<ISession*>& getClients() = 0;

	/**********************************************************
	*@brief : 挂载感兴趣的消息
	***********************************************************/
	virtual void mountMessage(unsigned id, messageCallBack cb) = 0;
};

}

#endif //ITCPServer_2016_3_28_42_H_