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

#ifndef GROOT_SAMPLER
 	#define GROOT_SAMPLER 1*CLOCK_SECOND

#endif

#ifndef GROOT_QUERY_LIMIT 
 	#define GROOT_QUERY_LIMIT 5
#endif

#ifndef GROOT_CHILD_LIMIT
 	#define GROOT_CHILD_LIMIT 3
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
 	struct broadcast_conn dc;
 	struct broadcast_conn rc;
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

#ifndef GROOT_HEADER_PROTOCOL
	struct GROOT_HEADER_PROTOCOL{
		uint8_t version;
		uint8_t magic[2];
	};
#endif

#ifndef GROOT_HEADER
	struct GROOT_HEADER{
		struct GROOT_HEADER_PROTOCOL protocol;
		rimeaddr_t esender;
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
		uint8_t has_cluster_head;
		unsigned long last_sampled;
		struct GROOT_QUERY query;
	};
#endif

#ifndef GROOT_CB
	struct GROOT_CB{
		int tmp;
	};
#endif

#ifndef GROOT_LOCAL
 	struct GROOT_LOCAL{
 		struct ctimer sampler_ctimer;
 		struct GROOT_CB cb_vars;
 		struct GROOT_SENSORS supported_sensors;
 	};
#endif

/**
 * Method Definitions
 */
void
groot_prot_init(struct GROOT_SENSORS *sensors);

int
groot_subscribe_snd(struct broadcast_conn *dc, uint16_t query_id, uint16_t sample_rate, struct GROOT_SENSORS *data_required, uint8_t aggregator);

int
groot_rcv(struct broadcast_conn *c, const rimeaddr_t *from);

int
groot_unsubscribe_snd(struct broadcast_conn *dc, uint16_t query_id);

void
groot_intent_snd();

void
groot_intent_rcv();

void
groot_unsubscribe_rcv();

void
groot_alternate_snd();

void
groot_alternate_rcv();

void
groot_publish_rcv();

void
groot_publish_snd();