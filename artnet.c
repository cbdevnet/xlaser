#include "artnet.h"
#include <sys/types.h>
#include <sys/socket.h>
#include "endian.h"

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

int artnet_output_handler(CONFIG* config, char* buf) {

	ArtDmxPacket* art = (ArtDmxPacket*) buf;

	art->length = be16toh(art->length);
	printf("sequence: %d\n", art->sequence);
	printf("physical: %d\n", art->physical);
	printf("subUni: %d\n", art->subUni);
	printf("net: %d\n", art->net);
	printf("data length: %d\n", art->length);
	/* print_dmx_output(art->data, art->length) */


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
