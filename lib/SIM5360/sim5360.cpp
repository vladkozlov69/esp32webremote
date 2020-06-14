#include "sim5360.h"

Sim5360::Sim5360()
{

}

void Sim5360::begin(const char * apnName, Stream * module, Stream * debugOut)
{
	strncpy(m_ApnName, apnName, sizeof(m_ApnName));
	m_Module = module;
	m_DebugOut = debugOut;
}

bool Sim5360::checkSimPresence()
{
	// in case of SIM error, module returns "+CME ERROR: SIM failure". "OK" otherwise
	return sendDataAndCheckOk("AT+CPIN?");
}

bool Sim5360::simReset()
{
	return sendDataAndCheckOk("AT+CRESET");
}

SIM_RESULT Sim5360::sendSms(const char * recipient, const char * body)
{
	if (sendDataAndCheckOk("AT+CMGF=1"))//Because we want to send the SMS in text mode
	{
		char cmgs[25];
		sprintf(cmgs, "AT+CMGS=\"%s\"", recipient);
		sendDataAndCheckPrompt(cmgs);
		sendDataAndCheckPrompt(body);
		m_Module->println((char)26);
		delay(100);
		return SIM_RESULT::OK;
	}

	return SIM_RESULT::ERROR;
}

SIM_RESULT Sim5360::inetConnect()
{
	char cgdcont[100];
	sprintf(cgdcont,"AT+CGDCONT=1, \"IP\", \"%s\", \"0.0.0.0\"",m_ApnName);

	if (sendDataAndCheckOk(cgdcont))
	{
		if (sendDataAndCheckOk("AT+CGATT=1"))
		{
			if (sendDataAndCheckOk("AT+CGACT=1, 1"))
			{
				return SIM_RESULT::OK;
			}
		}
	}

	return SIM_RESULT::ERROR;
}

SIM_RESULT Sim5360::inetDisconnect()
{
	if (sendDataAndCheckOk("AT+CGACT=0, 1"))
	{
		if (sendDataAndCheckOk("AT+CGATT=0"))
		{
			if (sendDataAndCheckOk("AT+CGDCONT=1"))
			{
				return SIM_RESULT::OK;
			}
		}
	}

	return SIM_RESULT::ERROR;
}

SIM_RESULT Sim5360::sendMail(const char * mailServer, int port,
		const char * username, const char * password,
		const char * mailFrom, const char * mailTo,
		const char * subj, const char * body)
{
	char buf[200];

	sprintf(buf, "AT+CSMTPSSRV=\"%s\",%d", mailServer, port);

	if (sendDataAndCheckOk(buf))
	{
		sprintf(buf, "AT+CSMTPSAUTH=1,\"%s\",\"%s\"", username, password);
		if (sendDataAndCheckOk(buf))
		{
			sprintf(buf, "AT+CSMTPSFROM=\"%s\",\"%s\"", mailFrom, mailFrom);
			if (sendDataAndCheckOk(buf))
			{
				sprintf(buf, "AT+CSMTPSRCPT=0, 0, \"%s\", \"%s\"", mailTo, mailTo);
				if (sendDataAndCheckOk(buf))
				{
					sprintf(buf, "AT+CSMTPSSUB=%d, \"utf-8\"", strlen(subj));
					if (sendDataAndCheckPrompt(buf))
					{
						sendDataAndCheckOk(subj);
						
						sprintf(buf, "AT+CSMTPSBODY=%d", strlen(body));
						if (sendDataAndCheckPrompt(buf))
						{
							if (sendDataAndCheckOk(body))
							{
								if (sendDataAndCheckOk("AT+CSMTPSSEND"))
								{
									return SIM_RESULT::OK;
								}
							}
						}
					}
				}
			}
		}
	}

	return SIM_RESULT::ERROR;
}

SIM_RESULT Sim5360::dial(const char * recipient)
{
	char atd[20];
	sprintf(atd, "ATD%s;", recipient);

	if (sendDataAndCheckOk(atd))
	{
		return SIM_RESULT::OK;
	}

	return SIM_RESULT::ERROR;
}

SIM_RESULT Sim5360::hangup()
{
	if (sendDataAndCheckOk("AT+CVHU=0"))
	{
		if (sendDataAndCheckOk("ATH"))
		{
			return SIM_RESULT::OK;
		}
	}

	return SIM_RESULT::ERROR;
}

bool Sim5360::checkRegistration()
{
	char buf[20];
	if (sendDataAndParseResponse("AT+CREG?", "%+CREG: %d,(%d)", 0, buf))
	{
		return strcmp("1", buf) == 0;
	}

	return false;
}

bool Sim5360::checkPacketStatus()
{
	char buf[20];
	if (sendDataAndParseResponse("AT+CGACT?", "%+CGACT: %d,(%d)", 0, buf))
	{
		return strcmp("1", buf) == 0;
	}

	return false;
}

int Sim5360::checkSmtpProgressStatus()
{
	char buf[5];
	if (sendDataAndParseResponse("AT+CSMTPSSEND?", "%+CSMTPSSEND: (%d)", 0, buf))
	{
		return atoi(buf);
	}

	return -1;
}

String Sim5360::getOperatorName()
{
	char buf[50];
	if (sendDataAndParseResponse("AT+COPS?", "%+COPS: .*,\"([%w%s]+)\",", 0, buf))
	{
		return String(buf);
	}

	return "NO REG";
}

int Sim5360::getSignalLevel()
{
	char buf[20];
	if (sendDataAndParseResponse("AT+CSQ", "%+CSQ: (%d+),", 0, buf))
	{
		return atoi(buf);
	}

	return 99;
}

int Sim5360::getActiveCallsCount()
{
	MatchState ms;
	lastResult = sendData("AT+CLCC", 2000);

	ms.Target((char *) (lastResult.c_str()));

	if (ms.Match("(%+CLCC:)") == REGEXP_MATCHED)
	{
		return ms.level;
	}

	return 0;
}

int Sim5360::getSmsMessages()
{
	m_SmsMessages.free();

	if (sendDataAndCheckOk("AT+CMGF=1"))
	{
		if (sendDataAndCheckOk("AT+CMGL=\"ALL\""))
		{
			MatchState ms;
			char buf[10];
			unsigned int matchIndex = 0;
			ms.Target((char *) (lastResult.c_str()));

			while (true)
  			{			
    			if (ms.Match("%+CMGL: (%d+),", matchIndex) == REGEXP_MATCHED)
    			{
      				for (int j = 0; j < ms.level; j++)
					{
						m_SmsMessages.add(atoi(ms.GetCapture(buf, j)));
					}
      				// move past matching string
      				matchIndex = ms.MatchStart + ms.MatchLength;
   				}  
    			else
				{
      				break;  // no match or regexp error
				}

  			}
		}
	}

	return m_SmsMessages.length();
}

SmsMessage Sim5360::getSmsMessage(int index)
{
	char buf[300];
	SmsMessage result;
	sprintf(buf, "AT+CMGR=%d", index);
	if (sendDataAndCheckOk(buf))
	{
		MatchState ms;
		ms.Target((char *) (lastResult.c_str()));
		if (ms.Match("%+CMGR: \".*\",\"([%+%d]+)\",\"?.*\",\"(.*)\"\r\n(.*)\r\n\r\nOK") == REGEXP_MATCHED)
		{
			result.sender = ms.GetCapture(buf, 0);
			result.body = ms.GetCapture(buf, ms.level-1);
		}
	}

	return result;
}

bool Sim5360::deleteSmsMessage(int index)
{
	char buf[20];
	sprintf(buf, "AT+CMGD=%d", index);

	return sendDataAndCheckOk(buf);
}

bool Sim5360::sendDataAndCheckOk(const char * command)
{
	return sendDataAndCheck(command, 2000, 100, "\r\nOK\r\n");
}

bool Sim5360::sendDataAndCheckPrompt(const char * command)
{
	return sendDataAndCheck(command, 2000, 100, "\r\n>");
}

char * Sim5360::sendDataAndParseResponse(const char * command, const char * regex, int captureNumber, char * buf)
{
	return sendDataAndParseResponse(command, NULL, 2000, regex, captureNumber, buf);
}

char * Sim5360::sendDataAndParseResponse(const char * command, const char * mandatorySignature, int timeout, const char * regex, int captureNumber, char * buf)
{
	MatchState ms;
	lastResult = sendData(command, mandatorySignature, timeout);
	ms.Target((char *) (lastResult.c_str()));

	if (ms.Match(regex) == REGEXP_MATCHED)
	{
		return ms.GetCapture (buf, 0);
	}

	return NULL;
}

bool Sim5360::sendDataAndCheck(const char * command, const int timeout, const int wait, const char * expectedTail)
{
	lastResult = sendData(command, timeout);
	delay(wait);
	return lastResult.endsWith(expectedTail);
}

String Sim5360::sendData(const char * command, const int timeout)
{
	return sendData(command, NULL, timeout);
}

String Sim5360::sendData(const char * command, const char * mandatorySignature, const int timeout)
{
	String response = "";

	m_Module->println(command);

	if (m_DebugOut)
	{
		m_DebugOut->print(" >> ");
		m_DebugOut->println(command);
	}

	unsigned long int time = millis();

	bool responseEndDetected = false;
	while((!responseEndDetected) && (time+timeout) > millis())
	{
		while((!responseEndDetected) && m_Module->available())
		{
			char c = m_Module->read();
			response += c;
			responseEndDetected = (response.endsWith("\r\nOK\r\n") 
				&& (NULL == mandatorySignature || response.indexOf(mandatorySignature) > 0)) 
			
			|| response.endsWith("\r\nERROR\r\n");
		}
	}

	if(m_DebugOut)
	{
		m_DebugOut->print(" << ");
		m_DebugOut->println(response);
	}

	return response;
}

bool Sim5360::isGPRSNetworkOpened()
{
	char buf[5];
	if (sendDataAndParseResponse("AT+NETOPEN?", "%+NETOPEN: (%d),%d", 0, buf))
	{
		return 1 == atoi(buf);
	}

	return false;
}

boolean Sim5360::openGPRSNetwork()
{
	// connect in transparent mode
	sendDataAndCheckOk("AT+CIPMODE=1");

	if (isGPRSNetworkOpened())
	{
		return true;
	}

	sendDataAndCheckOk("AT+NETOPEN");

	for (int i = 0; i < 10; i++)
	{
		if (isGPRSNetworkOpened())
		{
			return true;

			delay(200);
		}
	}

	return false;
}

bool Sim5360::closeGPRSNetwork()
{
	char buf[5];
	if (sendDataAndParseResponse("AT+NETCLOSE", "+NETCLOSE:", 5000, "%+NETCLOSE: (%d)", 0, buf))
	{
		return 0 == atoi(buf);
	}

	return false;
}

int Sim5360::getHttpsState(void)
{
	char buf[5];
	if (sendDataAndParseResponse("AT+CHTTPSSTATE", "%+CHTTPSSTATE: (%d)", 0, buf))
	{
		return atoi(buf);
	}

	return -1;
}

bool Sim5360::waitForHttpsState(int desiredState, int timeOut)
{
	unsigned long startedAt = millis();
	while (millis() < startedAt + timeOut)
	{
		if (desiredState == getHttpsState())
		{
			return true;
		}

		delay(200);
	}

	return false;
}

int8_t Sim5360::GPRSstate(void)
{
	char buf[5];
	if (sendDataAndParseResponse("AT+CGATT?", "%+CGATT: (%d)", 0, buf))
	{
		return atoi(buf);
	}

	return -1;
}

int Sim5360::waitForHttpReceive(int timeOut)
{
	unsigned long startedAt = millis();

	while (millis() < startedAt + timeOut)
	{
		char buf[10];
		if (sendDataAndParseResponse("AT+CHTTPSRECV?", "%+CHTTPSRECV: LEN,(%d+)", 0, buf));
		{
			int replyLen = atoi(buf);
			if (replyLen > 0)
			{
				return replyLen;
			}
		}
		delay(200);
	}
	
	return -1;
}

boolean Sim5360::postData(const char *server, uint16_t port, bool secure, const char *URL, const char *body)
{
	// Sample request URL: "GET /dweet/for/{deviceID}?temp={temp}&batt={batt} HTTP/1.1\r\nHost: dweet.io\r\n\r\n"

	if (openGPRSNetwork())
	{
		m_DebugOut->println(F("GPRS Network opened for HTTP(S) post"));
	}

	int httpState = getHttpsState();
	// check if state != 7, then call start
	if (HTTPS_SESSION_OPENED != httpState)
	{
  		sendDataAndCheckOk("AT+CHTTPSSTART");
	}

	if (HTTPS_SESSION_OPENED == httpState || waitForHttpsState(HTTPS_NETWORK_OPENED, 5000)) // it could be HTTPS_SESSION_OPENED as well
	{
		char auxStr[200];
		sprintf(auxStr, "AT+CHTTPSOPSE=\"%s\",%d,%d", server, port, secure ? 2 : 1);
		sendDataAndCheckOk(auxStr);

		if (waitForHttpsState(HTTPS_SESSION_OPENED, 5000))
		{
			sprintf(auxStr, "AT+CHTTPSSEND=%i", strlen(URL) + strlen(body));

			sendDataAndCheckPrompt(auxStr);

			if (NULL == body || strlen(body) == 0) 
			{
				sendDataAndCheckOk(URL);
			}
			else 
			{
				m_Module->print(URL);

				if (m_DebugOut)
				{
					m_DebugOut->print("\t---> ");
					m_DebugOut->println(URL);
				}

				sendDataAndCheckOk(body);
			}

			if (SIM_MODULE_GEN::SIM53XX == m_ModuleType)
			{
				sendDataAndCheckOk("AT+CHTTPSSEND");
			}

			uint16_t replyLen = waitForHttpReceive(10000);
			if (replyLen > 0)
			{
				sprintf(auxStr, "AT+CHTTPSRECV=%i", replyLen);
				Serial.println(auxStr);
				sendData(auxStr, 2000);
			}

			// Close HTTP/HTTPS session
			sendDataAndCheckOk("AT+CHTTPSCLSE");
		}

		//------
	}

	sendDataAndCheckOk("AT+CHTTPSSTOP");
	waitForHttpsState(0, 5000);
	
	if (closeGPRSNetwork())
	{
		m_DebugOut->println(F("GPRS Network closed after sending HTTP(S) request"));
	}

	return false;
}

void Sim5360::getNetworkInfo(void)
{
	sendDataAndCheckOk("AT+CPSI?");
}