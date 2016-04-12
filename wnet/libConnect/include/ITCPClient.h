#ifndef ITCPClient_2016_4_1_45_H_
#define ITCPClient_2016_4_1_45_H_

namespace WStone {

class __declspec(novtable) ITCPClient
{
public:
	class Listener {
		virtual void onConnect(ISession*) = 0;
		virtual void onError(ISession*) = 0;
		virtual void onClose(ISession*) = 0;
	};

public:
	/**********************************************************
	*@brief : 设置网络超时时间，单位都是毫秒。
	*@recvs : 接受超时时间
	*@sends : 发送超时时间
	***********************************************************/
	virtual void setTimeOut(unsigned int recvs, unsigned int sends) = 0;

	/**********************************************************
	*@brief : 连接TCP服务
	*@ip : 服务IP地址
	*@port : 端口值
	*@async : 默认异步IO
	*@return : 成功建立会话返回true，反之则反
	***********************************************************/
	virtual bool connect(const wchar_t* ip, 
		unsigned short port, bool async = true) = 0;

	/**********************************************************
	*@brief : 获取会话
	*@return : 失败返回nullptr
	***********************************************************/
	virtual ISession* getSession() = 0;

	/**********************************************************
	*@brief : 检测是否连接成功
	*@return : 成功返回true，反之则反
	***********************************************************/
	virtual bool isValidSession() = 0;

	/**********************************************************
	*@brief : 端口与服务的连接
	***********************************************************/
	virtual void disconnect() = 0;

	/**********************************************************
	*@brief : 挂载网络消息
	*@id : 消息id
	*@cb : 消息回调函数
	*@return : 
	***********************************************************/
	virtual void mountMessage(unsigned int id, messageCallBack cb) = 0;
};

}

#endif //ITCPClient_2016_4_1_45_H_