/**
 * @file
 * 	Header file for GROOT. Holds most data strucutures needed.
 * @author 
 * 	Johann Mifsud <johann.mifsud.13@ucl.ac.uk>
 */

#include "net/rime.h"

/**
 * General Definitions
 */

#ifndef GROOT_VERSION
	#define GROOT_VERSION 1
#endif

#ifndef DEBUG_LEVEL
	#define DEBUG_LEVEL 0
#endif

#ifndef GROOT_RM_UNSUBSCRIBE
 	#define GROOT_RM_UNSUBSCRIBE 5*CLOCK_SECOND
#endif

#ifndef GROOT_QUERY_LIMIT 
 	#define GROOT_QUERY_LIMIT 10
#endif

#ifndef GROOT_CHILD_LIMIT
 	#define GROOT_CHILD_LIMIT 3
#endif

#ifndef MAX_RETRANSMISSION
 	#define MAX_RETRANSMISSION 3
#endif

/**
 * Routing Definitions
 */
#ifndef GROOT_DATA_CHANNEL 
	#define GROOT_DATA_CHANNEL 128
#endif

#ifndef GROOT_ROUTING_CHANNEL
 	#define GROOT_ROUTING_CHANNEL 129
#endif

/**
 * Packet Definitions
 */

/**
 * The Data Channel Packet Types
 */
#ifndef GROOT_SUBSCRIBE_TYPE 
 	#define GROOT_SUBSCRIBE_TYPE 0x01
#endif

#ifndef GROOT_UNSUBSCRIBE_TYPE
 	#define GROOT_UNSUBSCRIBE_TYPE 0x02
#endif

#ifndef GROOT_NEW_MOTE_TYPE
 	#define GROOT_NEW_MOTE_TYPE 0x03
#endif

#ifndef GROOT_ALTERATION_TYPE
 	#define GROOT_ALTERATION_TYPE 0x04
#endif
#ifndef GROOT_PUBLISH_TYPE
 	#define GROOT_PUBLISH_TYPE 0xC8
#endif

/**
 * AGGREGATORS
 */
#ifndef GROOT_NO_AGGREGATION
 	#define GROOT_NO_AGGREGATION 0x00
#endif

#ifndef GROOT_MAX
	#define GROOT_MAX 0x01
#endif

#ifndef GROOT_AVG
	#define GROOT_AVG 0x02
#endif

#ifndef GROOT_MIN
	#define GROOT_MIN 0x03
#endif

#ifndef GROOT_MEDIAN
	#define GROOT_MEDIAN 0x04
#endif

/**
 * GROOT CHANNELS
 */
#ifndef GROOT_CHANNELS
 struct GROOT_CHANNELS{
 	struct broadcast_conn bc;
 	struct runicast_conn rc;
 };
#endif

/**
 * PACKET INFO
 */
#ifndef GROOT_SENSORS
	struct GROOT_SENSORS{
		uint8_t co2;
		uint8_t no;
		uint8_t temp;
		uint8_t humidity;
	};
#endif

#ifndef GROOT_SENSORS_DATA
	struct GROOT_SENSORS_DATA{
		uint16_t co2;
		uint16_t no;
		uint16_t temp;
		uint16_t humidity;
	};
#endif

#ifndef GROOT_HEADER_PROTOCOL
	struct GROOT_HEADER_PROTOCOL{
		uint8_t version;
		uint8_t magic[2];
	};
#endif

#ifndef GROOT_HEADER
	struct GROOT_HEADER{
		struct GROOT_HEADER_PROTOCOL protocol;
		rimeaddr_t end_addr;
		rimeaddr_t esender;
		rimeaddr_t ereceiver;
		rimeaddr_t received_from;
		uint8_t is_cluster_head;
		uint8_t type;
		uint16_t query_id;
	};
#endif

#ifndef GROOT_QUERY
	struct GROOT_QUERY{
		uint16_t sample_id;
		uint16_t sample_rate;
		uint8_t aggregator;
		struct GROOT_SENSORS sensors_required;
	};
#endif

#ifndef GROOT_SRT_CHILD
	struct GROOT_SRT_CHILD{
		rimeaddr_t child;
		int *next;
	};
#endif

#ifndef GROOT_QUERY_ITEM
	struct GROOT_QUERY_ITEM{
		struct GROOT_QUERY_ITEM *next;
		uint16_t query_id;
		rimeaddr_t esender;
		rimeaddr_t parent;
		rimeaddr_t parent_bkup;
		uint8_t has_cluster_head;
		uint8_t is_serviced;
		unsigned long unsubscribed;
		struct ctimer query_timer;
		struct GROOT_QUERY query;
	};
#endif

#ifndef GROOT_LOCAL
 	struct GROOT_LOCAL{
 		struct ctimer unsub_ctimer;
 		struct GROOT_SENSORS sensors;
 		struct GROOT_CHANNELS *channels;
 	};
#endif

/**
 * Method Definitions
 */
void
groot_prot_init(struct GROOT_SENSORS *sensors, struct GROOT_CHANNELS *channels);

int
groot_qry_snd(uint16_t query_id, uint8_t type, uint16_t sample_rate, struct GROOT_SENSORS *data_required, uint8_t aggregator);

int
groot_rcv(const rimeaddr_t *from);

int
groot_unsubscribe_snd(uint16_t query_id);

void
groot_intent_snd();

void
groot_intent_rcv();