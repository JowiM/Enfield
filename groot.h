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

/**
 * Sensor Definitions
 */
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
/**
 * @brief the structure used to signal what sensors are available
 */
#ifndef GROOT_SENSORS
	struct GROOT_SENSORS{
		uint8_t co2;
		uint8_t no;
		uint8_t temp;
		uint8_t humidity;
	};
#endif

/**
 * @brief the strcuture used to pass the data
 */
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

/**
 * @brief the header structure
 */
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

/**
 * @brief The actual query structure
 */
#ifndef GROOT_QUERY
	struct GROOT_QUERY{
		uint16_t sample_id;
		uint16_t sample_rate;
		uint8_t aggregator;
		struct GROOT_SENSORS sensors_required;
	};
#endif

/**
 * @brief The list that will hold the children associated with a query
 */
#ifndef GROOT_SRT_CHILD
	struct GROOT_SRT_CHILD{
		rimeaddr_t address;
		unsigned long last_set;
		struct GROOT_SENSORS_DATA data;
		struct GROOT_SRT_CHILD *next;
	};
#endif

/**
 * @brief Contains all the queries that is running on the network
 * @details  Contails all the queries that where sent by all sinks. Its also
 *           holds information reguarding routing the same query/
 */
#ifndef GROOT_QUERY_ITEM
	struct GROOT_QUERY_ITEM{
		struct GROOT_QUERY_ITEM *next;
		uint16_t query_id;
		rimeaddr_t ereceiver;
		rimeaddr_t parent;
		rimeaddr_t rcv_alter;
		uint8_t parent_is_cluster;
		uint8_t agg_passes;
		uint8_t is_serviced;
		unsigned long parent_last_seen; //When was parent last publish
		unsigned long unsubscribed; //What time unsubscribe received
		unsigned long last_published; //Last time the query was published
		struct ctimer query_timer;
		struct ctimer maintainer_t;
		struct GROOT_QUERY query;
		struct GROOT_SRT_CHILD *children;
	};
#endif

/**
 * @brief Local structur used to hold sensor and channels
 * @details Local structure used to hold sensor and channels
 * 
 * @param GROOT_SENSORS Sensors supported by the mote
 * @param GROOT_CHANNELS Channels needed for mote
 * @param is_sink is this a sink or a sensor?
 */
#ifndef GROOT_LOCAL
 	struct GROOT_LOCAL{
 		struct GROOT_SENSORS sensors;
 		struct GROOT_CHANNELS *channels;
 		uint8_t is_sink;
 	};
#endif

/**
 * @brief Initialise protocol
 * @details Initialise Groot portocol
 * 
 * @param GROOT_SENSORS Sensors it supports
 * @param GROOT_CHANNELS Channels
 * @param is_sink is_sink or sensor?
 */
void
groot_prot_init(struct GROOT_SENSORS *sensors, struct GROOT_CHANNELS *channels, uint8_t is_sink);

/**
 * @brief Send the actual query to the sensors
 * @details Pass the query to all sensors.
 * 
 * @param query_id The id of the query to send
 * @param type What query will be sent. Subscribe, or update??
 * @param sample_rate How often to sample the query
 * @param GROOT_SENSORS What sensor data will be collected.
 * @param aggregator What aggregation to use.
 */
int
groot_qry_snd(uint16_t query_id, uint8_t type, uint16_t sample_rate, struct GROOT_SENSORS *data_required, uint8_t aggregator);

/**
 * @brief Handle the receive values.
 * @details Handle the receive values.
 * 
 * @param from pass the address of whom the message arrived
 */
int
groot_rcv(const rimeaddr_t *from);

/**
 * @brief Send unsubscribe query.
 * @details Send unsubscribe query
 * 
 * @param query_id The query id needed for deletion
 */
int
groot_unsubscribe_snd(uint16_t query_id);