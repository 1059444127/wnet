#ifndef ISession_2016_3_29_12_H_
#define ISession_2016_3_29_12_H_

namespace WStone {

typedef char char8; // ����ͨѶʹ��UTF8����

class __declspec(novtable) ISession
{
public:
	virtual ~ISession() { }

public:
	/************************************************************
	@brief : ��ȡ�Ự�׽���
	*************************************************************/
	virtual unsigned int getHandle() = 0;

	/**********************************************************
	*@brief : ��ȡ�˵�IP��ַ��Ϣ
	***********************************************************/
	virtual const wchar_t* getPeerIP() = 0;

	/**********************************************************
	*@brief : ��ȡ�˵�˿���Ϣ
	***********************************************************/
	virtual unsigned short getPeerPort() = 0;

	/**********************************************************
	*@brief : �жϻỰ�Ƿ�����
	*@return : �ɹ�����true
	***********************************************************/
	virtual bool isAlive() = 0;
	
	/**********************************************************
	*@brief : ִ�����ݷ���
	*@id : ��ϢID
	*@msg : ��Ϣ��Ϣ��### UTF8���� ###
	*@lens : ���͵����ݳ���
	*@return : �ɹ�����true����֮��
	***********************************************************/
	virtual bool send(unsigned int id, const char8* msg) = 0;
	virtual bool send(unsigned int id, const char8* msg, unsigned int lens) = 0;

	/**********************************************************
	*@brief : ִ�л�ȡ����, �÷���ֻ����������ģʽ
	*@buffer : ���ܻ�����
	*@lens : ���ܻ�������С
	*@return : ���ػ�ȡ�������ݣ����ֽ�����
	***********************************************************/
	virtual unsigned int recv(char8* buffer, unsigned int lens) = 0;
};

}

#endif //ISession_2016_3_29_12_H_