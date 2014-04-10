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
	printf(" Destination: ");
	PRINT2ADDR(&hdr->ereceiver);
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
print_data(struct GROOT_SENSORS_DATA *data){
	printf("DATA ");
	PRINT2ADDR(&rimeaddr_node_addr);
	printf(" - { CO2 %d NO - %d TEMP - %d HUMIDITY - %d } \n", data->co2, data->no, data->temp, data->humidity);
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
	random_sensor_readings(struct GROOT_SENSORS *required, struct GROOT_SENSORS_DATA *data){
		if(required->co2 == 1){
			data->co2 = 78;
		} else {
			data->co2 = 0;
		}

		if(required->no == 1){
			data->no = 78;
		} else {
			data->no = 0;
		}

		if(required->temp == 1){
			data->temp = 15;
		} else {
			data->temp = 0;
		}

		if(required->humidity == 1){
			data->humidity = 30;
		} else {
			data->humidity = 0;
		}
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
	const rimeaddr_t *ereceiver, const rimeaddr_t *from, uint8_t type, uint16_t sample_rate, 
	struct GROOT_SENSORS *data_required, uint8_t aggregator, uint16_t sample_id){

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
	rimeaddr_copy(&hdr->ereceiver, ereceiver);
	rimeaddr_copy(&hdr->received_from, from);
	hdr->is_cluster_head = 0;
	hdr->type = type;
	hdr->query_id = query_id;
}

static void
packet_loader_qry(struct GROOT_HEADER *hdr, struct GROOT_QUERY *qry, struct GROOT_SENSORS_DATA *sensors_data){
	int data_l = sizeof(struct GROOT_HEADER);
	struct GROOT_QUERY *pkt_qry = NULL;
	struct GROOT_SENSORS_DATA *pkt_data = NULL;

	if(qry != NULL){
		data_l += sizeof(struct GROOT_QUERY);
	}

	if(sensors_data != NULL){
		data_l += sizeof(struct GROOT_SENSORS_DATA);
	}

	packetbuf_clear();
	packetbuf_set_datalen(data_l);
	
	struct GROOT_HEADER *pkt_hdr = packetbuf_dataptr();
	memcpy(pkt_hdr, hdr, sizeof(struct GROOT_HEADER));
	

	if(qry != NULL){
		pkt_qry = (packetbuf_dataptr() + sizeof(struct GROOT_HEADER));
		memcpy(pkt_qry, qry, sizeof(struct GROOT_QUERY));
	}

	if(sensors_data != NULL){
		if(qry == NULL){
			pkt_data = (packetbuf_dataptr()+sizeof(struct GROOT_HEADER));
		} else {
			pkt_data = (packetbuf_dataptr()+sizeof(struct GROOT_HEADER)+sizeof(struct GROOT_QUERY));
		}
		memcpy(pkt_data, sensors_data, sizeof(struct GROOT_SENSORS_DATA));
	}
}

static void
cb_sampler(void *i){
	struct GROOT_QUERY_ITEM *qry_itm = (struct GROOT_QUERY_ITEM *)i;
	struct GROOT_HEADER hdr;
	struct GROOT_QUERY qry;
	struct GROOT_SENSORS_DATA sensors_data;

	printf("CALLBACK FOR: %d \n", qry_itm->query_id);

	random_sensor_readings(&qry_itm->query.sensors_required, &sensors_data);

	hdr.protocol.version = GROOT_VERSION;
	hdr.protocol.magic[0] = 'G';
	hdr.protocol.magic[1] = 'T';
	rimeaddr_copy(&hdr.ereceiver, &qry_itm->esender);
	rimeaddr_copy(&hdr.esender, &rimeaddr_node_addr);
	rimeaddr_copy(&hdr.received_from, &rimeaddr_null);

	hdr.is_cluster_head = 0;
	hdr.type = GROOT_PUBLISH_TYPE;
	hdr.query_id = qry_itm->query_id;

	memcpy(&qry, &qry_itm->query, sizeof(struct GROOT_QUERY));
	packet_loader_qry(&hdr, &qry, &sensors_data);
	printf("SENDING DATA! - ");
	PRINT2ADDR(&rimeaddr_node_addr);
	printf("\n");
	runicast_send(&glocal.channels->rc, &qry_itm->parent, MAX_RETRANSMISSION);

	ctimer_set(&qry_itm->query_timer, qry_itm->query.sample_rate, cb_sampler, qry_itm);
	return;
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

/*--------------------------------------------- RCV METHODS -------------------------------------------------------------*/
static int
rcv_subscribe(struct GROOT_HEADER *hdr, const rimeaddr_t *from){
	struct GROOT_QUERY *qry_bdy = NULL;	
	struct GROOT_QUERY_ITEM *lst_itm = NULL;
	struct GROOT_QUERY snd_bdy;
	struct GROOT_HEADER snd_hdr;
	uint8_t is_serviced = 1;

	//Get Query from buffer
	qry_bdy = (struct GROOT_QUERY*) (packetbuf_dataptr() + sizeof(struct GROOT_HEADER));
	
	//Already Saved
	lst_itm = find_query(hdr->query_id, &hdr->esender);
	if(lst_itm != NULL){
		//Set Bkup connection if NULL
		if(rimeaddr_cmp(&lst_itm->parent_bkup, &rimeaddr_null) > 0){
			rimeaddr_copy(&lst_itm->parent_bkup, from);
		}
		return 0;
	}

	//Check that I have all the sensors needed
	if(!is_capable(&qry_bdy->sensors_required)){
		is_serviced = 0;
	}

	//Add the new qry to the table
	lst_itm = qry_to_list(hdr, qry_bdy, from, is_serviced);
	//NOT added to the table do nothing. FOLLOWS ASSUMPTION ALL QUERIES WILL BE SAVED
	if(lst_itm == NULL){
		return 0;
	}

	//Re-Broadcast Query so next neighbours can see it.
	//@todo check why I needed this?
	packet_qry_creator(&snd_hdr, &snd_bdy, lst_itm->query_id, &lst_itm->esender, &rimeaddr_null, from, GROOT_SUBSCRIBE_TYPE,
		lst_itm->query.sample_rate, &lst_itm->query.sensors_required, lst_itm->query.aggregator, lst_itm->query.sample_id);

	//Create Query Packet
	packet_loader_qry(&snd_hdr, &snd_bdy, NULL);
	//Send packet
	printf("SENDING BCAST FROM SENSOR!! - ");
	PRINT2ADDR(&rimeaddr_node_addr);
	printf("\n");
	if(broadcast_send(&glocal.channels->bc) == 0){
		return 0;
	}
	return 1;
}

static int
rcv_unsubscribe(struct GROOT_HEADER *hdr, const rimeaddr_t *from){
	struct GROOT_QUERY_ITEM *lst_itm = NULL;

	lst_itm = find_query(hdr->query_id, &hdr->esender);
	//Return if item already deleted or already bcast the unsubscribed
	if(lst_itm == NULL || lst_itm->unsubscribed != 0){
		return 0;
	}

	lst_itm->unsubscribed = clock_seconds();
	//Keep the query in the file for a few seconds to stop rebroadcasting
	ctimer_set(&glocal.unsub_ctimer, GROOT_RM_UNSUBSCRIBE, cb_rm_query, lst_itm);			

	rimeaddr_copy(&hdr->received_from, from);
	if(broadcast_send(&glocal.channels->bc) == 0){
		return 0;
	}
	return 1;
}

static int
rcv_publish(struct GROOT_HEADER *hdr){
	struct GROOT_QUERY_ITEM *lst_itm = NULL;
	struct GROOT_SENSORS_DATA *sns_data = NULL;

	if(rimeaddr_cmp(&hdr->ereceiver, &rimeaddr_node_addr) > 0){
		printf("RECEIVED DATA SUCCESS!!! \n");
		return 0;
	}

	lst_itm = find_query(hdr->query_id, &hdr->ereceiver);
	if(lst_itm == NULL){
		return;
	}

	sns_data = (struct GROOT_SENSORS_DATA *) (packetbuf_dataptr() + sizeof(struct GROOT_HEADER) + sizeof(struct GROOT_QUERY));
	print_data(sns_data);
	printf("RE-PUBLISHING - ");
	PRINT2ADDR(&rimeaddr_node_addr);
	printf("\n");
	runicast_send(&glocal.channels->rc, &lst_itm->parent ,MAX_RETRANSMISSION);
}

static int
rcv_alterate(struct GROOT_HEADER *hdr, const rimeaddr_t *from){
	struct GROOT_QUERY_ITEM *lst_itm = NULL;
	struct GROOT_QUERY *qry_bdy = NULL;
	uint8_t is_serviced = 1;

	//Get Query from buffer
	qry_bdy = (struct GROOT_QUERY*) (packetbuf_dataptr() + sizeof(struct GROOT_HEADER));

	//Check that I have all the sensors needed
	if(!is_capable(&qry_bdy->sensors_required)){
		is_serviced = 0;
	}
	printf("IS SERVICED: %d \n", is_serviced);

	rimeaddr_copy(&hdr->received_from, from);

	//Already Saved
	lst_itm = find_query(hdr->query_id, &hdr->esender);
	if(lst_itm == NULL){
		//Add the new qry to the table
		lst_itm = qry_to_list(hdr, qry_bdy, from, is_serviced);
		if(broadcast_send(&glocal.channels->bc) == 0){
			return 0;
		}
		return 1;
	}

	memcpy(&lst_itm->query.sensors_required, &qry_bdy->sensors_required, sizeof(struct GROOT_SENSORS));
	lst_itm->query.sample_id = qry_bdy->sample_id;
	lst_itm->query.sample_rate = qry_bdy->sample_rate;
	lst_itm->query.aggregator = qry_bdy->aggregator;

	if(broadcast_send(&glocal.channels->bc) == 0){
		return 0;
	}
	return 1;
}

static int
rcv_new_mote(struct GROOT_HEADER *hdr){
	return 1;
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
groot_qry_snd(uint16_t query_id, uint8_t type, uint16_t sample_rate, struct GROOT_SENSORS *data_required, uint8_t aggregator){
	struct GROOT_HEADER hdr;
	struct GROOT_QUERY qry;

	packet_qry_creator(&hdr, &qry, query_id, &rimeaddr_node_addr, &rimeaddr_null,
		&rimeaddr_null, type, sample_rate, data_required, aggregator, 0);

	//Create Query Packet
	packet_loader_qry(&hdr, &qry, NULL);
	
	printf("SENDING!!! \n");
	//Send packet
	if(!broadcast_send(&glocal.channels->bc)){
		printf("BCAST - COULDN'T Be SENT! - ");
		PRINT2ADDR(&rimeaddr_node_addr);
		printf("\n");
		return 0;
	}
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
	rimeaddr_copy(&hdr.ereceiver, &rimeaddr_null);

	hdr.is_cluster_head = 0;
	hdr.type = GROOT_UNSUBSCRIBE_TYPE;
	hdr.query_id = query_id;

	packet_loader_qry(&hdr, NULL, NULL);

	broadcast_send(&glocal.channels->bc);
	return 1;
}

int
groot_rcv(const rimeaddr_t *from){
	struct GROOT_HEADER *hdr = NULL;
	uint8_t is_success = 0;

	hdr = (struct GROOT_HEADER*) packetbuf_dataptr();
	//I just sent this packet ignore
	if(rimeaddr_cmp(&hdr->received_from, &rimeaddr_node_addr) == 1){
		return is_success;
	}

	//Is not correct protocol
	if(!is_groot_protocol(hdr)){
		return is_success;
	}

	switch(hdr->type){
		case GROOT_SUBSCRIBE_TYPE:
			printf("SUBSCRIBING \n");
			is_success = rcv_subscribe(hdr, from);
			break;
		case GROOT_UNSUBSCRIBE_TYPE:
			//Do I have this query?
			printf("UNSUBSRIBING!! \n");
			is_success = rcv_unsubscribe(hdr, from);
			break;
		case GROOT_NEW_MOTE_TYPE:
			//New mote wants in! Check if query he can join!
			printf("Im a new fella!! \n");
			is_success = rcv_new_mote(hdr);
			break;
		case GROOT_ALTERATION_TYPE:
			printf("ALTERATION!! \n");
			is_success = rcv_alterate(hdr, from);
			
			break;
		case GROOT_PUBLISH_TYPE:
			//Publish Sensed data
			printf("RECEIVED PUBLISH DATA!! \n");
			is_success = rcv_publish(hdr);
			break;
	}
	return is_success;
}

void
groot_intent_snd(){

}

void
groot_intent_rcv(){

}