#include <cstdlib>
#include <cstdio>

#include "protocol.hpp"
#include "packets.hpp"
#include "packets/connect.hpp"
#include "packets/ack.hpp"
#include "packetFactory.hpp"
#include "transmission.hpp"

extern "C" {

#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>

}

#define STRINGIZE(a) #a

static uint16_t port = 
#ifdef DEFAULT_PORT
	DEFAULT_PORT;
#else
	5000;
#endif

static const char *bindInetAddrStr = 
#ifdef DEFAULT_ADDR
	STRINGIZE(DEFAULT_ADDR);
#else
	"0.0.0.0";
#endif

static uint8_t maxNumPlayers =
#ifdef MAX_NUM_PLAYERS
	MAX_NUM_PLAYERS;
#else
	8;
#endif

static uint8_t connectionBufferSize = 
#ifdef CONNECTION_BUFFER_SIZE
	CONNECTION_BUFFER_SIZE;
#else
	maxNumPlayers;
#endif

static size_t packetBufferSize =
#ifdef TRANSMISSION_BUFF_SIZE
	TRANSMISSION_BUFF_SIZE;
#else
	alignUp((Packets::MAX_PACKET_SIZE + 20) * maxNumPlayers, Packets::PACKET_ALIGNMENT);
#endif

int main() {

	int err;

	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	
	if(fd < 0) return -1;

	in_addr s_addr;
   	err = inet_aton(bindInetAddrStr, &s_addr);

	if(err < 0) {
		perror("(main) Invalid host IP address");
		close(fd);
		return -1;
	}

	sockaddr_in addr;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr = s_addr;

	err = bind(fd, reinterpret_cast<const sockaddr *>(&addr), sizeof addr);

	if(err < 0) {
		perror("(main) Failed to bind to IP address");
		close(fd);
		return -1;
	}

	void *packetBuffer = aligned_alloc(Packets::PACKET_ALIGNMENT, packetBufferSize);

	if(packetBuffer == nullptr) {
		perror("(main) Not enough memory available on this system");
		close(fd);
		return -1;
	}

	auto *connectionBuffer = new(std::nothrow) 
        Transmission::Connection[connectionBufferSize];

	if(connectionBuffer == nullptr) {
		fprintf(stderr, "(main) Not enough memory available on this system\n");
		close(fd);
		return -1;
	}

	Packets::PacketFactory factory;
	Protocol::PacketProcessor pp(packetBuffer, packetBufferSize, factory);

	Transmission::ConnectionHolder connectionHolder(connectionBuffer, connectionBufferSize);

	Transmission::Reader reader(fd, &connectionHolder);
	Transmission::Writer writer(fd, &connectionHolder);

	printf("Hit enter to close the server\n");

	bool quit = false;
	
	pollfd pfd = {STDIN_FILENO, POLLIN, 0};
	
	while(!quit) {

		NetReturn res;
		res = pp.readPacket(reader);
		switch(res.errorCode) {
			case NetReturn::OK:
				break;
			case NetReturn::CANDIDATE:
				factory.setCandidate();
				break;
			case NetReturn::NOT_ENOUGH_SPACE:
				fprintf(stderr, "Warning: Not enough memory\n");
				continue;
			default:
				fprintf(stderr, "Warning: invalid packet received");
				continue;
		}


		Packets::PacketFactory::PacketUnion pu;
		
		res = pp.processPacket(&pu);
		if(res.errorCode != NetReturn::OK) {
			fprintf(stderr, "Warning: invalid packet received (%u)", res.errorCode);
			pp.dropPacket();
			pp.finishProcessing();
			continue;
		}
		switch(pu.tag) {
			case Packets::Tag::CONNECT: 
			{
				NetReturn id = pp.getSenderId();
				if(id.errorCode != NetReturn::OK) netHandleInvalidState();
				connectionHolder.addConnection(id.bytes);
				pp.addPacket(Packets::Ack());
				pp.dropPacket();
				pp.finishProcessing();
				break;
			}

			case Packets::Tag::ACK:
			{
				pp.dropPacket();
				pp.finishProcessing();
				break;
			}
		}

		do {
			res = pp.sendPacket(writer);
		} while (res.errorCode == NetReturn::OK && res.bytes > 0);
		

		poll(&pfd, 1, 0);

		if(pfd.revents & POLLIN) quit = true;	

		pfd.events = POLLIN;

	}

	close(fd);

	return 0;
}

