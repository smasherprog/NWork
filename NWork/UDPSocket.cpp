#include "UDPSocket.h"
#include "Common.h"
#include <string>

NWork::UDPSocket::UDPSocket() :_Socket(0) {Close(); }
NWork::UDPSocket::~UDPSocket(){ Close();}

unsigned int NWork::UDPSocket::Open(char* pAddress, unsigned short& port){
	Close();
	if(port==0) return 0;
	OUTPUT_NETWORK_DEBUG_MSG(__FUNCTION__"  - Initializing Socket." );
	
#if PLATFORM == PLATFORM_WINDOWS
	WSAData wsaData;
	int     error = WSAStartup( MAKEWORD( 2, 2 ), &wsaData );
	if( error == SOCKET_ERROR ){
		error = WSAGetLastError();
		if( error == WSAVERNOTSUPPORTED ){
			OUTPUT_NETWORK_DEBUG_MSG(__FUNCTION__"  - WSAStartup() error.\nRequested v2.2, found only v,"<<LOBYTE( wsaData.wVersion )<<HIBYTE( wsaData.wVersion ) );
			WSACleanup();
		} else OUTPUT_NETWORK_DEBUG_MSG(__FUNCTION__"  - WSAStartup() error ");
		return 0;
	}
#endif

	//create send socket
	_Socket = socket( AF_INET, SOCK_DGRAM, 0 );
	if( _Socket == INVALID_SOCKET ){
		OUTPUT_NETWORK_DEBUG_MSG(__FUNCTION__" - socket() error ");
		Close();
		return 0;
	}
	port=htons(port);// convert to network byte order
	SOCKADDR_IN localAddr;
	memset( &localAddr, 0, sizeof( SOCKADDR_IN ) );
	localAddr.sin_family = AF_INET;	
	localAddr.sin_port = port;
	localAddr.sin_addr.s_addr =  INADDR_ANY;

	int result=0;
	unsigned long host=1;
	if(pAddress!=nullptr){
		hostent *hostEntry = gethostbyname (pAddress);
		if (hostEntry == NULL || hostEntry -> h_addrtype != AF_INET) {
			host = inet_addr (pAddress);
			if (host == INADDR_NONE){
				Close();
				return 0;
			}
			localAddr.sin_addr.s_addr = host;
		} else {
			host =localAddr.sin_addr.s_addr = *(unsigned int*)hostEntry -> h_addr_list[0]; // a client connects to an address
		}
		result = connect(_Socket, (sockaddr *)&localAddr, sizeof( SOCKADDR_IN ) );
		Connected=true;
	}
	else {
		result = bind( _Socket, (sockaddr *)&localAddr, sizeof( SOCKADDR_IN ) );
		Connected=false;
	}

	if( result == SOCKET_ERROR ){
		OUTPUT_NETWORK_DEBUG_MSG(__FUNCTION__" - Network error ");
		Close();
		return 0;
	}
	unsigned long int val= 1;  // Anything non-zero.
	#if PLATFORM == PLATFORM_MAC || PLATFORM == PLATFORM_UNIX
	
        if ( fcntl( handle, F_SETFL, O_NONBLOCK, val ) == -1 )
        {
            OUTPUT_NETWORK_DEBUG_MSG(__FUNCTION__" - ioctlsocket() error ");
            return false;
        }
	#elif PLATFORM == PLATFORM_WINDOWS
	
	result = ioctlsocket( _Socket, FIONBIO, &val );// set up a non blocking socket
	if( result ){
		OUTPUT_NETWORK_DEBUG_MSG(__FUNCTION__" - ioctlsocket() error ");
		Close();
		return 0;
	}
#endif
	DstAddr=host;
	DstPort=port;

	OUTPUT_NETWORK_DEBUG_MSG(__FUNCTION__" Initalization of Socket complete. ");
	return host;
}

bool NWork::UDPSocket::Send(unsigned int addr, unsigned short port, char* data, unsigned short len){

	int  result=0;

	if(Connected) result = send(_Socket, data, len, 0);
	else {	
		SOCKADDR_IN     remoteAddr;
		memset( &remoteAddr, 0, sizeof( SOCKADDR_IN ) );
		remoteAddr.sin_family = AF_INET;
		remoteAddr.sin_addr.s_addr = addr;
		remoteAddr.sin_port = port;
		result = sendto( _Socket, data, len, 0, (SOCKADDR *)&remoteAddr, sizeof( SOCKADDR ) );
	}
	if( result == SOCKET_ERROR ){
		OUTPUT_NETWORK_DEBUG_MSG(__FUNCSIG__"  - error " );
		return false;
	}
	Total_BytesSent+=len;
	Total_PacketsSent+=1;
	return true;
}



bool NWork::UDPSocket::Receive(unsigned int& addr, unsigned short& port, char* data, int& result){

	if(Connected) {
		result = recv(_Socket, data, MAXMTU, 0);
		addr=DstAddr;
		port= DstPort;
	}
	else {
		SOCKADDR_IN     fromAddr;
		int             fromLen=sizeof( SOCKADDR );
		result = recvfrom(_Socket, data, MAXMTU, 0, (SOCKADDR *)&fromAddr, &fromLen );	
		addr = fromAddr.sin_addr.s_addr;
		port = fromAddr.sin_port;
	}

	if( result == SOCKET_ERROR ){
		switch (WSAGetLastError ()) {// the following errors are common
		case WSAEWOULDBLOCK:
		case WSAECONNRESET:
			return false;
		};
		OUTPUT_NETWORK_DEBUG_MSG(__FUNCSIG__"  - error ");
		return false;
	} 	
	Total_BytesRecieved += result;
	Total_PacketsReceived+=1;
	return result!=0;
}
void NWork::UDPSocket::Close(){
	#if PLATFORM == PLATFORM_MAC || PLATFORM == PLATFORM_UNIX
	close( _Socket );
    #elif PLATFORM == PLATFORM_WINDOWS
    closesocket( _Socket );
    #endif
	_Socket=0;
	Connected=false;
	Total_BytesRecieved= Total_BytesSent= Total_PacketsSent= Total_PacketsReceived=0;

}