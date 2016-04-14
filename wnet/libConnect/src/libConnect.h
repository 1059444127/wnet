#ifndef libConnect_2016_3_28_06_H_
#define libConnect_2016_3_28_06_H_

#include "include/ILibConnect.h"

namespace WStone {

class LibConnect : 
	public ILibConnect,
	public Singleton<LibConnect>
{
	friend class Singleton<LibConnect>;

public:
	LibConnect();

protected:
	~LibConnect();

public:
	virtual ITCPServer* getTCPServer() override;
	virtual ITCPClient* getTCPClient() override;

public:
	void setEncryptCallback(encryptCallback cb) override { _encryptCB = cb; }
	encryptCallback getEncryptCallback() { return _encryptCB; }
	void setDecryptCallback(decrptCallbalck cb) override { _decryptCB = cb; }
	decrptCallbalck getDecryptCallback() { return _decryptCB; }
	void setLogCallback(logCallback cb) override { _logCB = cb; };
	logCallback getLogCallback() { return _logCB; }

private:
	void loadSocketLib();
	void unLoadSocketLib();

private:
	encryptCallback _encryptCB;
	decrptCallbalck _decryptCB;
	logCallback _logCB;
};

}

#endif //libConnect_2016_3_28_06_H_