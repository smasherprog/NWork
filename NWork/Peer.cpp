#include "Peer.h"
#include "UDPSocket.h"

NWork::Peer::Peer(){
	Close();
}
NWork::Peer::~Peer(){

}
void NWork::Peer::Open(unsigned int addr, unsigned short port, unsigned int receivingid, unsigned short max_sendbandwidth, unsigned short max_receivebandwidth, unsigned char max_channelcount){
	Close();
	Address = addr;
	DestinationPort = port;
	ReceivingId = receivingid;
	State = PeerState::CONNECTING;
	_Channels.resize(max_channelcount);
	ConnectTime= std::chrono::steady_clock::now();
	NextPingTime = ConnectTime + std::chrono::milliseconds(PINGRATE);
}

void NWork::Peer::Close(){
	State = PeerState::DISCONNECTED;
	Address=DestinationPort=SendingId=ReceivingId=0;
	Sendbandwidth=Receivebandwidth=MINPEERBANDWIDTH;
	BytesSentPerSecond=BytesReceivedPerSecond=0;
	ChannelCount=MINCHANNELCOUNT;
	IncomingDataTotal=OutgoingDataTotal=0;
	_Channels.clear();
	_Send_Timer = _Receive_Timer=std::chrono::steady_clock::now();
	Sendbandwidth = Receivebandwidth=MINPEERBANDWIDTH;
}
unsigned int NWork::Peer::get_Ping(unsigned int channel){
	if(channel>=ChannelCount) return 0;
	return _Channels[channel].Ping;
}
unsigned int NWork::Peer::get_PacketLoss(unsigned int channel){
	if(channel>=ChannelCount) return 0;
	return _Channels[channel].PacketLoss;
}

void NWork::Peer::Flush(UDPSocket* socket){		
	auto rtime = std::chrono::steady_clock::now();
	if(std::chrono::duration_cast<std::chrono::milliseconds>(rtime - _Send_Timer).count() >=1000){//reset counters each second
		_Send_Timer = rtime;
		OutgoingDataTotal +=BytesSentPerSecond;
		BytesSentPerSecond=0;
	} 
	for(auto& x: _Channels){	
		for(auto& y: x.get_ReliableOut(BytesSentPerSecond, Sendbandwidth)) socket->Send(Address, DestinationPort, y->get_Data() - sizeof(Header), y->get_Length() + sizeof(Header));
		for(auto& y: x.get_UnreliableOut(BytesSentPerSecond, Sendbandwidth)) socket->Send(Address, DestinationPort, y.get_Data() - sizeof(Header), y.get_Length() + sizeof(Header));	
	}
}

void NWork::Peer::Send(Packet& packet, unsigned char channel){
	if(channel>=_Channels.size()) return;
	packet.get_Header()->PeerID = SendingId;
	packet.get_Header()->ChannelID = channel;
	if(IS_RELIABLE(packet.get_Header()->Command)) _Channels[channel].SendReliable(packet);
	else _Channels[channel].SendUnReliable(packet);
}

void NWork::Peer::Process(unsigned int addr, unsigned short port, char* buffer, int len){
	if(State == PeerState::DISCONNECTED || len<sizeof(NWork::Header) || DestinationPort!= port || Address != addr) return;// skip, bad info
	auto header = ((NWork::Header*)buffer);
	unsigned int channel = (unsigned int)header->ChannelID;
	if(channel>=_Channels.size()) return;

	auto rtime = std::chrono::steady_clock::now();
	if(std::chrono::duration_cast<std::chrono::milliseconds>(rtime - _Receive_Timer).count()>=1000){//reset counters each second
		_Receive_Timer = rtime;
		IncomingDataTotal += BytesReceivedPerSecond;
		BytesReceivedPerSecond=0;
	} 
	if(BytesReceivedPerSecond + len> Receivebandwidth) return;//drop packet
	else BytesReceivedPerSecond += len;
	_Channels[channel].Process(buffer, len);

}
void NWork::Peer::Run(std::function<void(const Peer*)>&	ondisconnect, std::function<void(const Peer*)>& onconnect,	std::function<void(const Peer*, std::vector<Packet>&)>&	onreceive){
	std::vector<NWork::Packet> temp;
	unsigned char channelid=0;
	auto highest= ConnectTime;//use this as time
	bool bufferoverrun = false;
	for(auto& x : _Channels){
		bufferoverrun = x.Buffer_Overrun();
		if(bufferoverrun)break;
		x.BuildAcks(channelid++, SendingId);
		if(x.LastTimeReceivedGoodData>highest) highest = x.LastTimeReceivedGoodData;
		for(auto& y : x.get_Packets()){
			int k =0;
			switch(y.get_Header()->Command){
			case(CONNECT):
				_Connect(y);
				onconnect(this);
				break;
			case(VERIFY_CONNECT):
				_Verify_Connect(y);
				onconnect(this);
				break;
			case(DISCONNECT):
				ondisconnect(this);
				_Disconnect();
				break;
			case(RELIABLE):
			case(UNRELIABLE):
				temp.emplace_back(std::move(y));
				break;
			default:
				break;
			};
		}
	}
	if(bufferoverrun){
		ondisconnect(this);
		_Disconnect();
		OUTPUT_NETWORK_DEBUG_MSG("Peer "<<SendingId<<" overran the receiving buffer");
		return;
	}
	if(!temp.empty()) onreceive(this, temp);//allow processing of any packets
	//check for timeout
	auto currtime = std::chrono::steady_clock::now();
	if(std::chrono::duration_cast<std::chrono::milliseconds>(currtime - highest).count()> TIMEOUT){
		ondisconnect(this);
		_Disconnect();
		OUTPUT_NETWORK_DEBUG_MSG("Peer "<<SendingId<<" Timed out");
	}
	if(currtime > NextPingTime && State == PeerState::CONNECTED){//time to send a ping
		Packet p(nullptr, 0, Commands::PING);
		Send(p, 0);
		NextPingTime = currtime + std::chrono::milliseconds(PINGRATE);
	}

}

void NWork::Peer::_Connect(Packet& packet){
	if(State == PeerState::CONNECTING && packet.get_Header()->SequenceNumber==0 && packet.get_Header()->ChannelID == 0 && packet.get_Length() == sizeof(unsigned short int)){
		Packet p((char*)&ReceivingId, sizeof(ReceivingId), Commands::VERIFY_CONNECT);
		SendingId = *((unsigned short int*)packet.get_Data());
		p.get_Header()->PeerID = SendingId;
		_Channels[0].SendReliable(p);
		State = PeerState::CONNECTED;
		ConnectTime= std::chrono::steady_clock::now();
		NextPingTime = ConnectTime + std::chrono::milliseconds(PINGRATE);
		OUTPUT_NETWORK_DEBUG_MSG("Connect Packet Received");
	}else OUTPUT_NETWORK_DEBUG_MSG(__FUNCTION__"  - error: Erroneous packet received! ");
}
void NWork::Peer::_Verify_Connect(Packet& packet){
	if(State == PeerState::CONNECTING && packet.get_Header()->SequenceNumber==0 && packet.get_Header()->ChannelID == 0  && packet.get_Length() == sizeof(unsigned short int)){
		SendingId = *((unsigned short int*)packet.get_Data());
		State = PeerState::CONNECTED;
		OUTPUT_NETWORK_DEBUG_MSG("Verify Connect Packet Received");
	}else OUTPUT_NETWORK_DEBUG_MSG(__FUNCTION__"  - error: Erroneous packet received! ");
}

void NWork::Peer::_Disconnect(){
	Close();
}
