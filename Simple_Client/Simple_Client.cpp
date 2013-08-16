#include "stdafx.h"
#include "../NWork/Host.h"
#include <iostream>
#include <vector>

NWork::Host* h=nullptr;
char reallylongstring[]="BEG I find myself needing to synthesize a ridiculously long string (like, tens of megabytes long) in JavaScript. (This is to slow down a CSS selector-matching operation to the point where it takes a measurable amount of time.) but I'm wondering if there's a more efficient way - one that doesn't involve creating a tens-of-megabytes array first. What test text do you try and type into your web forms to check that they handle all the edge cases properly (especially Unicode and XSS style problems). Your idea of HTML-sensitive characters is a good start. I also like using characters that are kind of readable, but are still Unicode. When I was doing this kind of testing for tabblo.com, I used this string: END";

void onconnect(const NWork::Peer* p){
	std::cout<<"Connected to peer"<<p->SendingId<<std::endl;
	NWork::Packet pac(reallylongstring, sizeof(reallylongstring), NWork::Commands::RELIABLE);
	h->Send(pac, 0, 0);
}
void ondisconnect(const NWork::Peer* p){
	std::cout<<"Disconnected from peer"<<p->SendingId<<std::endl;
}
void onreceive(const NWork::Peer* p, std::vector<NWork::Packet>& packets){
	std::cout<<"Received "<<packets.size()<<" from peer "<<p->SendingId<<std::endl;
}


int _tmain(int argc, _TCHAR* argv[])
{
	std::cout<<"Starting up simple server..."<<std::endl;
	h = new NWork::Host();
	h->Open("127.0.0.1", 12345);
	h->On_Connect = onconnect;
	h->On_Disconnect = ondisconnect;
	h->On_Receive = onreceive;
	auto timer = std::chrono::steady_clock::now();
	std::cout<<"Starting loop ..."<<std::endl;

	while(true){
		h->Run();
	}
	return 0;
}

