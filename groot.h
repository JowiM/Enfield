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

#ifndef GROOT_QUERY_LIMIT 
 	#define GROOT_QUERY_LIMIT 10
#endif

#ifndef GROOT_CHILD_LIMIT
 	#define GROOT_CHILD_LIMIT 5
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
 typedef struct {
 	struct broadcast_conn dc;
 	struct broadcast_conn rc;
 } GROOT_CHANNELS;
#endif

/**
 * PACKET INFO
 */
#ifndef GROOT_SENSORS
	typedef struct {
		uint8_t co2;
		uint8_t no;
		uint8_t temp;
		uint8_t humidity;
	} GROOT_SENSORS;
#endif

#ifndef GROOT_HEADER_PROTOCOL
	typedef struct {
		uint8_t version;
		uint8_t magic[2];
	} GROOT_HEADER_PROTOCOL;
#endif

#ifndef GROOT_HEADER
	typedef struct {
		GROOT_HEADER_PROTOCOL protocol;
		rimeaddr_t esender;
		uint8_t is_cluster_head;
		uint8_t type;
		uint16_t query_id;
		uint16_t sample_id;
		uint16_t sample_rate;
		uint8_t aggregator;
		GROOT_SENSORS sensors_required;
	} GROOT_HEADER;
#endif

#ifndef GROOT_SRT_CHILD
	typedef struct{
		rimeaddr_t child;
		int *next;
	} GROOT_SRT_CHILD;
#endif

#ifndef GROOT_QUERY_ITEM
	typedef struct{
		uint16_t query_id;
		rimeaddr_t esender;
		rimeaddr_t parent;
		rimeaddr_t parent_bkup;
		int last_sampled;
		GROOT_SRT_CHILD *children;
	} GROOT_QUERY_ITEM;
#endif

/**
 * Method Definitions
 */
void
groot_jibda(GROOT_SENSORS *sensors);

int
groot_subscribe_snd(struct broadcast_conn *dc, uint16_t query_id, uint16_t sample_rate, GROOT_SENSORS *data_required, uint8_t aggregator);

int
groot_subscribe_rcv(struct broadcast_conn *c, const rimeaddr_t *from);

void
groot_intent_snd();

void
groot_intent_rcv();

void
groot_unsubscribe_snd();

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