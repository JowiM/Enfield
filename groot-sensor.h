/**
 * @file 
 * 	Header file for GROOT Middleware for sensors. A Semantic Routing Tree protocol to pass queries to nodes.
 * @author 
 * 		Johann Mifsud <johann.mifsud.13@ucl.ac.uk>
 */
#include "groot.h"

void
sensor_bootstrap(struct GROOT_SENSORS *support);

int
sensor_publish();

int
sensor_destroy();