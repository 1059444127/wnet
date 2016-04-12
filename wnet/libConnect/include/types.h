#ifndef types_2016_4_11_11_H_
#define types_2016_4_11_11_H_


namespace WStone {

	class ISession;

	typedef char char8;

	/**********************************************************
	*@brief : data ��UTF8�����ַ�
	***********************************************************/
	typedef void(*messageCallBack)(ISession*, const char8* data, unsigned sizes);

	/**********************************************************
	*@brief : ������־�ص�������Ĭ�����������̨
	***********************************************************/
	typedef void (*logCallback)(const wchar_t*);

	/**********************************************************
	*@brief : ����ͨ�����ݡ�Ĭ�ϲ�����
	*@input : UTF8��������
	***********************************************************/
	typedef char8* (*encryptCallback)(const char8* input, unsigned sizes);

}


#endif //types_2016_4_11_11_H_