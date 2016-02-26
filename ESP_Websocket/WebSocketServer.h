#pragma once
#include <ESP8266WiFi/src/ESP8266WiFi.h>
#include <ESP8266WiFi/src/WiFiServer.h>
#include <ESP8266WiFi/src/WiFiClient.h>
#define WEBSOCKETBUFFERSIZE 1024
class WebSocketServer
{
public:
	WebSocketServer(int iPort);
	~WebSocketServer();
	void Process();
private:
	enum eClientState
	{ eAwaitingRequest, eSocketMode};
	WiFiServer m_wWifiInteraction;
	WiFiClient m_cClient;
	int m_iPort;
	eClientState m_eClientState;
	static const int m_kiBufferSize = 1024;
	char m_cSocketDataBufffer[m_kiBufferSize];
};

