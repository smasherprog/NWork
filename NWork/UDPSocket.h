#ifndef NWORK_SOCKET_H
#define NWORK_SOCKET_H
#include <chrono>
#include "Common.h"

namespace NWork{
	class UDPSocket{
	public:

		UDPSocket();
		~UDPSocket();

		unsigned int	Open(char* pAddress, unsigned short& port);
		bool			Send(unsigned int addr, unsigned short port, char* data, unsigned short len);
		bool			Receive(unsigned int& addr, unsigned short& port, char* data, int& len);
		void			Close();

		bool			Connected;
		SOCKET			_Socket;

		unsigned int    Total_BytesRecieved,
			Total_BytesSent,
			Total_PacketsSent, 
			Total_PacketsReceived;

		unsigned int DstAddr;
		unsigned short int DstPort;

	};
};

#endif		



