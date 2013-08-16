#ifndef NWORK_PEER_H
#define NWORK_PEER_H
#include "Common.h"
#include "Packet.h"
#include <vector>
#include "Channel.h"
#include <functional>
#include <memory>

namespace NWork{
	class UDPSocket;
	class Peer{	
	public:	
		Peer();
		~Peer();

		void		Send(Packet& packet, unsigned char channel);
		void		Flush(UDPSocket* socket);

		void		Process(unsigned int addr, unsigned short port, char* data, int len);
		void		Run(std::function<void(const Peer*)>&	ondisconnect, std::function<void(const Peer*)>& onconnect,	std::function<void(const Peer*, std::vector<Packet>&)>&	onreceive);

		void		Close();

		void		Open(unsigned int addr, unsigned short destport, unsigned int id, unsigned short max_sendbandwidth=MINPEERBANDWIDTH, unsigned short max_receivebandwidth=MINPEERBANDWIDTH, unsigned char max_channelcount=MINCHANNELCOUNT);

		PeerState State;//!< The current state of the client as in PeerState
		union{
			unsigned int	Address;//!< this is the destination address where packets are sent to
			unsigned char Address_Parts[4];
		};
		unsigned short  DestinationPort;//!< this is the destination port where packets are sent to
		unsigned short	ReceivingId;//!< This is the ID that the THIS peer expects to receive
		unsigned short	SendingId;//!< This is the ID used by THIS peer to send out.

		unsigned int	Sendbandwidth;//!< This is the max bytes that will be sent per second to the peer
		unsigned int	Receivebandwidth;//!< This is the max bytes that will be received from the peer --ignored right now
		unsigned int	BytesSentPerSecond;//!< The current amount of data sent. It is reset to zero each time a second passes to regular bandwidth
		unsigned int	BytesReceivedPerSecond;//!< The current amount of data received. It is reset to zero each time a second passes to regular bandwidth

		unsigned int	ChannelCount;//!< Number of open channels on this connection
		unsigned int	IncomingDataTotal;//!< Total amount of data received by this peer
		unsigned int	OutgoingDataTotal;//!< Total amount of data sent by this peer

		std::chrono::steady_clock::time_point	NextTimeout;//!< This holds the time that is updated for each packet received from a peer, but a pad is added to it of TIMEOUT. This is used to calc timeouts
		std::chrono::steady_clock::time_point	ConnectTime;//!< This is the time that the peer initiated the connection attempt. It is useful to see how long a peer has been connected
		std::chrono::steady_clock::time_point   NextPingTime;//!< This is the next time we will ping the peer

		unsigned int	get_Ping(unsigned int channel);
		unsigned int	get_PacketLoss(unsigned int channel);//!< Current Amount of packet loss as a percent from 0 to 100

	protected:
		
		void _Connect(Packet& packet);
		void _Verify_Connect(Packet& packet);
		void _Disconnect();
		std::vector<Channel> _Channels;

		std::chrono::steady_clock::time_point _Send_Timer, _Receive_Timer;
	};
};
inline std::ostream& operator<<(std::ostream& strem, NWork::Peer& obj){
	strem<<"Peer ReceivingId:"<<obj.ReceivingId<<" SendingId:"<<obj.SendingId<<std::endl;
	strem<<"\tAddress: "<<(int)obj.Address_Parts[0]<<"."<<(int)obj.Address_Parts[1]<<"."<<(int)obj.Address_Parts[2]<<"."<<(int)obj.Address_Parts[3]<<" Port: "<<(int)obj.DestinationPort<<std::endl;
	strem<<"\tIncomingDataTotal: "<<obj.IncomingDataTotal<<" OutgoingDataTotal:"<<obj.OutgoingDataTotal<<" Ping (0):"<<obj.get_Ping(0)<<" PacketLoss (0): "<<obj.get_PacketLoss(0)<<std::endl;
		return strem;
}
#endif
