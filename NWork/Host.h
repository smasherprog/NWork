#ifndef NWORK_CLIENT_H
#define NWORK_CLIENT_H
#include <vector>
#include "Packet.h"
#include "Peer.h"
#include <functional>
#include <map>
#include <vector>
#include "Common.h"

namespace NWork{
	class UDPSocket;	
	namespace _Internals{
		struct Internal{
			std::shared_ptr<UDPSocket>	Socket;
			unsigned int			Max_ChannelCount;
			std::vector<Peer>		Peers;
			std::vector<unsigned short int> Peerids;
			std::map<unsigned int, std::vector<unsigned short>> Ips;
		};
	};
	class Host {
	public:

		bool		Open(char* pAddress, unsigned short port, unsigned short max_sendbandwidth=MINPEERBANDWIDTH, unsigned short max_receivebandwidth=MINPEERBANDWIDTH, unsigned char max_channelcount=MINCHANNELCOUNT, unsigned short int maxpeercount = MINPEERCOUNT);
		void		Send(Packet& packet,unsigned short int peerid, unsigned char channel);

		void		Flush();//this will send or resend(if needed) any pending data
		
		void		Close();
		void		Run();//this will call Flush, process any data and call the On_Disconnect, On_Connect, and On_Receive events

		std::function<void(const Peer*)>	On_Disconnect;
		std::function<void(const Peer*)>	On_Connect;
		std::function<void(const Peer*, std::vector<Packet>&)>	On_Receive;

		
		_Internals::Internal	Internal;
	private:
		

		bool			_Peer_Maped(unsigned int addr, unsigned short port);
		unsigned short	_GetNewID(unsigned int addr, unsigned short port);
		void			_MapPeer(unsigned int addr, unsigned short port);
		void			_Un_MapPeer(unsigned int addr, unsigned short port);
		unsigned int	_Max_SendBytes, _Max_ReceiveBytes, _Max_Channels;
		
	};

};
#endif