#include "contiki.h"
#include "groot.h"
#include "lib/list.h"
#include "lib/memb.h"
#include "stdio.h"
#include "string.h"

#define PRINT2ADDR(addr) printf("%02x%02x", (addr)->u8[1], (addr)->u8[0])

LIST(groot_qry_table);
MEMB(groot_qrys, struct GROOT_QUERY_ITEM, GROOT_QUERY_LIMIT);
MEMB(groot_children, struct GROOT_SRT_CHILD, GROOT_QUERY_LIMIT*GROOT_CHILD_LIMIT);

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
		printf(" ] \n");

		qry_itm = qry_itm->next;
	}
}

static void
print_children(struct GROOT_SRT_CHILD *first_child){
	struct GROOT_SRT_CHILD *tmp_child = first_child;

	while(tmp_child != NULL){
		printf("CHILD ");
		PRINT2ADDR(&tmp_child->address);
		printf(" {Last set: %d }\n", tmp_child->last_set);

		tmp_child = tmp_child->next;
	}
}

/*------------------------------------------------- Other Methods ----------------------------------------------------------*/
static void
cpy_hdr(struct GROOT_HEADER *target, struct GROOT_HEADER *src){
	memcpy(&target->protocol, &src->protocol, sizeof(struct GROOT_HEADER*));
	rimeaddr_copy(&target->to, &src->to);
	rimeaddr_copy(&target->ereceiver, &src->ereceiver);
	rimeaddr_copy(&target->received_from, &src->received_from);
	target->is_cluster_head = src->is_cluster_head;
	target->type = src->type;
	target->query_id = src->query_id;

	print_raw_packetbuf();
}

static void
copy_qry(struct GROOT_QUERY *target, struct GROOT_QUERY *src){
	target->sample_id = src->sample_id;
	target->sample_rate = src->sample_rate;
	target->aggregator = src->aggregator;
	memcpy(&target->sensors_required, &src->sensors_required, sizeof(struct GROOT_SENSORS));
}

static struct GROOT_QUERY
*packetbuf_get_qry(){
	return (struct GROOT_QUERY*) (packetbuf_dataptr() + sizeof(struct GROOT_HEADER));
}

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

static struct GROOT_SRT_CHILD
*get_child(struct GROOT_SRT_CHILD *first_child, const rimeaddr_t *address){
	struct GROOT_SRT_CHILD *tmp_child = first_child;

	while(tmp_child != NULL){
		if(rimeaddr_cmp(&tmp_child->address, address)){
			return tmp_child;
		}

		tmp_child = tmp_child->next;
	}

	return NULL;
}

static int
child_length(struct GROOT_SRT_CHILD *first_child){
	struct GROOT_SRT_CHILD *tmp_children = first_child;
	int numb_childs = 0;

	while(tmp_children != NULL){
		numb_childs += 1;
		tmp_children = tmp_children->next;
	}

	return numb_childs;
}

static struct GROOT_SRT_CHILD
*child_lst_tail(struct GROOT_SRT_CHILD *first_child){
	struct GROOT_SRT_CHILD *tmp_children = first_child;

	while(tmp_children != NULL){
		if(tmp_children->next == NULL){
			return tmp_children;
		}
		tmp_children = tmp_children->next;
	}

	return NULL;
}

static struct GROOT_SRT_CHILD
*add_child(struct GROOT_SRT_CHILD *lst_child, struct GROOT_SRT_CHILD *new_child){
	struct GROOT_SRT_CHILD *tmp_child = child_lst_tail(lst_child);
	if(tmp_child == NULL){
		return NULL;
	}

	tmp_child->next = new_child;
	return new_child;
}

rm_child(){

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
	struct GROOT_SENSORS *data_required, uint8_t aggregator, uint8_t is_cluster_head, uint16_t sample_id){

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
	if(is_capable(data_required) == 0){
		hdr->is_cluster_head = 0;
	} else {
		hdr->is_cluster_head = 1;
	}
	hdr->type = type;
	hdr->query_id = query_id;
}

static void
packet_loader_qry(struct GROOT_HEADER *hdr, struct GROOT_QUERY *qry, struct GROOT_SENSORS_DATA *sensors_data){
	int data_l = sizeof(struct GROOT_HEADER);
	struct GROOT_QUERY *pkt_qry = NULL;
	struct GROOT_SENSORS_DATA *pkt_data = NULL;

	//Adding query?
	if(qry != NULL){
		data_l += sizeof(struct GROOT_QUERY);
	}
	//Adding sensor data?
	if(sensors_data != NULL){
		data_l += sizeof(struct GROOT_SENSORS_DATA);
	}
	//Clean buffer and set length
	packetbuf_clear();
	packetbuf_set_datalen(data_l);

	//Copy hdr in buffer
	struct GROOT_HEADER *pkt_hdr = packetbuf_dataptr();
	memcpy(pkt_hdr, hdr, sizeof(struct GROOT_HEADER));
	//If need be copy qry
	if(qry != NULL){
		pkt_qry = (packetbuf_dataptr() + sizeof(struct GROOT_HEADER));
		memcpy(pkt_qry, qry, sizeof(struct GROOT_QUERY));
	}
	//if need be copy sensor data
	if(sensors_data != NULL){
		if(qry == NULL){
			pkt_data = (packetbuf_dataptr()+sizeof(struct GROOT_HEADER));
		} else {
			pkt_data = (packetbuf_dataptr()+sizeof(struct GROOT_HEADER)+sizeof(struct GROOT_QUERY));
		}
		memcpy(pkt_data, sensors_data, sizeof(struct GROOT_SENSORS_DATA));
	}
}

static int
get_sensor_data(uint8_t sensor, struct GROOT_SENSORS_DATA *data){
	int saved = 0;
	switch(sensor){
		case SENSOR_CO2:
			saved = data->co2;
			break;
		case SENSOR_NO:
			saved = data->no;
			break;
		case SENSOR_HUMIDITY:
			saved = data->humidity;
			break;
		case SENSOR_TEMP:
			saved = data->temp;
			break;
	}

	return saved;
}

static int
aggregate_calc(struct GROOT_SRT_CHILD *children, uint8_t sensor, uint8_t aggregator){
	struct GROOT_SRT_CHILD *tmp_child;
	uint16_t result, tmp_data, tmp_result, count = 0;

	switch(aggregator){
		case GROOT_MAX:
			tmp_child = children;
			while(tmp_child != NULL){
				tmp_data = get_sensor_data(sensor, &tmp_child->data);	
				if(tmp_result < tmp_data){
					tmp_result = tmp_data;
				}
				tmp_child = tmp_child->next;
			}
			result = tmp_result;
			break;
		case GROOT_AVG:
			tmp_child = children;
			while(tmp_child != NULL){
				tmp_result += get_sensor_data(sensor, &tmp_child->data);
				count += 1;
				tmp_child = tmp_child->next;
			}
			result = tmp_result / count;
			break;
		case GROOT_MIN:
			tmp_child = children;
			while(tmp_child != NULL){
				tmp_data = get_sensor_data(sensor, &tmp_child->data);
				if(tmp_result > tmp_data){
					tmp_result = tmp_data;
				}
				result = tmp_result;
				tmp_child = tmp_child->next;
			}
			break;
	}

	return result;
}


static void
send_sample(struct GROOT_QUERY_ITEM *qry_itm, struct GROOT_SENSORS_DATA *sensors_data){
	struct GROOT_HEADER hdr;
	struct GROOT_QUERY qry;

	hdr.protocol.version = GROOT_VERSION;
	hdr.protocol.magic[0] = 'G';
	hdr.protocol.magic[1] = 'T';
	rimeaddr_copy(&hdr.to, &qry_itm->parent);
	rimeaddr_copy(&hdr.ereceiver, &qry_itm->esender);
	rimeaddr_copy(&hdr.esender, &rimeaddr_node_addr);
	rimeaddr_copy(&hdr.received_from, &rimeaddr_null);

	hdr.is_cluster_head = qry_itm->parent_is_cluster;
	hdr.type = GROOT_PUBLISH_TYPE;
	hdr.query_id = qry_itm->query_id;

	//Increment Sample Id
	qry_itm->query.sample_id += 1;
	memcpy(&qry, &qry_itm->query, sizeof(struct GROOT_QUERY));
	
	PRINT2ADDR(&rimeaddr_node_addr);
	printf("- { SENDING SAMPLE %d - QID - %d }\n", qry.sample_id, qry_itm->query_id);

	packet_loader_qry(&hdr, &qry, sensors_data);
	broadcast_send(&glocal.channels->bc);
}

static void
cb_publish_aggregate(void *i){
	struct GROOT_QUERY_ITEM *lst_itm = (struct GROOT_QUERY_ITEM *)i;
	struct GROOT_SRT_CHILD *tmp_child = lst_itm->children;
	struct GROOT_SENSORS_DATA data;
	struct GROOT_SENSORS *req = NULL;

	while(tmp_child != NULL){
		if(tmp_child->last_set <= clock_seconds()-(lst_itm->query.sample_rate+2) &&
			lst_itm->agg_passes < 3 && ctimer_expired(&lst_itm->maintainer_t))
		{
			//Increment Passes
			lst_itm->agg_passes += 1;
			ctimer_set(&lst_itm->maintainer_t, (rand()%(1*CLOCK_SECOND)), cb_publish_aggregate, lst_itm);
			return;
		}
		tmp_child = tmp_child->next;
	}
	
	//Sending so aggregation pass is done!
	lst_itm->agg_passes = 0;
	req = &lst_itm->query.sensors_required;
	if(req->co2 == 1){
		data.co2 = aggregate_calc(lst_itm->children, SENSOR_CO2, lst_itm->query.aggregator);
	}

	if(req->no == 1){
		data.no = aggregate_calc(lst_itm->children, SENSOR_NO, lst_itm->query.aggregator);
	}

	if(req->temp == 1){
		data.temp = aggregate_calc(lst_itm->children, SENSOR_TEMP, lst_itm->query.aggregator);
	}

	if(req->humidity == 1){
		data.humidity = aggregate_calc(lst_itm->children, SENSOR_HUMIDITY, lst_itm->query.aggregator);
	}
	//Send the data
	send_sample(lst_itm, &data);
}

static void
cb_sampler(void *i){
	struct GROOT_QUERY_ITEM *qry_itm = (struct GROOT_QUERY_ITEM *)i;
	struct GROOT_SENSORS_DATA sensors_data;

	//Get Sensor readings - in this case random numbers due to the use of a simulator
	random_sensor_readings(&qry_itm->query.sensors_required, &sensors_data);
	
	//Send the data
	if(qry_itm->query.aggregator == GROOT_NO_AGGREGATION){
		send_sample(qry_itm, &sensors_data);
	} else {
		//@todo where do I store this data???
		////Initialize ctimer to check if all children arrived or check with every sample
		if(ctimer_expired(&qry_itm->maintainer_t)){
			ctimer_set(&qry_itm->maintainer_t, (rand()%(1*CLOCK_SECOND)), cb_publish_aggregate, qry_itm);
		}
	}

	//Check if timer is being used by someone else
	if(ctimer_expired(&qry_itm->query_timer)){
		ctimer_set(&qry_itm->query_timer, qry_itm->query.sample_rate, cb_sampler, qry_itm);
	}
}

static void
cluster_join_send(void *lst_item){
	printf("JOIN SEND");
	struct GROOT_HEADER hdr;
	struct GROOT_QUERY_ITEM *itm = (struct GROOT_QUERY_ITEM *)lst_item;

	hdr.protocol.version = GROOT_VERSION;
	hdr.protocol.magic[0] = 'G';
	hdr.protocol.magic[1] = 'T';

	rimeaddr_copy(&hdr.to, &rimeaddr_null);
	rimeaddr_copy(&hdr.esender, &itm->esender);
	rimeaddr_copy(&hdr.ereceiver, &rimeaddr_null);
	rimeaddr_copy(&hdr.received_from, &rimeaddr_null);

	hdr.is_cluster_head = 1;
	hdr.type = GROOT_CLUSTER_JOIN_TYPE;
	hdr.query_id = itm->query_id;

	packet_loader_qry(&hdr, NULL, NULL);
	runicast_send(&glocal.channels->rc, &itm->parent, MAX_RETRANSMISSION);
}

static struct  GROOT_QUERY_ITEM
*qry_to_list(struct GROOT_HEADER *hdr, struct GROOT_QUERY *qry_bdy, const rimeaddr_t *from){
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
	new_item->parent_is_cluster = hdr->is_cluster_head;
	rimeaddr_copy(&new_item->parent_bkup, &rimeaddr_null);
	//Check that I have all the sensors needed
	new_item->is_serviced = is_capable(&qry_bdy->sensors_required);
	//When unsubscribe received set this time
	new_item->unsubscribed = 0;
	//Copy Query Values into row
	copy_qry(&new_item->query, qry_bdy);
	
	new_item->children = NULL;
	print_qrys();
	return new_item;
}

static struct GROOT_QUERY_ITEM
*find_query(uint16_t query_id, const rimeaddr_t *esender){
	struct GROOT_QUERY_ITEM *i;
	//If empty return
	if(list_length(groot_qry_table) == 0){
		return NULL;
	}
	//Get top of head a pass through all queries to find query
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
	struct GROOT_QUERY *qry_bdy;	
	struct GROOT_QUERY_ITEM *lst_itm = NULL;
	struct GROOT_QUERY snd_bdy;
	struct GROOT_HEADER snd_hdr;
	
	//Already Saved
	lst_itm = find_query(hdr->query_id, &hdr->esender);
	if(lst_itm != NULL){
		printf("QUERY ALREADY SAVED! \n");
		//Set Bkup connection if NULL
		if(rimeaddr_cmp(&lst_itm->parent_bkup, &rimeaddr_null) > 0){
			rimeaddr_copy(&lst_itm->parent_bkup, from);
		}
		return 0;
	}

	//Get Query from buffer
	qry_bdy = packetbuf_get_qry();
	
	//Add the new qry to the table
	lst_itm = qry_to_list(hdr, qry_bdy, from);
	//NOT added to the table do nothing. FOLLOWS ASSUMPTION ALL QUERIES WILL BE SAVED
	if(lst_itm == NULL){
		return 0;
	}
	//Re-Broadcast Query so next neighbours can see it.
	packet_qry_creator(&snd_hdr, &snd_bdy, lst_itm->query_id, &lst_itm->esender, &rimeaddr_null, from, GROOT_SUBSCRIBE_TYPE,
		lst_itm->query.sample_rate, &lst_itm->query.sensors_required, lst_itm->query.aggregator, lst_itm->is_serviced, lst_itm->query.sample_id);

	//Create Query Packet
	packet_loader_qry(&snd_hdr, &snd_bdy, NULL);
	
	PRINT2ADDR(&rimeaddr_node_addr);
	printf("- { RE-SENDING SUBSRCIBE }\n");
	
	//Send packet
	broadcast_send(&glocal.channels->bc);

	//If the query is serviced by this node create callback function to send samples
	if(lst_itm->is_serviced == 1){
		//Timer for sampling
		ctimer_set(&lst_itm->query_timer, qry_bdy->sample_rate+(rand()%(1*CLOCK_SECOND)), cb_sampler, lst_itm);
	}

	if(lst_itm->parent_is_cluster == 1){
		//Timer to send out join cluster
		ctimer_set(&lst_itm->maintainer_t, rand()%(1*CLOCK_SECOND), cluster_join_send, lst_itm);
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

	//Keep the query in the file for a few seconds to stop rebroadcasting
	lst_itm->unsubscribed = clock_seconds();
	ctimer_set(&lst_itm->maintainer_t, GROOT_RM_UNSUBSCRIBE, cb_rm_query, lst_itm);			
	//Update from value
	rimeaddr_copy(&hdr->received_from, from);
	
	PRINT2ADDR(&rimeaddr_node_addr);
	printf("- { RE-SENDING UNSUBSRIBE }\n");

	if(broadcast_send(&glocal.channels->bc) == 0){
		return 0;
	}
	return 1;
}

static int
rcv_publish(struct GROOT_HEADER *hdr, const rimeaddr_t *from){
	struct GROOT_QUERY_ITEM *lst_itm = NULL;
	struct GROOT_SENSORS_DATA *sns_data = NULL;
	struct GROOT_SRT_CHILD *child = NULL;
	struct GROOT_QUERY *qry_bdy = NULL;
	
	if(rimeaddr_cmp(&hdr->ereceiver, &rimeaddr_node_addr) > 0){
		printf("RECEIVED DATA SUCCESS!!! \n");
		print_hdr((struct GROOT_HEADER*)packetbuf_dataptr());
		printf("AWSOME!! \n");
		return 0;
	}

	//Sink can only accept Data
	if(glocal.is_sink == 1){
		return 0;
	}

	PRINT2ADDR(&hdr->to);
	printf(" <--to PUBLISH \n");
/*
	//Not for me!
	//@todo please clean this code :'(. Its ugly as hell!!
	//@todo if null still send if query exists
	if(rimeaddr_cmp(&hdr->to, &rimeaddr_node_addr) == 0){
		printf("NOT FOR ME!! \n");
		PRINT2ADDR(&rimeaddr_node_addr);
		//Check if I have query. If not add!
		if(find_query(hdr->query_id, &hdr->esender) == NULL){
			printf("I do not own the query!! Adding \n");
			//Get Query from buffer
			qry_bdy = (struct GROOT_QUERY*) (packetbuf_dataptr() + sizeof(struct GROOT_HEADER));
			//Add the new qry to the table
			lst_itm = qry_to_list(hdr, qry_bdy, from);
			if(lst_itm != NULL && lst_itm->is_serviced == 1){
				printf("Join in! \n");
				//Timer for sampling
				ctimer_set(&lst_itm->query_timer, qry_bdy->sample_rate+(rand()%(1*CLOCK_SECOND)), cb_sampler, lst_itm);
				ctimer_set(&lst_itm->maintainer_t, (rand()%(1*CLOCK_SECOND)), cluster_join_send, lst_itm);	
			}
		}
		return 0;
	}

	lst_itm = find_query(hdr->query_id, &hdr->ereceiver);
	if(lst_itm == NULL){
		return 0;
	}

	if(lst_itm->query.aggregator == GROOT_NO_AGGREGATION){
		rimeaddr_copy(&hdr->to, &lst_itm->parent);
		broadcast_send(&glocal.channels->bc);
	}

	sns_data = (struct GROOT_SENSORS_DATA *) (packetbuf_dataptr() + sizeof(struct GROOT_HEADER) + sizeof(struct GROOT_QUERY));
	child = get_child(lst_itm->children, &hdr->ereceiver);
	if(child != NULL){
		memcpy(&child->data, sns_data, sizeof(struct GROOT_SENSORS_DATA));
		child->last_set = clock_seconds();
	}

	//@todo DELETE
	print_data(sns_data);
	printf("RE-PUBLISHING - ");
	PRINT2ADDR(&rimeaddr_node_addr);
	printf("\n");
	print_children(lst_itm->children);*/
}

/*static int
rcv_alterate(struct GROOT_HEADER *hdr, const rimeaddr_t *from){
	struct GROOT_QUERY_ITEM *lst_itm = NULL;
	struct GROOT_QUERY *qry_bdy = NULL;
	
	//Get Query from buffer
	qry_bdy = (struct GROOT_QUERY*) (packetbuf_dataptr() + sizeof(struct GROOT_HEADER));
	//Changed Packet Received from
	rimeaddr_copy(&hdr->received_from, from);

	//Already Saved
	lst_itm = find_query(hdr->query_id, &hdr->esender);
	if(lst_itm == NULL){
		//Add the new qry to the table
		lst_itm = qry_to_list(hdr, qry_bdy, from);
	} else {
		//Update Query details
		memcpy(&lst_itm->query.sensors_required, &qry_bdy->sensors_required, sizeof(struct GROOT_SENSORS));
		lst_itm->query.sample_id = qry_bdy->sample_id;
		lst_itm->query.sample_rate = qry_bdy->sample_rate;
		lst_itm->query.aggregator = qry_bdy->aggregator;
		lst_itm->is_serviced = is_capable(&lst_itm->query.sensors_required);
	}

	//If the query is serviced by this node create callback function to send samples
	if(lst_itm->is_serviced > 0){
		ctimer_set(&lst_itm->query_timer, qry_bdy->sample_rate+(rand()%(1*CLOCK_SECOND)), cb_sampler, lst_itm);
	}

	if(broadcast_send(&glocal.channels->bc) == 0){
		return 0;
	}
	return 1;
}*/

static int
rcv_cluster_join(struct GROOT_HEADER *hdr, const rimeaddr_t *from){
	struct GROOT_QUERY_ITEM *lst_itm = NULL;
	struct GROOT_SRT_CHILD *child;
	printf("RECEIVED JOIN REQ! \n");

	lst_itm = find_query(hdr->query_id, &hdr->esender);
	//Item not in table or cannot accept more children
	if(lst_itm == NULL || 
		(child_length(lst_itm->children) >= GROOT_CHILD_LIMIT)){
		return 0;
	}

	//Get Child memory
	child = memb_alloc(&groot_children);
	rimeaddr_copy(&child->address, from);
	child->last_set = 0;
	child->next = NULL;
	//Add Child
	if(lst_itm->children == NULL){
		lst_itm->children = child;
	} else {
		add_child(lst_itm->children, child);
	}

	return 1;
}
/*--------------------------------------------- Main Methods ------------------------------------------------------------*/
void
groot_prot_init(struct GROOT_SENSORS *sensors, struct GROOT_CHANNELS *channels, uint8_t is_sink){
	//Initialise data structures
	list_init(groot_qry_table);
	memb_init(&groot_qrys);
	memb_init(&groot_children);

	//Copy Current Sensors
	memcpy(&glocal.sensors, sensors, sizeof(struct GROOT_SENSORS));
	//Set local channels
	glocal.channels = channels;
	glocal.is_sink = is_sink;
}

int
groot_qry_snd(uint16_t query_id, uint8_t type, uint16_t sample_rate, struct GROOT_SENSORS *data_required, uint8_t aggregator){
	struct GROOT_HEADER hdr;
	struct GROOT_QUERY qry;

	printf("SUBSRUBE SEND CALLING CREATOR: \n");
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
	rimeaddr_copy(&hdr.ereceiver, &rimeaddr_null);
	rimeaddr_copy(&hdr.received_from, &rimeaddr_null);
	hdr.is_cluster_head = 0;
	hdr.type = GROOT_SUBSCRIBE_TYPE;
	hdr.query_id = query_id;

	//Create Query Packet
	packet_loader_qry(&hdr, &qry, NULL);
	
	PRINT2ADDR(&rimeaddr_node_addr);
	printf("- { SENDING QUERY }\n");

	//Send packet
	if(!broadcast_send(&glocal.channels->bc)){
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

	rimeaddr_copy(&hdr.to, &rimeaddr_null);
	rimeaddr_copy(&hdr.received_from, &rimeaddr_null);
	rimeaddr_copy(&hdr.esender, &rimeaddr_node_addr);
	rimeaddr_copy(&hdr.ereceiver, &rimeaddr_null);

	hdr.is_cluster_head = 0;
	hdr.type = GROOT_UNSUBSCRIBE_TYPE;
	hdr.query_id = query_id;

	PRINT2ADDR(&rimeaddr_node_addr);
	printf("- { SENDING UNSUBSRIBE }\n");

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
	printf("RECEIVED PKT - %02x \n", hdr->type);
	if(rimeaddr_cmp(&hdr->received_from, &rimeaddr_node_addr) == 1){
		return is_success;
	}

	//Is not correct protocol
	if(!is_groot_protocol(hdr)){
		return is_success;
	}

	PRINT2ADDR(&rimeaddr_node_addr);
	printf("- { RECEIVED - %02x }\n", hdr->type);

	if(glocal.is_sink == 0){
		printf("IS NOT SINK!!");
		switch(hdr->type){
			case GROOT_SUBSCRIBE_TYPE:
				printf("SUBSCRIBING \n");
				is_success = rcv_subscribe(hdr, from);
				break;
			/*case GROOT_UNSUBSCRIBE_TYPE:
				//Do I have this query?
				printf("UNSUBSRIBING!! \n");
				is_success = rcv_unsubscribe(&hdr, from);
				break;
			case GROOT_ALTERATION_TYPE:
				printf("ALTERATION!! \n");
				is_success = rcv_alterate(&hdr, from);
				break;
			case GROOT_CLUSTER_JOIN_TYPE:
				printf("JOIN CLUSER!! \n");
				is_success = rcv_cluster_join(&hdr, from);
				break;*/
		}
	} 

	//Both Sink and Sensor have this functionality
	if(hdr->type == GROOT_PUBLISH_TYPE){
		//Publish Sensed data
		is_success = rcv_publish(hdr, from);
	}
	return is_success;
}