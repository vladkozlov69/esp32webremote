#ifndef SIM5360_H_
#define SIM5360_H_

#include "Arduino.h"
#include <Regexp.h>
#include "ESPAsyncWebServer.h"

#define HTTPS_NETWORK_OPENING 2
#define HTTPS_NETWORK_OPENED 4 
#define HTTPS_SESSION_OPENED 7

class IntArray : public LinkedList<int> {
public:
  
  IntArray() : LinkedList(nullptr) {}
};

enum class SIM_RESULT
{
	OK,
	ERROR
};

enum class SIM_MODULE_GEN
{
	SIM53XX,
	SIM7XXX
};

struct SmsMessage
{
	String sender;
	String body;
};

class Sim5360
{
	char m_ApnName[20];
	Stream * m_DebugOut = NULL;
	Stream * m_Module = NULL;
	String lastResult = "";
	IntArray m_SmsMessages;
	SIM_MODULE_GEN m_ModuleType = SIM_MODULE_GEN::SIM53XX;
public:
	Sim5360();
	void begin(const char * apnName, Stream * module, Stream * debugOut);
	bool checkSimPresence();
	bool checkRegistration();
	String getOperatorName();
	bool checkPacketStatus();
	int getActiveCallsCount();
	int getSmsMessages();
	IntArray * getSmsIndexes() { return &m_SmsMessages; }
	SmsMessage getSmsMessage(int index);
	bool deleteSmsMessage(int index);
	bool simReset();
	SIM_RESULT sendSms(const char * recipient, const char * body);
	SIM_RESULT inetConnect();
	SIM_RESULT inetDisconnect();
	SIM_RESULT sendMail(const char * mailServer, int port, const char * username, const char * password,
			const char * mailFrom, const char * mailTo, const char * subj, const char * body);
	SIM_RESULT dial(const char * recipient);
	SIM_RESULT hangup();

	bool isGPRSNetworkOpened();
	bool openGPRSNetwork();
	bool closeGPRSNetwork();
	int8_t GPRSstate(void);

	boolean postData(const char *server, uint16_t port, bool secure, const char *URL, const char *body);
	void getNetworkInfo(void);
	int getHttpsState(void);
	bool waitForHttpsState(int desiredState, int timeOut);

	int getSignalLevel();
	String sendData(const char * command, const int timeout);
	String sendData(const char * command, const char * mandatorySignature, const int timeout);
	int waitForHttpReceive(int timeOut);
	int checkSmtpProgressStatus();
	bool sendDataAndCheckOk(const char * command);
private:
	char * sendDataAndParseResponse(const char * command, const char * regex, int captureNumber, char * buf);
	char * sendDataAndParseResponse(const char * command, const char * mandatorySignature, int timeout, const char * regex, int captureNumber, char * buf);

	bool sendDataAndCheckPrompt(const char * command);
	bool sendDataAndCheck(const char * command, const int timeout, const int wait, const char * expectedTail);
};

#endif