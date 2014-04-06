#include "contiki.h"
#include "groot.h"

#include "lib/list.h"
#include "lib/memb.h"

#include <stdio.h>
#include <string.h>

#define PRINT2ADDR(addr) printf("%02x%02x:%02x%02x",(addr)->u8[3], (addr)->u8[2], (addr)->u8[1], (addr)->u8[0])

LIST(queries_list);
MEMB(current_queries, GROOT_QUERY_ITEM, GROOT_QUERY_LIMIT);
MEMB(cluster_children, GROOT_SRT_CHILD, GROOT_QUERY_LIMIT*GROOT_CHILD_LIMIT);

GROOT_SENSORS supported_sensors;
/*------------------------------------------------- Debug Methods -------------------------------------------------------*/
static void
print_raw_packetbuf(void){
  uint16_t i;
  for(i = 0; i < packetbuf_hdrlen(); i++) {
    printf("%02x ", *(char *)(packetbuf_hdrptr() + i));
  }
  printf("| ");
  for(i = 0; i < packetbuf_datalen(); i++) {
    printf("%02x ", *(char *)(packetbuf_dataptr() + i));
  }
  printf("[%d]\n", (int16_t) (packetbuf_dataptr() - packetbuf_hdrptr()));
}

static void
print_hdr(GROOT_HEADER *hdr){
	printf("!------ PRINTING:");
	PRINT2ADDR(&rimeaddr_node_addr);
	printf(" ------! \n");
	printf("{ Source: ");
	PRINT2ADDR(&hdr->esender);
	printf(" Version: %d MAGIC: %c%c ", hdr->protocol.version, hdr->protocol.magic[0], hdr->protocol.magic[1]);
	printf("Type: %02x Query ID: %d Sample ID: %d Sample Rate: %d Aggregator: %d } \n", hdr->type, hdr->query_id, hdr->sample_id ,hdr->sample_rate, hdr->aggregator);
}

static void
copy_sensors(GROOT_SENSORS *origin, GROOT_SENSORS *target){
	target->co2 = origin->co2;
	target->no = origin->no;
	target->temp = origin->temp;
	target->humidity = origin->humidity;
}

/*------------------------------------------------- Other Methods ----------------------------------------------------------*/
static uint8_t
is_groot_protocol(GROOT_HEADER *hdr){
	if(hdr->protocol.version != GROOT_VERSION){
		return 0;
	}
	if(hdr->protocol.magic[0] != 'G' || hdr->protocol.magic[1] != 'T'){
		return 0;
	}
	return 1;
}

static uint8_t
is_new_query(uint16_t query_id, const rimeaddr_t *esender){
	if(list_length(queries_list) == 0){
		return 1;
	}

	GROOT_HEADER *i;
	for(i = list_head(queries_list); i != NULL; i = list_item_next(queries_list)){
		if(i->query_id == query_id && rimeaddr_cmp(&i->esender, esender)){
			printf("Query Exists!!");
			return 0;
		}
	}
	printf("Query does not exist!!");
	return 1;
}

static void
packet_loader_hdr(GROOT_HEADER *hdr){
	packetbuf_clear_hdr();
	packetbuf_hdralloc(sizeof(GROOT_HEADER));
	memcpy(packetbuf_hdrptr(), hdr, sizeof(GROOT_HEADER));
}

/*--------------------------------------------- Main Methods ------------------------------------------------------------*/
void
groot_init(GROOT_SENSORS *sensors){
	printf("TESTING HEY!!! \n");
	//Copy Current Sensors
	copy_sensors(sensors, &supported_sensors);
	//Initialize Lists
	list_init(queries_list);
	memb_init(&current_queries);
	memb_init(&cluster_children);
}

int
groot_subscribe_snd(struct broadcast_conn *dc, uint16_t query_id, uint16_t sample_rate, GROOT_SENSORS *data_required, uint8_t aggregator){
	GROOT_HEADER hdr;

	//Initialise query header
	hdr.protocol.version = GROOT_VERSION;
	hdr.protocol.magic[0] = 'G';
	hdr.protocol.magic[1] = 'T';
	
	//Main Variables
	hdr.is_cluster_head = 0;
	hdr.type = GROOT_SUBSCRIBE_TYPE;
	hdr.query_id = query_id;
	hdr.sample_id = 0;
	hdr.sample_rate = sample_rate;
	hdr.aggregator = aggregator;
	copy_sensors(data_required, &hdr.sensors_required);

	//Current Address
	rimeaddr_copy(&hdr.esender, &rimeaddr_node_addr);

	//Create Query Packet
	packet_loader_hdr(&hdr);
	//Send packet
	broadcast_send(dc);
	return 0;
}

int
groot_subscribe_rcv(struct broadcast_conn *c, const rimeaddr_t *from){
	GROOT_QUERY_ITEM new_qry;
	GROOT_HEADER *hdr;
	uint8_t type;

	hdr = (GROOT_HEADER*) packetbuf_dataptr();
	print_hdr(hdr);
	print_raw_packetbuf();
	//Is not correct protocol
	if(!is_groot_protocol(hdr)){
		return 0;
	}

	type = hdr->type;
	switch(type){
		case GROOT_SUBSCRIBE_TYPE:
			//Can I handle this?
			printf("SUBSCRIBING!! \n");

			if(!is_new_query(hdr->query_id, &hdr->esender)){
				return 0;
			}

			printf("NEW QUERY!! \n");
			//Add to list
			
			//Broadcast message if no head available be as head!
			break;
		case GROOT_UNSUBSCRIBE_TYPE:
			//Do I have this query?
			printf("UNSUBSRIBING!! \n");
			break;
		case GROOT_NEW_MOTE_TYPE:
			//New mote wants in! Check if query he can join!
			printf("Im a new fella!! \n");
			break;
		case GROOT_PUBLISH_TYPE:
			//Publish Sensed data
			printf("Im publishing!! \n");
			break;
		default:
			return 0;
	}

	return 1;
}

void
groot_intent_snd(){

}

void
groot_intent_rcv(){

}

void
groot_unsubscribe_snd(){

}

void
groot_unsubscribe_rcv(){

}

void
groot_alternate_snd(){

}

void
groot_alternate_rcv(){

}

void
groot_publish_rcv(){

}

void
groot_publish_snd(){

}