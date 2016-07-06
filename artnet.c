#include "artnet.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "endian.h"

void *get_in_addr(const struct sockaddr *sa) {
  return sa->sa_family == AF_INET
      ? (void *) &(((struct sockaddr_in*)sa)->sin_addr)
      : (void *) &(((struct sockaddr_in6*)sa)->sin6_addr);
}

ArtNetPacket create_artnet_packet(ART_OPCODE op) {

	ArtNetPacket art = {
		.opcode = op,
		.protVerHi = PROT_VER_HI,
		.protVerLo = PROT_VER_LO
	};

	memcpy(art.id, ART_ID, 8);

	return art;
}

void print_dmx_output(uint8_t data[], int length) {
	int i;
	int j = 0;
	int k = 0;

	printf("%2d: ", k);

	for (i = 0; i < length; i++) {
		if (j < 10) {
			printf(" %3d", data[i]);
			j++;
		} else {
			j = 1;
			k++;
			printf("\n%2d: %3d", k, data[i]);
		}
	}
	printf("\n");
}

int get_node_address(CONFIG* config, uint8_t address[4]) {
	//TODO get ip address
	memset(address, 0, 4);

	return 0;
}

int get_mac_address(CONFIG* config, uint8_t mac[8]) {
	//TODO get mac address
	memset(mac, 0, 8);

	return 0;
}

int artnet_artpoll_handler(CONFIG* config, char* buf, const struct sockaddr* src, socklen_t src_len) {
	char addr[INET6_ADDRSTRLEN];
	//ArtPollPacket* artPoll = (ArtPollPacket*) buf;
	printf("Got packet from: %s\n",
			inet_ntop(src->sa_family, get_in_addr(src), addr, sizeof(addr)));

	struct sockaddr_in bcast  = {
		.sin_family = AF_INET,
		.sin_port = htons(ART_NET_PORT),
		.sin_addr = {
			.s_addr = htonl(INADDR_BROADCAST)
		}
	};
	socklen_t bcast_len = sizeof(bcast);

	memset(bcast.sin_zero, 0, sizeof(bcast.sin_zero));

	ArtPollReplyPacket artReply = {
		.opcode = ART_OP_POLL_REPLY,
		.port = ART_NET_PORT,
		.versInfoH = 0,
		.versInfoL = 1,
		.oemHi = 0xFF,
		.oem = 0xFF,
		.netSwitch = config->art_net,
		.subSwitch = config->art_subUni,
		.ubeaVersion = 0,
		.status1 = STATUS1_3,
		.estaManLo = 0x00,
		.estaManHi = 0x00,
		.numPortsHi = 0x00,
		.numPortsLo = 0x01,
		.portTypes = {
			0x85,
			0x00,
			0x00,
			0x00
		},
		.goodInput = {
			0x00,
			0x00,
			0x00,
			0x00
		},
		.goodOutput = {
			0x82,
			0x00,
			0x00,
			0x00
		},
		.swIn = {
			0x00, 0x00, 0x00, 0x00
		},
		.swOut = {
			config->art_universe, 0x00, 0x00, 0x00
		},
		.swVideo = 0,
		.swMacro = 0,
		.swRemote = 0,
		.spare1 = 0,
		.spare2 = 0,
		.spare3 = 0,
		.style = ST_NODE,
		.bindIp = {0, 0, 0, 0},
		.bindIndex = 0,
		.status2 = 0x00
	};
	memcpy(artReply.id, ART_ID, 8);
	memset(artReply.nodeReport, 0, 64);
	memset(artReply.longName, 0, 64);
	memset(artReply.shortName, 0, 18);
	memset(artReply.filler, 0, 26);

	memcpy(artReply.shortName, "XLaser", 6);
	memcpy(artReply.longName, "XLaser - Whatever", 17);

	get_node_address(config, artReply.address);
	get_mac_address(config, artReply.mac);

	ssize_t len = sizeof(artReply);
	ssize_t sended = 0;


	printf("send %d bytes.\n", (int) len);
	char* sendbuf = (char*) &artReply;
	while (len > 0) {
		sended = sendto(config->sockfd, sendbuf, len, 0, (struct sockaddr*) &bcast, bcast_len);
		if (sended < 0) {
			perror("art_poll");
			return -1;
		}

		sendbuf += sended;
		len -= sended;
	}

	return 0;
}

int artnet_output_handler(CONFIG* config, char* buf) {

	ArtDmxPacket* art = (ArtDmxPacket*) buf;

	art->length = be16toh(art->length);
	printf("sequence: %d\n", art->sequence);
	printf("physical: %d\n", art->physical);
	printf("subUni: %d\n", art->subUni);
	printf("net: %d\n", art->net);
	printf("data length: %d\n", art->length);
	/* print_dmx_output(art->data, art->length) */


	if (art->net != config->art_net) {
		printf("not my net.\n");
		return 0;
	}

	if (art->subUni != config->art_subUni) {
		printf("not my subUniverse.\n");
		return 0;
	}

	if (config->dmx_address + DMX_CHANNELS > art->length) {
		fprintf(stderr, "dmx_address is too high for received data\n");
		return -1;
	}
	printf("dmx_address: %d\n", config->dmx_address);

	if (config->dmx_address == 0) {
		config->dmx_address = 1;
	}

	memcpy(config->dmx_channels, art->data + config->dmx_address - 1, DMX_CHANNELS);

	print_dmx_output(config->dmx_channels, DMX_CHANNELS);


	return 0;
}

int artnet_handler(CONFIG* config) {

	char buf[1024];

	struct sockaddr src;
	socklen_t srclen = sizeof src;

	ssize_t bytes = recvfrom(config->sockfd, &buf, 1024, 0, &src, &srclen);

	if (bytes < 0) {
		fprintf(stderr, "Error in recvfrom.\n");
		return -1;
	} else if (bytes < 8) {
		fprintf(stderr, "Packet too small (%zu bytes). Ignore.\n", bytes);
	}

	ArtNetPacket* art = (ArtNetPacket*) &buf;

	if (memcmp(art->id, ART_ID, 8)) {
		fprintf(stderr, "Not an artnet packet. Ignore.\n");
		return 0;
	}

	printf("Found artnet package :)\n");
	printf("protVerHi: %d\nprotVerLo: %d\n", art->protVerHi, art->protVerLo);

	char* opcode_str;
	switch(art->opcode) {
		case ART_OP_POLL:
			opcode_str = "ArtNetPoll";
			artnet_artpoll_handler(config, (char*) &buf, &src, srclen);
			break;
		case ART_OP_POLL_REPLY:
			opcode_str = "ArtNetPollReply";
			break;
		case ART_OP_DIAG_DATA:
			opcode_str = "ArtNetDiagData";
			break;
		case ART_OP_COMMAND:
			opcode_str = "ArtNetCommand";
			break;
		case ART_OP_OUTPUT:
			opcode_str = "ArtNetOutput";
			artnet_output_handler(config, (char*) &buf);
			break;
		case ART_OP_NZS:
			opcode_str = "ArtNetNZS";
			break;
		case ART_OP_SYNC:
			opcode_str = "ArtNetSync";
			break;
		case ART_OP_ADDRESS:
			opcode_str = "ArtNetAddress";
			break;
		case ART_OP_INPUT:
			opcode_str = "ArtNetInput";
			break;
		default:
			opcode_str = "Unkown";
			break;
	}
	printf("It's a %s packet.\n", opcode_str);

	return 0;
}
