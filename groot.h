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
	#define GROOT_VERSION 1;
#endif

#ifndef DEBUG_LEVEL
	#define DEBUG_LEVEL 0;
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
 	#define GROOT_SUBSRIBE_TYPE 0x01
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
#ifndef GROOT_HEADER_PROTOCOL
	typedef struct {
		uint8_t version;
		uint8_t magic[2];
	} GROOT_HEADER_PROTOCOL;
#endif

#ifndef GROOT_TOC
	typedef struct {
		uint8_t co2;
		uint8_t no;
		uint8_t temp;
		uint8_t humidity;
	} GROOT_TOC;
#endif

#ifndef GROOT_HEADER
	typedef struct {
		GROOT_HEADER_PROTOCOL protocol;
		rimeaddr_t esender;
		uint8_t type;
		uint16_t query_id;
		uint16_t sample_id;
		uint16_t sample_rate;
		uint8_t aggregator;
		GROOT_TOC sensors_required;
	} GROOT_HEADER;
#endif

#ifndef GROOT_QUERY_RW
	typedef struct{
		uint16_t query_id;
		rimeaddr_t esender;
		rimeaddr_t sender;
		rimeaddr_t sender_bkup;
		int *next;
	} GROOT_QUERY_RW;
#endif

#ifndef GROOT_SRT_RW
	typedef struct{
		uint16_t query_id;
		rimeaddr_t parent;
		int *children;
	} GROOT_SRT_RW;
#endif

#ifndef GROOT_SRT_CHILD
	typedef struct{
		rimeaddr_t child;
		int *next;
	} GROOT_SRT_CHILD;
#endif

/**
 * Method Definitions
 */
void
groot_intent_snd();

void
groot_intent_rcv();

void
groot_subscribe_snd();

void
groot_subscribe_rcv();

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