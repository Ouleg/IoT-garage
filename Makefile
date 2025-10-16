CC := cc
CFLAGS := -O2 -Wall -pthread
INCLUDES := -Iinclude -I/usr/include/cjson
PKG := $(shell pkg-config --cflags --libs libmosquitto)
LIBS := -lcjson

BIN := bin
CTRL := $(BIN)/garage_controller
SENS := $(BIN)/sensors
SIM  := $(BIN)/garage_sim

all: $(CTRL) $(SENS) $(SIM)

$(BIN):
	mkdir -p $(BIN)

# Kontroler (NE uključuje sensors.c)
$(CTRL): controller/controller.c actuators/actuators.c | $(BIN)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(PKG) $(LIBS)

# Senzori (sensors.c već ima main kad NEMA BUILD_AS_LIB)
$(SENS): sensors/sensors.c ssdp_dev.c | $(BIN)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(PKG) $(LIBS)

# Simulator (one-click scenarijo: senzori + echo state)
$(SIM): garage_sim.c | $(BIN)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(PKG) $(LIBS)

clean:
	rm -rf $(BIN)

.PHONY: all clean
