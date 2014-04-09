#include "contiki.h"
#include "groot.h"
#include "lib/list.h"
#include "lib/memb.h"
#include "stdio.h"
#include "string.h"
#include "net/rime.h"

#define PRINT2ADDR(addr) printf("%02x%02x:%02x%02x",(addr)->u8[3], (addr)->u8[2], (addr)->u8[1], (addr)->u8[0])

LIST(groot_qry_table);
MEMB(groot_qrys, struct GROOT_QUERY_ITEM, GROOT_QUERY_LIMIT);

static struct GROOT_LOCAL glocal;
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
print_hdr(struct GROOT_HEADER *hdr){
	printf("HEADER ");
	PRINT2ADDR(&rimeaddr_node_addr);
	printf(" - { Source: ");
	PRINT2ADDR(&hdr->esender);
	printf(" Version: %d MAGIC: %c%c ", hdr->protocol.version, hdr->protocol.magic[0], hdr->protocol.magic[1]);
	printf("Type: %02x Query ID: %d } \n", hdr->type, hdr->query_id);
}

static void
print_qry(struct GROOT_QUERY *qry){
	printf("QUERY ");
	PRINT2ADDR(&rimeaddr_node_addr);
	printf(" - { Sample Rate: %d Aggregator: %d } - ", qry->sample_rate, qry->aggregator);
	printf(" { CO2 - %d NO - %d TEMP - %d HUMIDITY - %d } \n", qry->sensors_required.co2, qry->sensors_required.no, qry->sensors_required.temp, qry->sensors_required.humidity);
}

static void
print_qrys(){
	struct GROOT_QUERY_ITEM *qry_itm;

	qry_itm = list_head(groot_qry_table);
	while(qry_itm != NULL){

		printf(" QUERY ITEM - [ ");
		printf(" QUERY ID: %d ESENDER: ", qry_itm->query_id);
		PRINT2ADDR(&qry_itm->esender);
		printf(" Parent: ");
		PRINT2ADDR(&qry_itm->parent);
		printf(" Has_cluster_head: %d", qry_itm->has_cluster_head);
		printf(" ] \n");

		qry_itm = qry_itm->next;
	}
}

/*------------------------------------------------- Other Methods ----------------------------------------------------------*/
static void
cb_sampler(void *i){
	struct GROOT_QUERY_ITEM *qry_itm = (struct GROOT_QUERY_ITEM *)i;
	printf("CALLBACK FOR: %d \n", qry_itm->query_id);

	
	
	ctimer_set(&qry_itm->query_timer, qry_itm->query.sample_rate, cb_sampler, qry_itm);
	return;
}

static void
cb_rm_query(void *i){
	printf("REMOVING QUERY!! \n");
	struct GROOT_QUERY_ITEM *lst_itm = (struct GROOT_QUERY_ITEM *)i;
	
	if(!ctimer_expired(&lst_itm->query_timer)){
		//Stop Sampe timer
		ctimer_stop(&lst_itm->query_timer);
	}

	list_remove(groot_qry_table, lst_itm);
	memb_free(&groot_qrys, lst_itm);
	memset(lst_itm, ' ', sizeof(struct GROOT_QUERY_ITEM));
}

static uint8_t
is_groot_protocol(struct GROOT_HEADER *hdr){
	if(hdr->protocol.version != GROOT_VERSION){
		return 0;
	}
	if(hdr->protocol.magic[0] != 'G' || hdr->protocol.magic[1] != 'T'){
		return 0;
	}
	return 1;
}

static uint8_t
is_capable(struct GROOT_SENSORS *qry_sensors){
	if((qry_sensors->co2 == 1 && glocal.sensors.co2 == 0) ||
		(qry_sensors->no == 1 && glocal.sensors.no == 0) ||
		(qry_sensors->temp == 1 && glocal.sensors.temp == 0) ||
		(qry_sensors->humidity == 1 && glocal.sensors.humidity == 0)){
		return 0;
	}

	return 1;
}

static void
packet_qry_creator(struct GROOT_HEADER *hdr, struct GROOT_QUERY *qry, uint16_t query_id, const rimeaddr_t *esender, 
	const rimeaddr_t *from, uint8_t type, uint16_t sample_rate, struct GROOT_SENSORS *data_required, 
	uint8_t aggregator, uint16_t sample_id){

	//QRY BODY
	qry->sample_id = sample_id;
	qry->sample_rate = sample_rate;
	qry->aggregator = aggregator;
	memcpy(&qry->sensors_required, data_required, sizeof(struct GROOT_SENSORS));

	//Initialise query header
	hdr->protocol.version = GROOT_VERSION;
	hdr->protocol.magic[0] = 'G';
	hdr->protocol.magic[1] = 'T';
	
	//Main Variables
	rimeaddr_copy(&hdr->esender, esender);
	rimeaddr_copy(&hdr->received_from, from);
	hdr->is_cluster_head = 0;
	hdr->type = type;
	hdr->query_id = query_id;
}

static void
packet_loader_qry(struct GROOT_HEADER *hdr, struct GROOT_QUERY *qry){
	int data_l = sizeof(struct GROOT_HEADER);
	if(qry != NULL){
		data_l += sizeof(struct GROOT_QUERY);
	}

	packetbuf_clear();
	packetbuf_set_datalen(data_l);
	
	struct GROOT_HEADER *pkt_hdr = packetbuf_dataptr();
	memcpy(pkt_hdr, hdr, sizeof(struct GROOT_HEADER));

	if(qry != NULL){
		struct GROOT_QUERY *pkt_qry = (packetbuf_dataptr() + sizeof(struct GROOT_HEADER));
		memcpy(pkt_qry, qry, sizeof(struct GROOT_QUERY));
	}
}

static struct  GROOT_QUERY_ITEM
*qry_to_list(struct GROOT_HEADER *hdr, struct GROOT_QUERY *qry_bdy, const rimeaddr_t *from, uint8_t is_serviced){
	struct GROOT_QUERY_ITEM *new_item;
	
	//LIST and all MEMORY USED
	if(list_length(groot_qry_table) >= GROOT_QUERY_LIMIT){
		return NULL;
	}

	new_item = memb_alloc(&groot_qrys);
	list_add(groot_qry_table, new_item);

	new_item->query_id = hdr->query_id;
	rimeaddr_copy(&new_item->esender, &hdr->esender);
	rimeaddr_copy(&new_item->parent, from);
	rimeaddr_copy(&new_item->parent_bkup, &rimeaddr_null);
	//Is sender a cluster head?
	//@todo request to join cluster
	new_item->has_cluster_head = 0;
	//Never Sampled
	new_item->is_serviced = is_serviced;
	//When unsubscribe received set this time
	new_item->unsubscribed = 0;
	//@todo add Children

	//Copy Query Values into row
	memcpy(&new_item->query, qry_bdy, sizeof(struct GROOT_QUERY));

	//If the query is serviced by this node create callback function to send samples
	if(is_serviced > 0){
		ctimer_set(&new_item->query_timer, qry_bdy->sample_rate, cb_sampler, new_item);
	}
	print_qrys();

	return new_item;
}

static struct GROOT_QUERY_ITEM
*find_query(uint16_t query_id, const rimeaddr_t *esender){
	struct GROOT_QUERY_ITEM *i;

	if(list_length(groot_qry_table) == 0){
		return NULL;
	}

	i = list_head(groot_qry_table);
	while(i != NULL){
		if(i->query_id == query_id && rimeaddr_cmp(&i->esender, esender)){
			return i;
		}
		i = i->next;
	}

	return NULL;
}

/*--------------------------------------------- Main Methods ------------------------------------------------------------*/
void
groot_prot_init(struct GROOT_SENSORS *sensors, struct GROOT_CHANNELS *channels){
	//Initialise data structures
	list_init(groot_qry_table);
	memb_init(&groot_qrys);
	//Copy Current Sensors
	memcpy(&glocal.sensors, sensors, sizeof(struct GROOT_SENSORS));
	//Set local channels
	glocal.channels = channels;
}

int
groot_subscribe_snd(uint16_t query_id, uint16_t sample_rate, struct GROOT_SENSORS *data_required, uint8_t aggregator){
	struct GROOT_HEADER hdr;
	struct GROOT_QUERY qry;
	rimeaddr_t esender;
	static rimeaddr_t received_from;

	rimeaddr_copy(&received_from, &rimeaddr_null);
	rimeaddr_copy(&esender, &rimeaddr_node_addr);

	packet_qry_creator(&hdr, &qry, query_id, &esender, &received_from, GROOT_SUBSCRIBE_TYPE, sample_rate, data_required, aggregator, 0);

	//Create Query Packet
	packet_loader_qry(&hdr, &qry);
	
	printf("SENDING!!! \n");
	//Send packet
	broadcast_send(&glocal.channels->bc);
	return 1;
}

int
groot_unsubscribe_snd(uint16_t query_id){
	struct GROOT_HEADER hdr;
	
	hdr.protocol.version = GROOT_VERSION;
	hdr.protocol.magic[0] = 'G';
	hdr.protocol.magic[1] = 'T';

	rimeaddr_copy(&hdr.received_from, &rimeaddr_null);
	rimeaddr_copy(&hdr.esender, &rimeaddr_node_addr);

	hdr.is_cluster_head = 0;
	hdr.type = GROOT_UNSUBSCRIBE_TYPE;
	hdr.query_id = query_id;

	packet_loader_qry(&hdr, NULL);

	broadcast_send(&glocal.channels->bc);
	return 1;
}

int
groot_rcv(const rimeaddr_t *from){
	struct GROOT_HEADER *hdr = NULL;
	struct GROOT_HEADER snd_hdr;
	struct GROOT_QUERY *qry_bdy = NULL; 
	struct GROOT_QUERY snd_bdy;
	struct GROOT_QUERY_ITEM *srch_item, *lst_itm = NULL;
	uint8_t type, is_serviced;

	hdr = (struct GROOT_HEADER*) packetbuf_dataptr();
	//I just sent this packet ignore
	if(rimeaddr_cmp(&hdr->received_from, &rimeaddr_node_addr) == 1){
		return 0;
	}

	//Is not correct protocol
	if(!is_groot_protocol(hdr)){
		return 0;
	}

	type = hdr->type;
	switch(type){
		case GROOT_SUBSCRIBE_TYPE:
			//Get Query from buffer
			qry_bdy = (struct GROOT_QUERY*) (packetbuf_dataptr() + sizeof(struct GROOT_HEADER));
			//Already Saved
			srch_item = find_query(hdr->query_id, &hdr->esender);
			if(srch_item != NULL){
				//Set Bkup connection
				if(rimeaddr_cmp(&srch_item->parent_bkup, &rimeaddr_null) > 0){
					rimeaddr_copy(&srch_item->parent_bkup, from);
				}
				return 0;
			}

			//Check that I have all the sensors needed
			is_serviced = 1;
			if(!is_capable(&qry_bdy->sensors_required)){
				is_serviced = 0;
			}
			printf("IS SERVICED: %d \n", is_serviced);

			//Add the new qry to the table
			lst_itm = qry_to_list(hdr, qry_bdy, from, is_serviced);
			printf("ST ITEM %d \n", lst_itm);
			//NOT added to the table do nothing. FOLLOWS ASSUMPTION ALL QUERIES WILL BE SAVED
			if(lst_itm == NULL){
				return 0;
			}

			//Re-Broadcast Query so next neighbours can see it.
			packet_qry_creator(&snd_hdr, &snd_bdy, lst_itm->query_id, &lst_itm->esender, from, GROOT_SUBSCRIBE_TYPE,
				lst_itm->query.sample_rate, &lst_itm->query.sensors_required, lst_itm->query.aggregator, lst_itm->query.sample_id);

			//Create Query Packet
			packet_loader_qry(&snd_hdr, &snd_bdy);
			//Send packet
			printf("SENDING BCAST FROM SENSOR!! - ");
			PRINT2ADDR(&rimeaddr_node_addr);
			printf("\n");
			broadcast_send(&glocal.channels->bc);

			//Clean up packet buffer
			packetbuf_clear();

			break;
		case GROOT_UNSUBSCRIBE_TYPE:
			//Do I have this query?
			printf("UNSUBSRIBING!! \n");

			lst_itm = find_query(hdr->query_id, &hdr->esender);
			//Return if item already deleted or already bcast the unsubscribed
			if(lst_itm == NULL || lst_itm->unsubscribed != 0){
				return 0;
			}

			lst_itm->unsubscribed = clock_seconds();
			//Keep the query in the file for a few seconds to stop rebroadcasting
			ctimer_set(&glocal.unsub_ctimer, GROOT_RM_UNSUBSCRIBE, cb_rm_query, lst_itm);			

			rimeaddr_copy(&hdr->received_from, from);
			broadcast_send(&glocal.channels->bc);

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