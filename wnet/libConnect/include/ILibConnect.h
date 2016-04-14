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
@brief : �ڲ�ʹ��UTF8�����������ݣ�����ʹ��UNICODE
*************************************************************/
class __declspec(novtable) ILibConnect 
{
public:
	/**********************************************************
	*@brief : ���üӽ��ܻص�
	***********************************************************/
	virtual void setEncryptCallback(encryptCallback cb) = 0;
	virtual void setDecryptCallback(decrptCallbalck cb) = 0;

	/**********************************************************
	*@brief : ������־�ص�
	***********************************************************/
	virtual void setLogCallback(logCallback cb) = 0;

	/**********************************************************
	*@brief : ��ȡTCP����˽ӿ�
	***********************************************************/
	virtual ITCPServer* getTCPServer() = 0;

	/**********************************************************
	*@brief : ��ȡTCP�ͻ��˽ӿ�
	***********************************************************/
	virtual ITCPClient* getTCPClient() = 0;
};

/**********************************************************
*@brief : ����ģ���ʼ����������
***********************************************************/
extern "C" {
	_LIBCONNECT_EXPORTS ILibConnect* _LIBCONNECT_CALL openNetManager();
	_LIBCONNECT_EXPORTS void _LIBCONNECT_CALL closeNetManager();
}

}

#define NETMANAGER WStone::openNetManager()
#define CLOSENETMANAGER WStone::closeNetManager()

#endif //ILibConnect_2016_3_28_44_H_