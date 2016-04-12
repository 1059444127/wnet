#ifndef types_2016_4_11_11_H_
#define types_2016_4_11_11_H_


namespace WStone {

	class ISession;

	typedef char char8;

	/**********************************************************
	*@brief : data 是UTF8编码字符
	***********************************************************/
	typedef void(*messageCallBack)(ISession*, const char8* data, unsigned sizes);

	/**********************************************************
	*@brief : 设置日志回调函数。默认输出到控制台
	***********************************************************/
	typedef void (*logCallback)(const wchar_t*);

	/**********************************************************
	*@brief : 加密通信数据。默认不加密
	*@input : UTF8编码数据
	***********************************************************/
	typedef char8* (*encryptCallback)(const char8* input, unsigned sizes);

}


#endif //types_2016_4_11_11_H_