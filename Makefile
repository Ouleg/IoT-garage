APP=garage_ctrl

SRC=apps/cli.c \
    controller/controller.c \
    controller/registry.c \
    actuators/actuators.c \
    sensors/sensors.c \
    ssdp_dev.c

INC=include controller actuators sensors
CFLAGS=-O2 -Wall $(addprefix -I,$(INC))

CFLAGS  := -O2 -Wall -Iinclude -Icontroller -Iactuators -Isensors
CFLAGS  += $(shell pkg-config --cflags libmosquitto)
CFLAGS  += -I/usr/include/cjson      # <-- OVO DODAJ
LDFLAGS := $(shell pkg-config --libs libmosquitto) -lcjson


LIBS=`pkg-config --libs --cflags libmosquitto` -lcjson

all: $(APP)
$(APP): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f $(APP)
