#include "stdafx.h"

#include <limits>
#include <helpers/StringCovert.h>

void saveToFile(const WStone::char8* data, unsigned len)
{
	FILE* pFile = nullptr;
	fopen_s(&pFile, ".\\maomao.txt", "w");

	if(nullptr != pFile) {
		fwrite(data, sizeof(WStone::char8), len, pFile);
		fclose(pFile);
	}
}

void msgCallBack(WStone::ISession* session, 
				 const WStone::char8* data,
				 unsigned int lengs)
{
	if(1024 * 8 >= lengs) {
		OutputDebugStringW(L"收到的数据为\n");
		OutputDebugStringW(WStone::Utf8ToUnicode(data).c_str());
		std::wcout.imbue(std::locale("chs"));
		std::wcout << L"收到数据\n";
		std::wcout << WStone::Utf8ToUnicode(data).c_str() << "\n";

	} else {
		std::wcout << L"收到大数据，保存在文本maomao.txt中\n";
		saveToFile(data, lengs);
		session->send(11, data, lengs);
	}
}

void testTCPServer()
{
	WStone::ITCPServer* pTCP = NETMANAGER->getTCPServer();
	pTCP->start(6666);
	pTCP->mountMessage(10, msgCallBack);
	getchar();
	pTCP->stop();

	WStone::closeNetManager();
}

int _tmain(int argc, _TCHAR* argv[])
{
	std::wcout.imbue(std::locale("chs"));

	testTCPServer();

	getchar();
	
	return 0;
}

