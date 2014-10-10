#TSDIR=/MyFile/ats_source/trafficserver-4.0.1/
TSDIR=/root/MyFile/ats_source/trafficserver-4.2.0
INC=-I $(TSDIR)/lib/ts
TARGET=cachecontrol
SRC=$(MAIN)

MAIN=-c cachecontrol.c -c ats_common.c -c common.c


all: $(TARGET)
install: $(TARGET)_install
clean:
	rm *.lo *~ $(TARGET).so
$(TARGET):
	tsxs -o $@.so $(SRC) $(INC)
$(TARGET)_install:
	tsxs -o $(TARGET).so -i
