#ifndef ILibConnect_2016_3_28_44_H_
#define ILibConnect_2016_3_28_44_H_

#ifdef LIBCONNECT_EXPORTS
#define _LIBCONNECT_EXPORTS __declspec(dllexport)
#else
#define _LIBCONNECT_EXPORTS __declspec(dllimport)
#endif
#define _LIBCONNECT_CALL __stdcall

#include "types.h"
#include "ISession.h"
#include "ITCPServer.h"
#include "ITCPClient.h"

namespace WStone {

/************************************************************
@brief : 内部使用UTF8处理网络数据，程序使用UNICODE
*************************************************************/
class __declspec(novtable) ILibConnect 
{
public:
	/**********************************************************
	*@brief : 设置加解密回调
	***********************************************************/
	virtual void setEncryptCallback(encryptCallback cb) = 0;
	virtual void setDecryptCallback(decrptCallbalck cb) = 0;

	/**********************************************************
	*@brief : 设置日志回调
	***********************************************************/
	virtual void setLogCallback(logCallback cb) = 0;

	/**********************************************************
	*@brief : 获取TCP服务端接口
	***********************************************************/
	virtual ITCPServer* getTCPServer() = 0;

	/**********************************************************
	*@brief : 获取TCP客户端接口
	***********************************************************/
	virtual ITCPClient* getTCPClient() = 0;
};

/**********************************************************
*@brief : 网络模块初始化和清理函数
***********************************************************/
extern "C" {
	_LIBCONNECT_EXPORTS ILibConnect* _LIBCONNECT_CALL openNetManager();
	_LIBCONNECT_EXPORTS void _LIBCONNECT_CALL closeNetManager();
}

}

#define NETMANAGER WStone::openNetManager()
#define CLOSENETMANAGER WStone::closeNetManager()

#endif //ILibConnect_2016_3_28_44_H_