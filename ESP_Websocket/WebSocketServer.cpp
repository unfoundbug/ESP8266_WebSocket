#include "WebSocketServer.h"



WebSocketServer::WebSocketServer(int iPort) : m_wWifiInteraction(iPort), m_iPort(iPort)
{
	m_wWifiInteraction.begin();
}


WebSocketServer::~WebSocketServer()
{
}

void WebSocketServer::Process()
{
		if (m_cClient.connected())
		{
			switch (m_eClientState)
			{
			case (eAwaitingRequest) :
			{
				if (m_cClient.available())
					m_cClient.readBytes(m_cSocketDataBufffer, m_kiBufferSize);
				char* pcUpgradeSocket = strstr(m_cSocketDataBufffer, "Upgrade: websocket");


			}break;
			case (eSocketMode) :
			{
			}break;
			}
		}
		else
		{
			m_cClient = m_wWifiInteraction.available();
			m_eClientState = eAwaitingRequest;
		}
}
