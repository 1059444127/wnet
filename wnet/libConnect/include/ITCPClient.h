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
	*@brief : �������糬ʱʱ�䣬��λ���Ǻ��롣
	*@recvs : ���ܳ�ʱʱ��
	*@sends : ���ͳ�ʱʱ��
	***********************************************************/
	virtual void setTimeOut(unsigned int recvs, unsigned int sends) = 0;

	/**********************************************************
	*@brief : ����TCP����
	*@ip : ����IP��ַ
	*@port : �˿�ֵ
	*@async : Ĭ���첽IO
	*@return : �ɹ������Ự����true����֮��
	***********************************************************/
	virtual bool connect(const wchar_t* ip, 
		unsigned short port, bool async = true) = 0;

	/**********************************************************
	*@brief : ��ȡ�Ự
	*@return : ʧ�ܷ���nullptr
	***********************************************************/
	virtual ISession* getSession() = 0;

	/**********************************************************
	*@brief : ����Ƿ����ӳɹ�
	*@return : �ɹ�����true����֮��
	***********************************************************/
	virtual bool isValidSession() = 0;

	/**********************************************************
	*@brief : �˿�����������
	***********************************************************/
	virtual void disconnect() = 0;

	/**********************************************************
	*@brief : ����������Ϣ
	*@id : ��Ϣid
	*@cb : ��Ϣ�ص�����
	*@return : 
	***********************************************************/
	virtual void mountMessage(unsigned int id, messageCallBack cb) = 0;
};

}

#endif //ITCPClient_2016_4_1_45_H_