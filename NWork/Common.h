#ifndef NWORK_COMMON_H
#define NWORK_COMMON_H
#include <iostream>

#define PLATFORM_WINDOWS  1
#define PLATFORM_MAC      2
#define PLATFORM_UNIX     3

#if defined(_WIN32)
#define PLATFORM PLATFORM_WINDOWS
#elif defined(__APPLE__)
#define PLATFORM PLATFORM_MAC
#else
#define PLATFORM PLATFORM_UNIX
#endif

#if PLATFORM == PLATFORM_WINDOWS

#pragma comment(lib, "Ws2_32")
#define WINDOWS_LEAN_AND_MEAN
#include <winsock2.h>


#elif PLATFORM == PLATFORM_MAC || PLATFORM == PLATFORM_UNIX

#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#endif


namespace NWork{

#if defined(_DEBUG) 
#ifndef OUTPUT_NETWORK_DEBUG_MSG // nothing special about this, I use the console for standard debug info
#define OUTPUT_NETWORK_DEBUG_MSG(x) {												\
	std::cout<<x<<std::endl;													     	\
}
#endif
#else
#ifndef OUTPUT_NETWORK_DEBUG_MSG
#define OUTPUT_NETWORK_DEBUG_MSG(x) {}
#endif
#endif 

#define MINCHANNELCOUNT			1 //!< Min number of channels allowed
#define MINPEERCOUNT			1//!< Min number of peers allowed
#define MAXCHANNELCOUNT			31 //!< Max number of channels +1. This is includes 0, so 31 is 32. Channels [0, MAXCHANNELCOUNT]
#define MINPEERBANDWIDTH		5000 //!< The minimum Per Peer bandwidth 
#define UNLIMITEDBANDWIDTH		0xffff //!< Unlimited bandwidth!!
#define MTU						200 //!< this is a good value to use as a default. Check out http://en.wikipedia.org/wiki/Maximum_transmission_unit for more information 
#define MAXPEERCOUNT			4096   //!< the maximum number of peers allowed. This is the highest ID number allowed as well
#define INVALIDPEERID			MAXPEERCOUNT -1
#define MAXMTU					4096   //!< most Ethernets only allowed a 1500 size frame (or packet), but I figured, what the heck the IP layer will fragment the packet if it is higher than the Link Layers MTU
#define MAX_FRAGMENTS			60000//!< The maximum number of fragments that a packet can be broke into. This means the largest single packet is 60000*MTU bytes large

#define ACK_MAXPERMSG           256 //!< Max number of ack messages allowed per Ack packet. This should work fine in most cases

#define MINPING					50 //<! if a ping is too low, it can cause instability with resending packets. 
#define PING_RECORDLENGTH       32  //!< the window, or number of reliable packets used to calculate the ping and packet loss. Lager number means more packets are taken into account to calculate the ping, too long doesnt give accurate results
#define PING_DEFAULT			1000  //!<Time in milliseconds that is setup before a connection is established for a default ping value. If a lower number is used, the client or server will hammer the other peer trying to connect. This is basically the default ping until we send and receive enough packets to calculate one
#define TIMEOUT					10000 //!<timeout in milliseconds before a client or server will terminate a connection with a peer after not hearing from them.
#define PINGRATE				500   //!< ping users every 500 ms, which is every half a second
#define PACKETLOSSDISCONNECT   .40f //!< this is the percent a users packet loss can goto before you disconnect from them. Users with high packet loss can be a stress on a connection because of the resending
#define PACKETLOSS_WINDOW       5000  //!< the window, in ms used to calculate the packet loss.

	enum PeerState{
		DISCONNECTED,              
		CONNECTING,               
		ACKNOWLEDGING_CONNECT,       
		CONNECTED,                   
		ACKNOWLEDGING_DISCONNECT,    
		ENCRYPTION_NOT_READY,       
		EXCHANGING_KEYS,           
		ENCRYPTION_READY,           
	};
	enum Commands {		    
	
		ACKNOWLEDGE,
		DISCONNECT,  
		UNRELIABLE_FRAGMENT,
		UNRELIABLE_FRAGMENT_END,
		ENCRYPTED_UNRELIABLE_PACKET,
		UNRELIABLE,   

		CONNECT,
		VERIFY_CONNECT,
		PING,   
		RELIABLE_FRAGMENT,
		RELIABLE_FRAGMENT_END,
		INIT_ENCRYPTION,
		ENCRYPTED_RELIABLE_PACKET,
		RELIABLE,          
		NUMOFCOMMANDS
	};

#define IS_RELIABLE(x) (x >= CONNECT) 


#pragma pack(push, 1)// this ensures that all of the structures below are aligned to within 1 byte (packed tight). 

	class Header{
	public:
		Header() :SequenceNumber(0),  Command(0), ChannelID(0), PeerID(0){}
		unsigned short SequenceNumber;
		unsigned char Command, ChannelID;
		unsigned short PeerID;
	};
#pragma pack(pop)

};
#endif