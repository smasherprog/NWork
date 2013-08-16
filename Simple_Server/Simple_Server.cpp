#include "stdafx.h"
#include "../NWork/Host.h"
#include <iostream>
#include <vector>
#include "../NWork/UDPSocket.h"
#include <string>


void onconnect(const NWork::Peer* p){
	std::cout<<"Connected to peer"<<p->ReceivingId<<std::endl;
}
void ondisconnect(const NWork::Peer* p){
	std::cout<<"Disconnected from peer"<<p->ReceivingId<<std::endl;
}
void onreceive(const NWork::Peer* p, std::vector<NWork::Packet>& packets){
	std::cout<<"Received "<<packets.size()<<" from peer "<<p->ReceivingId<<std::endl;
	for(auto& x: packets){
		std::cout<<x<<std::endl;
		if(x.get_Length()>0){
			std::string s(x.get_Data(), x.get_Length());
			std::cout<<s<<std::endl;
		}
	}
}


int _tmain(int argc, _TCHAR* argv[])
{
	std::cout<<"Starting up simple server..."<<std::endl;
	NWork::Host h;
	h.Open(nullptr, 12345, 5000, 5000, 1, 5);
	h.On_Connect = onconnect;
	h.On_Disconnect = ondisconnect;
	h.On_Receive = onreceive;
	auto timer = std::chrono::steady_clock::now();
	auto timer1 = std::chrono::steady_clock::now();
	std::cout<<"Starting loop "<<std::endl;
	while(true){
		if(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - timer1).count() > 4){
			for(auto& x: h.Internal.Peers){
				if(x.State!=NWork::PeerState::DISCONNECTED){//output stats for connected peers
					std::cout<<x<<std::endl;
				}
			}
			timer1 = std::chrono::steady_clock::now();
		}
		h.Run();
	}
	return 0;
}

