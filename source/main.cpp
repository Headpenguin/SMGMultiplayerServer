#include <cstdlib>
#include <cstdio>

#include "protocol.hpp"
#include "packets.hpp"
#include "packets/connect.hpp"
#include "packets/ack.hpp"
#include "packetFactory.hpp"
#include "transmission.hpp"
#include "players.hpp"

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
	5029;
#endif

static const char *bindInetAddrStr = 
#ifdef DEFAULT_ADDR
	STRINGIZE(DEFAULT_ADDR);
#else
	"0.0.0.0";
#endif

constexpr uint8_t maxNumPlayers =
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

Player::Player players[maxNumPlayers];

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

    bool full = false;
	
	while(!quit) {

		poll(&pfd, 1, 0);

		if(pfd.revents & POLLIN) quit = true;	

		pfd.events = POLLIN;

		NetReturn res;
		res = pp.readPacket(reader);
        bool fail = true;
		switch(res.errorCode) {
			case NetReturn::OK:
                fail = false;
				break;
			case NetReturn::CANDIDATE:
				pp.getPacketFactory().setCandidate();
                fail = false;
				break;
			case NetReturn::NOT_ENOUGH_SPACE:
				fprintf(stderr, "Warning: Not enough memory\n");
				break;
            case NetReturn::FILTERED:
                if(!full) {
                    full = true;
                    fprintf(stderr, "Warning: Attempt made to connect to full server\n");
                }
                break;
			default:
				fprintf(stderr, "Warning: invalid packet received\n");
				break;
		}

        if(!fail) {

            Packets::PacketFactory::PacketUnion pu;
            
            res = pp.processPacket(&pu);
            if(res.errorCode != NetReturn::OK) {
                fprintf(stderr, "Warning: invalid packet received (%u)\n", res.errorCode);
                pp.dropPacket();
                pp.finishProcessing();
                //continue;
            }
            else {
                switch(pu.tag) {
                case Packets::Tag::CONNECT: 
                {
                    NetReturn id = pp.getSenderId();
                    if(id.errorCode != NetReturn::OK) netHandleInvalidState();
                    if(!connectionHolder.addConnection(id.bytes)) {
                        fprintf(stderr, "Failed to add connection %d", id.bytes);
                    }
                    const Transmission::Connection *c 
                        = connectionHolder.getConnection(id.bytes);
                    const auto *ipAddr = reinterpret_cast<const uint8_t *>
                        (&c->addr.sin_addr.s_addr);
                    uint16_t port = ntohs(c->addr.sin_port);
                    fprintf(stderr, "Connected to %d.%d.%d.%d on port %d (%d)\n",
                        ipAddr[0],
                        ipAddr[1],
                        ipAddr[2],
                        ipAddr[3],
                        port,
                        id.bytes
                    );
                    pp.addPacket(Packets::ServerInitialResponse(
                        Protocol::MAJOR, Protocol::MINOR, id.bytes
                    ), 0x80 | id.bytes);
                    pp.dropPacket();
                    pp.finishProcessing();
                    break;
                }

                case Packets::Tag::ACK:
                case Packets::Tag::SERVER_INITIAL_RESPONSE:
                {
                    pp.dropPacket();
                    pp.finishProcessing();
                    break;
                }
                case Packets::Tag::PLAYER_POSITION:
                {
                    const Packets::PlayerPosition &pos = pu.playerPos;
                    NetReturn id = pp.getSenderId();
                    if(id.errorCode != NetReturn::OK) netHandleInvalidState();

                    if(id.bytes != pos.playerId) {
                        pp.dropPacket();
                        fprintf(stderr, "Client %d is impersonating %d\n", 
                            id.bytes, pu.playerPos.playerId);
                    }

                    players[pos.playerId].updateInfo(&pos.position, &pos.velocity, &pos.direction);

                    pp.finishProcessing();

                    break;
                }
                case Packets::Tag::MAX_TAG: // invalid state
                    break;
            }

        }}
        pp.getPacketFactory().resetCandidate();

		do {
			res = pp.sendPacket(writer);
		} while (res.errorCode == NetReturn::OK && res.bytes > 0);
		

	}

	close(fd);

	return 0;
}

