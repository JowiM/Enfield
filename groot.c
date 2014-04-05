#include "contiki.h"
#include "groot.h"
#include <stdio.h>

static void
packet_loader_qry(GROOT_HEADER *hdr){
	packetbuf_clear();
	packetbuf_copyfrom(hdr, sizeof(hdr));
}

/*--------------------------------------------- Main Methods ------------------------------------------------------------*/
int
groot_subscribe_snd(struct broadcast_conn *dc, uint16_t query_id, uint16_t sample_rate, GROOT_SENSORS *data_required, uint8_t aggregator){
	GROOT_HEADER hdr;
	//Initialise query header
	hdr.protocol.version = GROOT_VERSION;
	hdr.protocol.magic[0] = 'G';
	hdr.protocol.magic[1] = 'T';
	rimeaddr_copy(&hdr.esender, &rimeaddr_node_addr);
	hdr.query_id = query_id;
	hdr.sample_rate = sample_rate;
	hdr.aggregator = aggregator;
	//@todo find better way
	hdr.sensors_required.co2 = data_required->co2;
	hdr.sensors_required.no = data_required->no;
	hdr.sensors_required.temp = data_required->temp;
	hdr.sensors_required.humidity = data_required->humidity;
	//Create Query Packet
	packet_loader_qry(&hdr);
	//Send packet
	broadcast_send(dc);
	return 0;
}

GROOT_QUERY_RW
*groot_subscribe_rcv(GROOT_QUERY_RW *first_query, struct broadcast_conn *c, const rimeaddr_t *from){
	GROOT_QUERY_RW rw;
	return &rw;
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