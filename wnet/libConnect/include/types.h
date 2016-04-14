#ifndef types_2016_4_11_11_H_
#define types_2016_4_11_11_H_

namespace WStone {

	class ISession;

	typedef char char8;

	/**********************************************************
	*@ISession : 会话信息
	*@data : UTF8编码字符
	*@sizes : data数据大小
	***********************************************************/
	typedef void(*messageCallBack)(ISession*, const char8* data, unsigned sizes);

	/**********************************************************
	*@brief : 设置日志回调函数。debug版本默认输出到控制台和output窗口
	* release版本默认输出到当前工作目录下libConnect_x.log文件
	* 网络库启动时新建该该日志文件
	***********************************************************/
	typedef void (*logCallback)(const wchar_t*);

	/**********************************************************
	*@brief : 加解密通信数据。默认不加解密
	*@input : UTF8编码数据
	*@sizes : 使用者需要返回加解密后数据的大小
	***********************************************************/
	typedef char8* (*encryptCallback)(const char8* input, unsigned& sizes);
	typedef char8* (*decrptCallbalck)(const char8* input, unsigned& sizes);

}
#endif //types_2016_4_11_11_H_