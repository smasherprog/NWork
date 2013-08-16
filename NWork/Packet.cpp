#include "Packet.h"

NWork::Packet::Packet(char* data, unsigned int len, Commands t):Times_Sent(0){
	Append(data, len); 
	get_Header()->Command = t;
}
void NWork::Packet::Append(char* data, unsigned int len){
	unsigned int size=0;
	if(_Data.size()==0) size = sizeof(NWork::Header);
	size += _Data.size();
	_Data.resize(size + len);
	if(data != nullptr) memcpy(&_Data[size], data, len);
}
char* NWork::Packet::get_Data(){
	return (&_Data[0]) + sizeof(NWork::Header);
}
unsigned int NWork::Packet::get_Length(){
	return _Data.size() - sizeof(NWork::Header);
}
NWork::Header* NWork::Packet::get_Header(){
	return (NWork::Header*)(&_Data[0]);
}
