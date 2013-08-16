#ifndef NWORK_PACKET_H
#define NWORK_PACKET_H
#include "Common.h"
#include <vector>
#include <chrono>
#include <iostream>

namespace NWork{
	class Packet{
	public:
		Packet():Times_Sent(0){}//invalid state
		Packet(char* data, unsigned int len, Commands t);
		void Append(char* data, unsigned int len);
		char* get_Data();
		unsigned int get_Length();
		Header* get_Header();

		std::chrono::steady_clock::time_point Tag_Time;//if this is a received packet, this will be the time received. If a sending packet, this is the first time the packet was sent
		unsigned int Times_Sent;//number of times sent

	private:
		std::vector<char> _Data;
	};
};

inline std::ostream& operator<<(std::ostream& strem, NWork::Packet& obj){
	strem<<"Packetid:"<<(int)obj.get_Header()->SequenceNumber<<" Channel id:"<<(int)obj.get_Header()->ChannelID<<" Peerid:"<<(int)obj.get_Header()->PeerID<<" Commandid:"<<(int)obj.get_Header()->Command<<" Timesent:"<<obj.Times_Sent<<" Length:"<<obj.get_Length();
	return strem;
}



#endif