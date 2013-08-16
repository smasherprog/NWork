#ifndef NWORK_CHANNEL_H
#define NWORK_CHANNEL_H
#include <list>
#include "Packet.h"

namespace NWork{
	class Channel{
	public:
							Channel();
							~Channel();

		void				Process(char* data, int len);

		void				BuildAcks(unsigned char channelid, unsigned short int peerid);
			
		void				SendReliable(Packet& p);
		void				SendUnReliable(Packet& p);

		std::vector<Packet> get_UnreliableOut(unsigned int& bytesent, const unsigned int maxsent);
		std::vector<Packet*> get_ReliableOut(unsigned int& bytesent, const unsigned int maxsent);

		std::vector<NWork::Packet> get_Packets();
		bool				Buffer_Overrun(){ return (ReliableInOrdered.size() > MAX_FRAGMENTS + ACK_MAXPERMSG) || (UnReliableIn.size() > MAX_FRAGMENTS + ACK_MAXPERMSG); }
	

		unsigned int		Ping;
		unsigned int		PacketLoss;

		std::chrono::steady_clock::time_point	LastTimeReceivedGoodData, PacketLoss_Second_Counter;		
		
	private:

		unsigned short		_ReliableOutId; 
		unsigned short		_ReliableInId;
		unsigned short		_UnReliableOutId;
		unsigned short		_UnReliableInId;
		void				_AddPing(unsigned int ping);
		unsigned int		Pings[PING_RECORDLENGTH];
		unsigned int		Num_of_Pings;
		float				Num_Packets_Resent, Num_Packets_Sent;

		void				_ProcessIncomingReliable(char* data, int len);
		void				_ProcessIncomingUnReliable(char* data, int len);
		void				_ProcessIncomingACKs(char* data, int len);

		void				_ProcessAck(unsigned int packetid);	
		void				_Check_Reliable_Fragment();

		bool				BuildAckPacket;

		std::list<Packet>	ReliableOut;	
		std::list<Packet>	UnReliableOut;	
		std::list<Packet>	ReliableInOrdered;
		std::list<Packet>	ReliableInUnOrdered;
		std::list<Packet>	UnReliableIn;
	};
};

#endif