CC=gcc
CFLAGS=-O2 -Iinclude
LDFLAGS=-lmosquitto

SRC_CTRL=controller/controller.c controller/ssdp_ctrl.c controller/registry.c
BIN=bin/iot-controller

actuator: actuators/actuators.c sensors/ssdp_dev.c
	$(CC) $^ -o bin/actuator $(CFLAGS) $(LDFLAGS)

.PHONY: all clean

all: $(BIN)

$(BIN): $(SRC_CTRL)
	@mkdir -p bin
	$(CC) $(SRC_CTRL) -o $(BIN) $(CFLAGS) $(LDFLAGS)

clean:
	rm -rf bin
