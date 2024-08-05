#include <cstdlib>

extern "C" {

#include <sys/socket.h>
#include <netinet/ip.h>

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
	alignUp((MAX_PACKET_SIZE + 20) * maxNumPlayers, Packets::PACKET_ALIGNMENT);
#endif

int main() {

	int err;

	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	
	if(fd < 0) goto fail;

	in_addr s_addr;
   	err = inet_aton(bindInetAddrStr, &s_addr);

	if(err < 0) {
		perror("(main) Invalid host IP address");
		goto failSocket;
	}

	sockaddr_in addr;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = s_addr;

	err = bind(fd, &addr, sizeof addr);

	if(err < 0) {
		perror("(main) Failed to bind to IP address");
		goto failSocket;
	}

	void *packetBuffer = aligned_alloc(Packets::PACKET_ALIGNMENT, packetBufferSize);

	if(packetBuffer == nullptr) {
		perror("(main) Not enough memory available on this system");
		goto failSocket;
	}

	auto *connectionBuffer = new(std::nothrow) Transmission::Connection[connectionBufferSize];

	if(connectionBuffer == nullptr) {
		fprint(stderr, "(main) Not enough memory available on this system\n");
		goto failSocket;
	}

	Packets::PacketFactory factory;
	PacketProcessor pp(packetBuffer, packetBufferSize, factory);

	Transmission::ConnectionHolder connectionHolder(connectionBuffer, connectionBufferSize);

	Transmission::Reader reader(fd, &connectionHolder);
	Transmission::Writer writer(fd, &connectionHolder);

	printf("Hit enter to close the server\n");

	bool quit = false;
	
	pollfd poll = {STDIN_FILENO, POLLIN, 0};
	
	while(!quit) {

		NetReturn res;
		res = pp.readPackets(reader);
		switch(res.errorCode) {
			case NetReturn::OK:
				break;
			case NetReturn::CANDIDATE:
				factory.setCandidate();
				break;
			case NetReturn::NOTENOUGHSPACE:
				fprintf(stderr, "Warning: Not enough memory\n");
				continue;
			default:
				fprintf(stderr, "Warning: invalid packet received");
				continue;
		}


		Packets::PacketUnion pu;
		
		res = pp.processPacket(&pu);
		if(res.errorCode != NetReturn::OK) {
			fprintf(stderr, "Warning: invalid packet received (%u)", res.errorCode);
			pp.dropPacket();
			pp.finishProcessing();
			continue;
		}
		switch(pu.tag) {
			case Packets::CONNECT:
				pp.addPacket(Packets::Ack());
				pp.dropPacket();
				pp.finishProcesssing();
				break;
			case Packet::ACK:
				pp.dropPacket();
				pp.finishProcessing();
				break;
		}

		do {
			res = pp.sendPackets(writer);
		} while (res.errorCode == NetReturn::OK && res.bytes > 0);
		

		poll(&poll, 1, 0);

		if(poll.revents & POLLIN) quit = true;	

		poll.events = POLLIN;

	}

failSocket:
	close(fd);

fail:
	return -1;
}

