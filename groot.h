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

#ifndef GROOT_RETRIES_AGGREGATION
 	#define GROOT_RETRIES_AGGREGATION 3
#endif

 #ifndef GROOT_RETRIES_PARENT
 	#define GROOT_RETRIES_PARENT 3
#endif

#ifndef GROOT_QUERY_LIMIT 
 	#define GROOT_QUERY_LIMIT 10
#endif

#ifndef GROOT_CHILD_LIMIT
 	#define GROOT_CHILD_LIMIT 5
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

#ifndef GROOT_CLUSTER_JOIN_TYPE
	#define GROOT_CLUSTER_JOIN_TYPE 0x06
#endif

#ifndef GROOT_CLUSTER_ACCEPTED_TYPE
 	#define GROOT_CLUSTER_ACCEPTED_TYPE 0x07
#endif

#ifndef GROOT_CLUSTER_REJECTED_TYPE
 	#define GROOT_CLUSTER_REJECTED_TYPE 0x08
#endif

#ifndef GROOT_PUBLISH_TYPE
 	#define GROOT_PUBLISH_TYPE 0xC8
#endif

#ifndef SENSOR_CO2
 	#define SENSOR_CO2 0x01
#endif

#ifndef SENSOR_NO
 	#define SENSOR_NO 0x02
#endif

#ifndef SENSOR_HUMIDITY
 	#define SENSOR_HUMIDITY 0x03
#endif

#ifndef SENSOR_TEMP
 	#define SENSOR_TEMP 0x04
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
		float co2;
		float no;
		float temp;
		float humidity;
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
		rimeaddr_t to;
		uint8_t is_cluster_head;
		uint8_t type;
		rimeaddr_t ereceiver;
		uint16_t query_id;
		rimeaddr_t received_from;
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
		rimeaddr_t address;
		unsigned long last_set;
		struct GROOT_SENSORS_DATA data;
		struct GROOT_SRT_CHILD *next;
	};
#endif

#ifndef GROOT_QUERY_ITEM
	struct GROOT_QUERY_ITEM{
		struct GROOT_QUERY_ITEM *next;
		uint16_t query_id;
		rimeaddr_t ereceiver;
		rimeaddr_t parent;
		uint8_t parent_is_cluster;
		uint8_t agg_passes;
		uint8_t is_serviced;
		unsigned long parent_last_seen;
		unsigned long unsubscribed;
		unsigned long last_published;
		struct ctimer query_timer;
		struct ctimer maintainer_t;
		struct GROOT_QUERY query;
		struct GROOT_SRT_CHILD *children;
	};
#endif

#ifndef GROOT_LOCAL
 	struct GROOT_LOCAL{
 		struct GROOT_SENSORS sensors;
 		struct GROOT_CHANNELS *channels;
 		uint8_t is_sink;
 	};
#endif

/**
 * Method Definitions
 */
void
groot_prot_init(struct GROOT_SENSORS *sensors, struct GROOT_CHANNELS *channels, uint8_t is_sink);

int
groot_qry_snd(uint16_t query_id, uint8_t type, uint16_t sample_rate, struct GROOT_SENSORS *data_required, uint8_t aggregator);

int
groot_rcv(const rimeaddr_t *from);

int
groot_unsubscribe_snd(uint16_t query_id);