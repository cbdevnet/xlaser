#pragma once

#define ART_DEBUG_LINE_LEN 8
#define ART_INTERNAL_READ_BUFFER 1024

typedef enum /*_ART_OPCODE*/ {
	ART_OP_POLL = 0x2000, /* This is an ArtPoll Packet, no other data is contained in this UDP packet.*/
	ART_OP_POLL_REPLY = 0x2100, /* This is an ArtPollReply Packet. It contains device status information.*/
	ART_OP_DIAG_DATA = 0x2300, /* Diagnostics and data logging packet.*/
	ART_OP_COMMAND = 0x2400, /* Used to send text based parameter commands.*/
	ART_OP_OUTPUT = 0x5000, /* This is an ArtDMX data packet. It contains zero start code DMX512 information for a single universe.*/
	ART_OP_NZS = 0x5100, /* This is an ArtNzs data packet. It contains non-zero start code (except RDM) DMX512 information for a single universe.*/
	ART_OP_SYNC = 0x5200, /* This is an ArtSync data packet. It is used to force synchronous transfer of ArtDMX packets to a node's output.*/
	ART_OP_ADDRESS = 0x600, /* This is an ArtAddress packet. It contains remote programming information for a Node.*/
	ART_OP_INPUT = 0x7000 /* This is an ArtInput packet. It contains enable - disable data for DMX inputs.*/
} ART_OPCODE;

const uint16_t ART_NET_PORT = 0x1936; /* 6454*/
const uint8_t ART_ID[] = {'A', 'r', 't', '-', 'N', 'e', 't', 0x00};
const uint8_t ART_PROT_VER_HI = 0;
const uint8_t ART_PROT_VER_LO = 14;

/* Talk to me bit masks (page 14) */
typedef enum /*_TALK_TO_ME*/ {
	TALK_TO_ME_ZERO = 0x01, /* deprecated. */
	TALK_TO_ME_ONLY = 0x02, /* 1 = Send ArtPollReply whenever Node conditions change.*/
	TALK_TO_ME_DIAG = 0x04, /* 1 = Send me diagnostics messages.*/
	TALK_TO_ME_DIAG_BROADCAST = 0x08, /* 1 = Diagnostics messages are unicast (if bit 2) */
	TALK_TO_ME_VLC = 0x10, /* 1 = Disable VLC transmission.*/
	TALK_TO_ME_75 = 0xD0 /* Unused, transmit as zero, do not test upon receipt.*/
} ART_TALK_TO_ME;

typedef enum /*_NODE_REPORT_CODES*/ {
	RC_DEBUG = 0x0000, /* Booted in debug mode (Only used in development */
	RC_POWER_OK = 0x0001, /* Power On Tests successful */
	RC_POWER_FAIL = 0x0002, /* Hardware tests fail at Power On */
	RC_SOCKET_WR1 = 0x0003, /* Last UDP from Node failed due to truncated length, Most likely caused by a collition. */
	RC_PARSE_FAIL = 0x0004, /* Unable to identify last UDP transmission. Check OpCode and packet length.*/
	RC_UDP_FAIL = 0x0005, /* Unable to open UDP Socket in last transmission attempt. */
	RC_SH_NAME_OK = 0x0006, /* Confirms that Short Name programming via ArtAddress, was successful.*/
	RC_LO_NAME_OK = 0x0007, /* Confirms that Long Name programming via ArtAddress, was successful. */
	RC_DMX_ERROR = 0x0008, /* DMX512 receive errors detected. */
	RC_DMX_UDP_FULL = 0x0009, /* Ran out of internal DMX transmit buffers.*/
	RC_DMX_RX_FULL = 0x000A, /* Ran out of internal DMX Rx buffers.*/
	RC_SWITCH_ERR = 0x000B, /* Rx Universe switches conflict.*/
	RC_CONFIG_ERR = 0x000C, /* Product configuration does not match firmware.*/
	RC_DMX_SHORT = 0x000D, /* DMX output short detected. See GoodOutput field.*/
	RC_FIRMWARE_FAIL = 0x000E, /* Last attempt to upload new firmware failed. */
	RC_USER_FAIL = 0x000F /* User changed switch settings when address locked by remote programming. User changes ignored.*/
} ART_NODE_REPORT_CODES;

typedef enum /*_ART_STATUS1*/ {
	STATUS1_UBEA = 0x01, /* 1 = UBEA present */
	STATUS1_RDM = 0x02, /* 1 = Capable of Remote Device Management.*/
	STATUS1_ROM = 0x04, /* 1 = Booted from ROM.*/
	STATUS1_3 = 0x08, /* Not implemented, transmit as zero, receivers do not test.*/
	STATUS1_PAPA = 0x30, /* Port Address Programming Authority. (page 20.)*/
	STATUS1_INDI = 0xC0 /* Indicator state. (page 20).*/
} ART_STATUS1;

typedef enum /*_ART_STATUS2*/ { /* see page 24*/
	STATUS2_WEB = 0x01, /* Set = Product supports web browser configuration.*/
	STATUS2_DHCP_CONF = 0x02, /*CLR = Node's IP is manually configured.*/
	STATUS2_DHCP_CAP = 0x04, /* CLR = Node is not DHCP capable.*/
	STATUS2_ADDRESS = 0x08, /* CLR = Node supports 8bit Port-Address (Art-Net II).*/
} ART_STATUS2;

typedef enum /*_ART_STYLE_CODES*/ {
	ST_NODE = 0x00, /* A DMX to / from Art-Net device.*/
	ST_CONTROLLER = 0x01, /* A lighting console.*/
	ST_MEDIA = 0x02, /* A Media Server.*/
	ST_ROUTE = 0x03, /* A network routing device.*/
	ST_BACKUP = 0x04, /* A backup device.*/
	ST_CONFIG = 0x05, /* A configuration or diagnostic tool.*/
	ST_VISUAL = 0x06 /* A visualiser. */
} ART_STYLE_CODES;

typedef enum /*_ART_ADDRESS_COMMANDS*/ { /* see page 32*/
	AC_NONE = 0x00,
	AC_CANCEL_MERGE = 0x01,
	AC_LED_NORMAL = 0x02,
	AC_LED_MUTE = 0x03,
	AC_LED_LOCATE = 0x04,
	AC_RESET_RX_FLAGS = 0x05,
	AC_MERGE_LTP0 = 0x10,
	AC_MERGE_LTP1 = 0x11,
	AC_MERGE_LTP2 = 0x12,
	AC_MERGE_LTP3 = 0x13,
	AC_MERGE_HTP0 = 0x50,
	AC_MERGE_HTP1 = 0x51,
	AC_MERGE_HTP2 = 0x52,
	AC_MERGE_HTP3 = 0x53,
	AC_CLEAR_OP1 = 0x91,
	AC_CLEAR_OP2 = 0x92,
	AC_CLEAR_OP3 = 0x93
} ART_ADDRESS_COMMANDS;

typedef enum /*_ART_PRIORITY*/ { /* Diagnostics Priority codes. See page 35.*/
	DP_LOW = 0x10,
	DP_MED = 0x40,
	DP_HIGH = 0x80,
	DP_CRITICAL = 0xE0,
	DP_VOLATILE = 0xF0
} ART_PRIORITY;

typedef struct {
	uint8_t id[8]; /* Array of 8 characters, the final character is a null termination.*/
	uint16_t opcode; /* The OpCode defines the class of data following ArtPoll within this UDP packet.*/
	uint8_t protVerHi; /* High byte of the Art-Net protocol revision number.*/
	uint8_t protVerLo; /* Low byte of the Art-Net protocol revision number. Current value 14. Controllers should ignore communication with nodes using a protocol version lower than 14.*/
} ArtNetPacket;

typedef struct /*_ArtPollPacket*/ {
	ArtNetPacket hdr;
	uint8_t talkToMe; /* Set behaviour of Node. See TALK_TO_ME*/
	uint8_t priority; /* The lowest priority of diagnostics message that should be send. */
} ArtNetPollPacket;

typedef struct /*_ArtPollReplyPacket*/ {
	uint8_t id[8]; /* Array of 8 characters, the final character is a null termination.*/
	uint16_t opcode; /* OpPollReply. Transmitted low byte first. */
	uint8_t address[4]; /* Array containing the Node's IP address. First array entry is most significant byte of address.*/
	uint16_t port; /* The Port is always 0x1936. Transmitted low byte first.*/
	uint8_t versInfoH; /* High byte of Node's firmware revision number.*/
	uint8_t versInfoL; /* Low byte of Node's firmware revision number.*/
	uint8_t netSwitch; /* Bits 14-8 of the 15 bit Port-Address are encoded into the bottom 7 bits of this field. This is used in combination with SubSwitch and SwIn[] or SwOut[] to produce the full universe address.*/
	uint8_t subSwitch; /* Bits 7-4 of the 15 bit Port-Address are encoded into the bottom 4 bits of this field. This is used in combination with NetSwitch and SwIn[] or SwOut[] to produce the full universe address.*/
	uint8_t oemHi; /* The high byte of the OEM value.*/
	uint8_t oem; /* The low byte of the OEM value. The OEM word describes the equipment vendor and the feature set available. Bit 15 high indicates extended features available. */
	uint8_t ubeaVersion; /* This field contains the firmware version of the User Bios Extension Area (UBEA). If the UBEA is not programmed, this field contains zero.*/
	uint8_t status1; /* General Status register containing bit fields (see ART_STATUS1). */
	uint8_t estaManLo; /* The ESTA manufacturer code.*/
	uint8_t estaManHi; /* Hi byte of above. */
	uint8_t shortName[18]; /* The array represents a null terminated short name for the node.*/
	uint8_t longName[64]; /* The array represents a null terminated long name for the node.*/
	uint8_t nodeReport[64]; /* The array is a textual report of the node's operating status or operational errors.*/
	uint8_t numPortsHi; /* The high byte of the word describing the number of input or output ports. The high byte is for future expansion and is currently zero.*/
	uint8_t numPortsLo; /* The low byte of the word describing the number of input or output ports. If number of inputs is not equal to number of outputs, the largest value is taken. Zero is legal value if no input or output ports are implemented. The maximal value is 4.*/
	uint8_t portTypes[4]; /* This array defines the operation and protocol of each channel. (page 22)*/
	uint8_t goodInput[4]; /* This array defines input status of the node. (page 22)*/
	uint8_t goodOutput[4]; /* This array defines output status of the node (page 22)*/
	uint8_t swIn[4]; /* Bits 3-0 of the 15 bit Port-Address for each of the 4 possible input ports are encoded into the low nibble.*/
	uint8_t swOut[4]; /* Bits 3-0 of the 15 bit Port-Address for each of the 4 possible output ports are encoded into the low nibble.*/
	uint8_t swVideo; /* deprecated.*/
	uint8_t swMacro; /* If the node supports macro key inputs, this byte represesnts the trigger values.*/
	uint8_t swRemote; /* If the node supports remote trigger inputs, this byte represents the trigger values. */
	uint8_t spare1;
	uint8_t spare2;
	uint8_t spare3;
	uint8_t style; /* The style code defines the equipment style of the device (see STYLE_CODES)*/
	uint8_t mac[6]; /* MAC address*/
	uint8_t bindIp[4]; /* If this unit is part of a larger or modular product, this is the IP of the root device*/
	uint8_t bindIndex; /* Set to zero if no binding, otherwise this number represents the order of bound devices. A lower number means closer to root device. A value of 1 means root device.*/
	uint8_t status2; /* see STATUS2*/
	uint8_t filler[26]; /* Transmit as zero. For future expansion.*/
} ArtPollReplyPacket;

typedef struct /*_ArtAddressPacket*/ {
	ArtNetPacket hdr;
	uint8_t netSwitch; /* see ArtPollReplyPacket description. */
	uint8_t filler2; /* Pad length to match ArtPoll*/
	uint8_t shortName[18]; /* see ArtPollReplyPacket description. */
	uint8_t longName[64];
	uint8_t swIn[4];
	uint8_t swOut[4];
	uint8_t subSwitch;
	uint8_t swVideo; /* reserved */
	uint8_t command; /* Node configuration commands.*/
} ArtAddressPacket;

typedef struct /*_ArtDiagDataPacket*/ {
	ArtNetPacket hdr;
	uint8_t filler1;
	uint8_t priority;
	uint8_t filler2;
	uint8_t filler3;
	uint8_t lengthHi;
	uint8_t lengthLo;
	uint8_t data[]; /* max 512 bytes including null term.*/
} ArtDiagDataPacket;

typedef struct /*_ArtDmxPacket*/ { /* page 44*/
	ArtNetPacket hdr;
	uint8_t sequence; /* The sequence number is used to ensure that ArtDMX packets are used in the correct order.*/
	uint8_t physical; /* The physical input port from which DMX512 data was input. This field is for information only. Use universe for data routing.*/
	uint8_t subUni; /* The lowByte of the 15 bit Port-Address to which this packet is destined.*/
	uint8_t net; /* The top 7 bits of the 15 bit Port-Address to which this packet is destined.*/
	uint16_t length; /* The length of the DMX512 data array. This value should be an even number in the range 2 - 512.*/
	uint8_t data[]; /* A array of DMX512 lighting data. Length is defined in the above variable length.*/
} ArtDmxPacket;

int artnet_handler(CONFIG* config);
