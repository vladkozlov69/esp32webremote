#ifndef SIM5360_H_
#define SIM5360_H_

#include "Arduino.h"
#include <Regexp.h>
#include "ESPAsyncWebServer.h"

class IntArray : public LinkedList<int> {
public:
  
  IntArray() : LinkedList(nullptr) {}
  
//   bool containsIgnoreCase(const String& str){
//     for (const auto& s : *this) {
//       if (str.equalsIgnoreCase(s)) {
//         return true;
//       }
//     }
//     return false;
//   }
};

enum class SIM_RESULT
{
	OK,
	ERROR
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
public:
	Sim5360();
	void begin(const char * apnName, Stream * module, Stream * debugOut);
	bool checkSimPresent();
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
	int getSignalLevel();
	String sendData(const char * command, const int timeout);
	int checkSmtpProgressStatus();
	bool sendDataAndCheckOk(const char * command);
private:
	char * sendDataAndParseResponse(const char * command, const char * regex, int captureNumber, char * buf);
	bool sendDataAndCheckPrompt(const char * command);
	bool sendDataAndCheck(const char * command, const int timeout, const int wait, const char * expectedTail);
};

#endif