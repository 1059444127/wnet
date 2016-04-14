#ifndef types_2016_4_11_11_H_
#define types_2016_4_11_11_H_

namespace WStone {

	class ISession;

	typedef char char8;

	/**********************************************************
	*@ISession : �Ự��Ϣ
	*@data : UTF8�����ַ�
	*@sizes : data���ݴ�С
	***********************************************************/
	typedef void(*messageCallBack)(ISession*, const char8* data, unsigned sizes);

	/**********************************************************
	*@brief : ������־�ص�������debug�汾Ĭ�����������̨��output����
	* release�汾Ĭ���������ǰ����Ŀ¼��libConnect_x.log�ļ�
	* ���������ʱ�½��ø���־�ļ�
	***********************************************************/
	typedef void (*logCallback)(const wchar_t*);

	/**********************************************************
	*@brief : �ӽ���ͨ�����ݡ�Ĭ�ϲ��ӽ���
	*@input : UTF8��������
	*@sizes : ʹ������Ҫ���ؼӽ��ܺ����ݵĴ�С
	***********************************************************/
	typedef char8* (*encryptCallback)(const char8* input, unsigned& sizes);
	typedef char8* (*decrptCallbalck)(const char8* input, unsigned& sizes);

}
#endif //types_2016_4_11_11_H_