#include "Host.h"
#include "Common.h"
#include "Packet.h"
#include "UDPSocket.h"
#include "Peer.h"
#include <memory>
#include <ctime>
#include <iomanip>

bool NWork::Host::Open(char* pAddress, unsigned short port, unsigned short max_sendbandwidth, unsigned short max_receivebandwidth, unsigned char max_channelcount, unsigned short int maxpeercount ){
	if(pAddress != nullptr && maxpeercount!=1) return false;
	Internal.Socket = std::make_shared<UDPSocket>();
	unsigned int addr = Internal.Socket->Open(pAddress, port);
	if(addr!=0){
		Internal.Peers.resize(maxpeercount);
		unsigned short int peerid=0;
		for(auto& x:Internal.Peers) Internal.Peerids.push_back(peerid++);
	} else {
		Close();
		return false;
	}
	_Max_Channels=max_channelcount;
	_Max_SendBytes=max_sendbandwidth;
	_Max_ReceiveBytes=max_receivebandwidth;
	if(pAddress!=nullptr){//client connection.. send connect request
		unsigned short int id=0;
		Internal.Peers[0].Open(addr, port, id, max_sendbandwidth, max_receivebandwidth, max_channelcount);
		_MapPeer(addr, port);
		Packet p((char*)&id, sizeof(id), RELIABLE);
		p.get_Header()->Command = Commands::CONNECT;
		Send(p, id, 0);
	}
	return true;
}
void NWork::Host::Send(Packet& packet, unsigned short int peer, unsigned char channel){ 
	if(peer>=Internal.Peers.size()) return;
	packet.get_Header()->PeerID = peer;
	Internal.Peers[peer].Send(packet, channel); 
}
void NWork::Host::Flush(){ 
	auto s = Internal.Socket.get();
	for(auto& x: Internal.Peers) x.Flush(s);
}

void NWork::Host::Run(){
	Flush();//send out anything in the buffer
	char buffer[MAXMTU];
	int len=0;
	unsigned int addr=0;
	unsigned short int port=0;
	while(Internal.Socket->Receive(addr, port, buffer, len)) {
		if(len<sizeof(NWork::Header)) continue;
		auto header = ((NWork::Header*)buffer);
		if(!_Peer_Maped(addr, port)){// new peer
			if(header->Command!=Commands::CONNECT) continue;// invalid packet for a new connection
			auto newid = _GetNewID(addr, port);
			if(newid!=-1) _MapPeer(addr, port);
			else continue;//no more ids
			header->PeerID = newid;// write the new id
			OUTPUT_NETWORK_DEBUG_MSG("New client connecting. . .addr:"<<addr<<" and port"<<port<<" ID assigned: "<<header->PeerID);
			Internal.Peers[header->PeerID].Open(addr, port, header->PeerID, _Max_SendBytes, _Max_ReceiveBytes, _Max_Channels);
		}

		if(header->PeerID>=Internal.Peers.size()) continue;
		Internal.Peers[header->PeerID].Process(addr, port, buffer, len);

	}
	for(auto& p: Internal.Peers) {
		if(p.State== PeerState::DISCONNECTED) continue;// skip disconnected peers
		PeerState ps = p.State;
		p.Run(On_Disconnect, On_Connect, On_Receive);
		if(ps!=p.State && ps==PeerState::DISCONNECTED) {//peer went to disconnected state.. get the Id and place it into the pool
			_Un_MapPeer(p.Address, p.DestinationPort);
			Internal.Peerids.push_back((&p - &Internal.Peers[0]));
		}
	}
}
bool NWork::Host::_Peer_Maped(unsigned int addr, unsigned short port){
	auto f = Internal.Ips.find(addr);
	if(f==Internal.Ips.end()) return false;// new client
	for(auto& x: f->second){
		if(port == x) return true;
	}
	return false;//not mapped
}
unsigned short	NWork::Host::_GetNewID(unsigned int addr, unsigned short port){
	if(Internal.Peerids.empty()) return -1;// no more ids, or this is a client in which case it doesn't accept more connections
	auto newid = Internal.Peerids.back();
	Internal.Peerids.pop_back();
	return newid;
}
void NWork::Host::_MapPeer(unsigned int addr, unsigned short port){
	std::vector<unsigned short> ports;
	ports.push_back(port);
	auto f= Internal.Ips.find(addr);
	if (f!=Internal.Ips.end()){
		f->second.push_back(port);
	} else {
		Internal.Ips.insert(std::pair<unsigned int, std::vector<unsigned short>>(addr, ports));//insert the new 
	}
}
void NWork::Host::_Un_MapPeer(unsigned int addr, unsigned short port){
	auto f= Internal.Ips.find(addr);
	if (f!=Internal.Ips.end()){
		for(auto b = f->second.begin(); b != f->second.end(); b++) {
			if(*b == port){
				f->second.erase(b);
				if(f->second.empty()) Internal.Ips.erase(f);
			}
		}
	}
}
void NWork::Host::Close(){ 
	Internal.Peers.clear();
	Internal.Peerids.clear();
}