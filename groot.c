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

static struct GROOT_SENSORS supported_sensors;
/*------------------------------------------------- Debug Methods -------------------------------------------------------*/
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

static void
packet_loader_qry(struct GROOT_HEADER *hdr, struct GROOT_QUERY *qry){
	packetbuf_clear();
	packetbuf_set_datalen(sizeof(struct GROOT_HEADER)+sizeof(struct GROOT_QUERY));
	
	struct GROOT_HEADER *pkt_hdr = packetbuf_dataptr();
	memcpy(pkt_hdr, hdr, sizeof(struct GROOT_HEADER));
	struct GROOT_QUERY *pkt_qry = (packetbuf_dataptr() + sizeof(struct GROOT_HEADER));
	memcpy(pkt_qry, qry, sizeof(struct GROOT_QUERY));
}

static void
qry_to_list(struct GROOT_HEADER *hdr, const rimeaddr_t *from){
	struct GROOT_QUERY_ITEM *new_item;
	struct GROOT_QUERY *qry_bdy;
	
	new_item = memb_alloc(&groot_qrys);
	list_add(groot_qry_table, new_item);

	new_item->query_id = hdr->query_id;
	rimeaddr_copy(&new_item->esender, &hdr->esender);
	rimeaddr_copy(&new_item->parent, from);
	//Is sender a cluster head?
	//@todo request to join cluster
	new_item->has_cluster_head = 0;
	//Never Sampled
	new_item->last_sampled = 0;
	//@todo add Children
			
	qry_bdy = (struct GROOT_QUERY*) (packetbuf_dataptr() + sizeof(struct GROOT_HEADER));

	//Copy Query Values into row
	memcpy(&new_item->query, qry_bdy, sizeof(struct GROOT_QUERY));

	print_qrys();
}

static uint8_t
is_new_query(uint16_t query_id, const rimeaddr_t *esender){
	struct GROOT_QUERY_ITEM *i;

	if(list_length(groot_qry_table) == 0){
		return 1;
	}

	i = list_head(groot_qry_table);
	while(i != NULL){
		if(i->query_id == query_id && rimeaddr_cmp(&i->esender, esender)){
			return 0;
		}
		i = i->next;
	}

	return 1;
}

/*--------------------------------------------- Main Methods ------------------------------------------------------------*/
void
groot_prot_init(struct GROOT_SENSORS *sensors){
	list_init(groot_qry_table);
	memb_init(&groot_qrys);
	//Copy Current Sensors
	memcpy(&supported_sensors, sensors, sizeof(struct GROOT_SENSORS));
}

int
groot_subscribe_snd(struct broadcast_conn *dc, uint16_t query_id, uint16_t sample_rate, struct GROOT_SENSORS *data_required, uint8_t aggregator){
	struct GROOT_HEADER hdr;
	struct GROOT_QUERY qry;

	//QRY BODY
	qry.sample_id = 0;
	qry.sample_rate = sample_rate;
	qry.aggregator = aggregator;
	memcpy(&qry.sensors_required, data_required, sizeof(struct GROOT_SENSORS));

	//Initialise query header
	hdr.protocol.version = GROOT_VERSION;
	hdr.protocol.magic[0] = 'G';
	hdr.protocol.magic[1] = 'T';
	
	//Main Variables
	rimeaddr_copy(&hdr.esender, &rimeaddr_node_addr);
	hdr.is_cluster_head = 0;
	hdr.type = GROOT_SUBSCRIBE_TYPE;
	hdr.query_id = query_id;

	//Create Query Packet
	packet_loader_qry(&hdr, &qry);
	
	printf("SENDING!!! \n");
	//Send packet
	broadcast_send(dc);
	return 0;
}

int
groot_subscribe_rcv(struct broadcast_conn *c, const rimeaddr_t *from){
	struct GROOT_HEADER *hdr;
	uint8_t type;

	hdr = (struct GROOT_HEADER*) packetbuf_dataptr();
	
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
			qry_to_list(hdr, from);
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