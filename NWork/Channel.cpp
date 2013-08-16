#include "Channel.h"
#include <assert.h>
#include <string>

NWork::Channel::Channel(){
	_ReliableOutId=_ReliableInId=_UnReliableOutId=_UnReliableInId=Num_of_Pings=0;
	BuildAckPacket=false;
	Ping=PING_DEFAULT;
	Num_Packets_Resent=0.0f;
	PacketLoss=0;
	PacketLoss_Second_Counter = LastTimeReceivedGoodData = std::chrono::steady_clock::now();
}
NWork::Channel::~Channel(){

}
//Hack to construct packet
NWork::Packet GenPacket(char* data, int len){
	auto p = NWork::Packet(nullptr, len- sizeof(NWork::Header), NWork::Commands::CONNECT);
	memcpy(p.get_Data() - sizeof(NWork::Header), data, len);
	p.Tag_Time = std::chrono::steady_clock::now();
	return p;
}
void NWork::Channel::BuildAcks(unsigned char channelid, unsigned short int peerid){
	if(BuildAckPacket){

		unsigned short int highestorderedid = _ReliableInId;
		unsigned int beg = _ReliableInId;
		unsigned int end = _ReliableInId;
		if(!ReliableInUnOrdered.empty()) end = ReliableInUnOrdered.back().get_Header()->SequenceNumber;
		if(end-beg>ACK_MAXPERMSG) end = beg+ACK_MAXPERMSG;// cap the max
		unsigned int spaceneeded = (end-beg)/8;//in bytes
		if(spaceneeded*8 != end-beg) spaceneeded+=1;// this means there was a remainder of bytes so round up
		Packet p(nullptr, spaceneeded + sizeof(unsigned short int) +1, ACKNOWLEDGE);// add the last in ordered received id
		p.get_Header()->ChannelID = channelid;
		p.get_Header()->PeerID = peerid;
		unsigned char* ptr = (unsigned char*)p.get_Data();
		memcpy(ptr, &highestorderedid, sizeof(highestorderedid));
		ptr+=sizeof(highestorderedid);
		*ptr++ = (unsigned char)end-beg;
		memset(ptr, 0, spaceneeded);// set to zero
		unsigned char mask = 0x80;
		while(beg++<=end){
			if( mask == 0 ) {//advance to the next byte
				mask = 0x80;
				ptr++;
			}
			bool e = false;
			for(auto x: ReliableInUnOrdered) {//search for the packet
				if(x.get_Header()->SequenceNumber == beg) {
					e =true;
					break;
				}
			}
			if(e) *ptr |= mask;//found it so mark the bit
			mask >>=1;// shift mask down by one, leaving the bit zero meaning we do not have the packet
		}
		SendUnReliable(p);
	}
	BuildAckPacket=false;
}
void NWork::Channel::Process(char* data, int len){
	NWork::Header* header = (NWork::Header*)data;
	if(IS_RELIABLE(header->Command)) _ProcessIncomingReliable(data, len);
	else  _ProcessIncomingUnReliable(data, len);
}

void NWork::Channel::_ProcessIncomingReliable(char* data, int len){
	NWork::Header* header = (NWork::Header*)data;
	if( _ReliableInId > header->SequenceNumber ) return;// if the packet we received has already been processed, ignore it

	BuildAckPacket=true;
	if( _ReliableInId == header->SequenceNumber){
		// This packet is the next ordered packet. Add it to the ordered list and then move all unordered that can be moved to the ordered list.

		ReliableInOrdered.emplace_back(GenPacket(data, len));
		_ReliableInId++;
		_Check_Reliable_Fragment();
		while( !ReliableInUnOrdered.empty()){
			if(_ReliableInId != ReliableInUnOrdered.front().get_Header()->SequenceNumber ) break;
			ReliableInOrdered.push_back( std::move(ReliableInUnOrdered.front()) );
			ReliableInUnOrdered.pop_front();
			_ReliableInId++;
			_Check_Reliable_Fragment();
		}


	} else {  // ReliableInId > packet->Id
		// Out of order.  Sort into the list.
		std::list<Packet>::iterator iPacket;
		bool  bExists = false;
		for( iPacket = ReliableInUnOrdered.begin(); iPacket != ReliableInUnOrdered.end(); ++iPacket ) {
			if( iPacket->get_Header()->SequenceNumber == header->SequenceNumber ){// Already in list - get out now!
				bExists = true;
				break;
			}
			if( iPacket->get_Header()->SequenceNumber > header->SequenceNumber) break;
		}
		if( !bExists){// the packet is not in the list.. insert it
			// We've gone 1 past the spot where pPacket belongs.  Back up and insert.
			ReliableInUnOrdered.emplace( iPacket, GenPacket(data, len) );
		}
	}
	LastTimeReceivedGoodData = std::chrono::steady_clock::now();
}
void NWork::Channel::_Check_Reliable_Fragment(){
	if(ReliableInOrdered.back().get_Header()->Command==Commands::RELIABLE_FRAGMENT_END){
		auto beg = ReliableInOrdered.begin();
		auto channel = beg->get_Header()->ChannelID;
		auto peerid = beg->get_Header()->PeerID;
		for(; beg!= ReliableInOrdered.end(); beg++){
			if(beg->get_Header()->Command == Commands::RELIABLE_FRAGMENT) break;
		}
		auto end = beg;
		unsigned int size =0;
		for(; end!= ReliableInOrdered.end(); end++){
			size+= end->get_Length();
			if(end->get_Header()->Command == Commands::RELIABLE_FRAGMENT_END) break;
		}
		//[beg, end]
		std::vector<char> tempdata;
		tempdata.resize(size);
		auto copybeg= beg;
		end++;
		char* begcopydata = &tempdata[0];
		for(; beg != end;beg++){
			memcpy(begcopydata, beg->get_Data(), beg->get_Length());
			begcopydata+=beg->get_Length();
		}
		ReliableInOrdered.emplace_back(Packet(&tempdata[0], size, Commands::RELIABLE));
		ReliableInOrdered.erase(copybeg, --end);//erase the range
		ReliableInOrdered.back().get_Header()->SequenceNumber = _ReliableInId-1;
		ReliableInOrdered.back().get_Header()->ChannelID = channel;
		ReliableInOrdered.back().get_Header()->PeerID = peerid;
	}
}
void NWork::Channel::_ProcessIncomingUnReliable(char* data, int len){
	NWork::Header* header = (NWork::Header*)data;
	if(header->SequenceNumber < _UnReliableInId ) return;
	_UnReliableInId = header->SequenceNumber +1;
	if(header->Command == Commands::ACKNOWLEDGE) _ProcessIncomingACKs(data, len);
	else UnReliableIn.emplace_back(GenPacket(data, len));
	LastTimeReceivedGoodData = std::chrono::steady_clock::now();
}
void  NWork::Channel::_ProcessIncomingACKs(char* data, int len){
	// Get the number of ACKs in this message, not counting the base ACK.
	unsigned short int basePacketID=0;
	char* ptr = data + sizeof(NWork::Header);
	memcpy( &basePacketID, ptr, sizeof( unsigned short int ) );// Get the base packet ID, which indicates all the ordered packets received so far.
	ptr += sizeof( unsigned short int );
	unsigned char numAcks = *ptr++;// Get the number of additional ACKs.
	unsigned short int ackID = 0;
	if(ReliableOut.empty())  ackID= _ReliableOutId;
	else ackID= ReliableOut.front().get_Header()->SequenceNumber;

	while( ackID < basePacketID ){
		_ProcessAck( ackID );
		ackID++;
	}
	unsigned char mask = 0x80;
	while( ackID < basePacketID + numAcks ){
		if( mask == 0 ) {
			mask = 0x80;
			ptr++;
		}
		if( ( *ptr & mask ) != 0 )
			_ProcessAck( ackID );
		mask >>= 1;
		ackID++;
	}
	LastTimeReceivedGoodData = std::chrono::steady_clock::now();
}
void NWork::Channel::_AddPing(unsigned int ping){
	if(ping< MINPING) ping = MINPING;
	unsigned int pingindex = Num_of_Pings++;
	if(pingindex>=PING_RECORDLENGTH) {
		Num_of_Pings=0;//reset the index and calc the pings
		unsigned int p=0;
		for(unsigned int x : Pings) p+=x;
		if(p!=0) p = p/PING_RECORDLENGTH;
		Ping =p;
		pingindex=0;
	}
	Pings[pingindex]=ping;
}
void NWork::Channel::_ProcessAck(unsigned int packetid){
	for(auto beg =ReliableOut.begin(); beg != ReliableOut.end(); ++beg ){
		if(beg->get_Header()->SequenceNumber == packetid){
			_AddPing((unsigned int)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - beg->Tag_Time).count());
			ReliableOut.erase(beg);
			return;
		}
	}
}

void NWork::Channel::SendReliable(Packet& p){
	if(p.get_Length() > MTU){//must fragment packet
		assert(p.get_Length() < MAX_FRAGMENTS * MTU);//packet is TOOO large
		char* beg = p.get_Data();
		char* end =p.get_Length()+beg;

		while(beg<end){
			unsigned int bytesleft=end-beg;//remaining bytes to send total
			unsigned int bytestocopy = bytesleft >MTU ? MTU: bytesleft;
			ReliableOut.emplace_back(Packet(beg, bytestocopy, NWork::Commands::RELIABLE_FRAGMENT));//generate packet with enough space
			auto header = ReliableOut.back().get_Header();
			header->SequenceNumber = _ReliableOutId++;
			header->ChannelID= p.get_Header()->ChannelID;
			header->PeerID = p.get_Header()->PeerID;
			beg +=bytestocopy;
		}
		ReliableOut.back().get_Header()->Command = Commands::RELIABLE_FRAGMENT_END;
	} else {
		p.get_Header()->SequenceNumber = _ReliableOutId++;
		ReliableOut.emplace_back(std::move(p));
	}
}
void NWork::Channel::SendUnReliable(Packet& p){
	if(p.get_Length() > MTU){//must fragment packet
		assert(p.get_Length() < MAX_FRAGMENTS * MTU);//packet is TOOO large
		char* beg = p.get_Data();
		char* end =p.get_Length()+beg;

		while(beg<end){
			unsigned int bytesleft=end-beg;//remaining bytes to send total
			unsigned int bytestocopy = bytesleft >MTU ? MTU: bytesleft;
			ReliableOut.emplace_back(Packet(beg, bytestocopy, NWork::Commands::UNRELIABLE_FRAGMENT));//generate packet with enough space
			auto header = ReliableOut.back().get_Header();
			header->SequenceNumber = _UnReliableOutId++;
			header->ChannelID= p.get_Header()->ChannelID;
			header->PeerID = p.get_Header()->PeerID;
			beg +=bytestocopy;
		}
		UnReliableOut.back().get_Header()->Command = Commands::UNRELIABLE_FRAGMENT_END;
	} else p.get_Header()->SequenceNumber = _UnReliableOutId++;
	UnReliableOut.emplace_back(std::move(p));
}

std::vector<NWork::Packet> NWork::Channel::get_Packets(){
	std::vector<NWork::Packet> temp;
	while(!ReliableInOrdered.empty()) {
		if(ReliableInOrdered.front().get_Header()->Command == Commands::RELIABLE_FRAGMENT) break;
		temp.emplace_back(std::move(ReliableInOrdered.front()));
		ReliableInOrdered.pop_front();
	}
	while(!UnReliableIn.empty()) {
		if(ReliableInOrdered.front().get_Header()->Command == Commands::UNRELIABLE_FRAGMENT) break;
		temp.emplace_back(std::move(UnReliableIn.front()));
		UnReliableIn.pop_front();
	}
	return temp;
}
std::vector<NWork::Packet> NWork::Channel::get_UnreliableOut(unsigned int& bytesent, const unsigned int maxsent){
	std::vector<NWork::Packet> temp;
	for(auto& x: UnReliableOut) {
		if(bytesent + x.get_Length() + sizeof(Header) <maxsent){
			x.Times_Sent+=1;
			bytesent += x.get_Length() + sizeof(Header);
			temp.emplace_back(std::move(x));
		} else break;
	}
	UnReliableOut.clear();
	return temp;
}
std::vector<NWork::Packet*> NWork::Channel::get_ReliableOut(unsigned int& bytesent, const unsigned int maxsent){
	std::vector<NWork::Packet*> temp;
	temp.reserve(ReliableOut.size());
	for(auto& x: ReliableOut) {
		auto currtime = std::chrono::steady_clock::now();
		if(x.Times_Sent ==0) x.Tag_Time = currtime;
		else {// this packet has already been sent out once, dont resend unless enough time has passed
			unsigned int timesincelastsend= (unsigned int)std::chrono::duration_cast<std::chrono::milliseconds>(currtime - x.Tag_Time).count();
			if(timesincelastsend< x.Times_Sent*(Ping/2 + Ping/8)) continue;// skip this packet.. it isnt time to resend yet
		}		
		if(x.Times_Sent !=0) {
			Num_Packets_Resent +=1.0f;
		} else Num_Packets_Sent +=1.0f;
		if(bytesent + x.get_Length() + sizeof(Header) <maxsent){
			x.Times_Sent+=1;
			bytesent += x.get_Length() + sizeof(Header);
			temp.push_back(&x);
		} else break;

	}
	auto currtime_ = std::chrono::steady_clock::now();
	unsigned int dt= (unsigned int)std::chrono::duration_cast<std::chrono::milliseconds>(currtime_ - PacketLoss_Second_Counter).count();
	if(dt>PACKETLOSS_WINDOW){
		if(Num_Packets_Resent > 0.0f){
			PacketLoss = (unsigned int)((Num_Packets_Resent/(Num_Packets_Resent +Num_Packets_Sent))*100.0f);
		} else {
			PacketLoss=0;
		}
		PacketLoss_Second_Counter=currtime_;
		Num_Packets_Resent = Num_Packets_Sent =0.0f;
	}
	return temp;
}
