CONTIKI = ../..

#CONTIKI_PROJECT = enfield-sensor

# 1 = Success Only
# 2 = Print Q
# 3 = Verbose Mode

#CONTIKI_PROJECT = dtn
all: enfield-sensor enfield-sink

CONTIKI_SOURCEFILES += groot.c
CONTIKI_SOURCEFILES += groot-sensor.c
CONTIKI_SOURCEFILES += groot-sink.c

include $(CONTIKI)/Makefile.include

