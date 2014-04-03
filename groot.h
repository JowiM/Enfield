/**
 * @file
 * 	Header file for GROOT. Holds most data strucutures needed.
 * @author 
 * 	Johann Mifsud <johann.mifsud.13@ucl.ac.uk>
 */

#include "net/rime.h"

#define GROOT_VERSION 1;
#define DEBUG_LEVEL 0;

typedef struct {
	uint8_t version;
	uint8_t magic[2];
} groot_header_protocol;

typedef struct {
	uint8_t co2;
	uint8_t no;
	uint8_t temp;
	uint8_t humidity;
} groot_toc;

typedef struct {
	groot_header_protocol protocol;
	rimeaddr_t esender;
	rimeaddr_t ereceiver;
	uint16_t queryid;
	groot_toc payload_table;
} groot_header;