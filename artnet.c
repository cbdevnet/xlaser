#include "artnet.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "endian.h"

char* artnet_packet_type(uint16_t opcode){
	switch(opcode){
		case ART_OP_POLL:
			return "ArtPoll";
		case ART_OP_POLL_REPLY:
			return "ArtPollReply";
		case ART_OP_DIAG_DATA:
			return "ArtDiagData";
		case ART_OP_COMMAND:
			return "ArtCommand";
		case ART_OP_OUTPUT:
			return "ArtOutput";
		case ART_OP_NZS:
			return "ArtNZS";
		case ART_OP_SYNC:
			return "ArtSync";
		case ART_OP_ADDRESS:
			return "ArtAddress";
		case ART_OP_INPUT:
			return "ArtInput";
	}

	return "Unknown";
}

void artnet_packet_init(ArtNetPacket* pkt, ART_OPCODE op, size_t pkt_len) {
	memset(pkt, 0, pkt_len);
	pkt->opcode = op;
	pkt->protVerHi = ART_PROT_VER_HI;
	pkt->protVerLo = ART_PROT_VER_LO;

	memcpy(pkt->id, ART_ID, 8);
}

void print_dmx_output(uint8_t data[], int length) {
	int i = 0;
	for(i = 0; i < length; i++){
		if(i % ART_DEBUG_LINE_LEN == 0){
			if(i != 0){
				printf("\n");
			}
			printf("%d: ", i / ART_DEBUG_LINE_LEN);
		}
		printf(" %3d", data[i]);
	}
	printf("\n");
}

int get_node_address(CONFIG* config, uint8_t address[4]) {
	//TODO return node ipv4 address
	memset(address, 0, 4);
	return 0;
}

int get_mac_address(CONFIG* config, uint8_t mac[8]) {
	//TODO return node mac address
	memset(mac, 0, 8);
	return 0;
}

int artnet_artpoll_handler(CONFIG* config, uint8_t* buf, const struct sockaddr* src, socklen_t src_len) {
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

	memcpy(artReply.shortName, SHORTNAME, strlen(SHORTNAME));
	memcpy(artReply.longName, XLASER_VERSION, strlen(XLASER_VERSION));

	get_node_address(config, artReply.address);
	get_mac_address(config, artReply.mac);

	ssize_t len = sizeof(artReply);
	ssize_t bytes_sent = 0;


	printf("Sent %zd bytes\n", len);
	char* sendbuf = (char*) &artReply;
	while (len > 0) {
		bytes_sent = sendto(config->sockfd, sendbuf, len, 0, (struct sockaddr*) &bcast, bcast_len);
		if (bytes_sent < 0) {
			perror("art_poll");
			return -1;
		}

		sendbuf += bytes_sent;
		len -= bytes_sent;
	}

	return 0;
}

int artnet_output_handler(CONFIG* config, ArtNetPacket* packet) {
	ArtDmxPacket* dmx_packet = (ArtDmxPacket*)packet;

	dmx_packet->length = be16toh(dmx_packet->length);
	/*printf("sequence: %d\n", art->sequence);
	printf("physical: %d\n", art->physical);
	printf("subUni: %d\n", art->subUni);
	printf("net: %d\n", art->net);
	printf("data length: %d\n", art->length);*/
	/* print_dmx_output(art->data, art->length) */

	if (dmx_packet->net != config->art_net) {
		fprintf(stderr, "Data for another net, ignoring\n");
		return 0;
	}

	if (dmx_packet->subUni != config->art_subUni) {
		fprintf(stderr, "Data for another subuniverse, ignoring\n");
		return 0;
	}

	if (config->dmx_address + DMX_CHANNELS > dmx_packet->length) {
		fprintf(stderr, "Payload does not include data for local address\n");
		return -1;
	}

	memcpy(config->dmx_data, dmx_packet->data + config->dmx_address - 1, DMX_CHANNELS);
	print_dmx_output(config->dmx_data, DMX_CHANNELS);

	return 0;
}

int artnet_handler(CONFIG* config) {
	uint8_t data_buffer[ART_INTERNAL_READ_BUFFER];
	struct sockaddr_storage src_addr;
	socklen_t src_len = sizeof src_addr;
	ArtNetPacket* art_packet = NULL;

	ssize_t bytes_read = recvfrom(config->sockfd, &data_buffer, ART_INTERNAL_READ_BUFFER, 0, (struct sockaddr*)&src_addr, &src_len);

	if(bytes_read < 0){
		perror("recvfrom");
		return -1;
	} 
	else if(bytes_read < 8){
		fprintf(stderr, "Ignoring packet of length %zu\n", bytes_read);
		return 0;
	}

	art_packet = (ArtNetPacket*)data_buffer;
	if(memcmp(art_packet->id, ART_ID, 8)) {
		fprintf(stderr, "Header check failed, ignoring\n");
		return 0;
	}

	printf("Handling %s type data\n", artnet_packet_type(art_packet->opcode));
	//printf("protVerHi: %d\nprotVerLo: %d\n", art_packet->protVerHi, art_packet->protVerLo);
	switch(art_packet->opcode) {
		case ART_OP_POLL:
			artnet_artpoll_handler(config, data_buffer, (struct sockaddr*)&src_addr, src_len);
			break;
		case ART_OP_OUTPUT:
			artnet_output_handler(config, art_packet);
			break;
	}

	return 0;
}


