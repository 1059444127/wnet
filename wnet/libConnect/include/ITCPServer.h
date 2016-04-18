#ifndef ITCPServer_2016_3_28_42_H_
#define ITCPServer_2016_3_28_42_H_

#include <vector>

namespace WStone {

class __declspec(novtable) ITCPServer
{
public:
	/**********************************************************
	*@brief :  ֹͣ�������
	***********************************************************/
	virtual void stop() = 0;

	/**********************************************************
	*@brief : �����������
	***********************************************************/
	virtual bool start(unsigned short port) = 0;

	/**********************************************************
	*@brief : ��ȡ����Ŀͻ����б�
	***********************************************************/
	virtual const std::vector<ISession*>& getClients() = 0;

	/**********************************************************
	*@brief : ���ظ���Ȥ����Ϣ
	***********************************************************/
	virtual void mountMessage(unsigned id, messageCallBack cb) = 0;
};

}

#endif //ITCPServer_2016_3_28_42_H_