# ===== build config =====
CC        := cc
CFLAGS    := -O2 -Wall -Wextra -std=c11
INCLUDES  := -Iinclude -Icontroller -Iactuators -Isensors -I/usr/include/cjson
PKG       := $(shell pkg-config --cflags --libs libmosquitto)
LIBS      := $(PKG) -lcjson
PREFIX    := bin

# Ako si na starijem glibc pa kuka za clock_gettime:
# LIBS += -lrt

# ===== sources/objs =====
CONTROLLER_SRC    := controller/controller.c
CORE_NOMAIN_SRCS  := controller/registry.c actuators/actuators.c ssdp_dev.c
CORE_NOMAIN_OBJS  := $(CORE_NOMAIN_SRCS:.c=.o)

# kontroler: controller sa main-om
CTRL_MAIN_OBJ     := controller/controller.main.o
CTRL_MAIN_OBJS    := $(CTRL_MAIN_OBJ) $(CORE_NOMAIN_OBJS)

# simulator: controller bez main-a (BUILD_AS_LIB)
CTRL_LIB_OBJ      := controller/controller.lib.o
CTRL_LIB_OBJS     := $(CTRL_LIB_OBJ) $(CORE_NOMAIN_OBJS)

# senzorski CLI (ima svoj main)
SENSORS_SRCS      := sensors/sensors.c ssdp_dev.c
SENSORS_OBJS      := $(SENSORS_SRCS:.c=.o)

SIM_MAIN          := garage_sim.c

# ===== targets =====
all: $(PREFIX)/garage_controller $(PREFIX)/sensors_cli $(PREFIX)/garage_sim

$(PREFIX)/garage_controller: $(CTRL_MAIN_OBJS)
	@mkdir -p $(PREFIX)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $(CTRL_MAIN_OBJS) $(LIBS)

$(PREFIX)/sensors_cli: $(SENSORS_OBJS)
	@mkdir -p $(PREFIX)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $(SENSORS_OBJS) $(LIBS)

$(PREFIX)/garage_sim: $(CTRL_LIB_OBJS) $(SIM_MAIN)
	@mkdir -p $(PREFIX)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $(CTRL_LIB_OBJS) $(SIM_MAIN) $(LIBS)

# specijalna pravila: isti source, dva različita objekta
controller/controller.main.o: $(CONTROLLER_SRC)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

controller/controller.lib.o: $(CONTROLLER_SRC)
	$(CC) $(CFLAGS) $(INCLUDES) -DBUILD_AS_LIB -c $< -o $@

# generičko pravilo
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f controller/controller.main.o controller/controller.lib.o \
	      $(CORE_NOMAIN_OBJS) $(SENSORS_OBJS) \
	      $(PREFIX)/garage_controller $(PREFIX)/sensors_cli $(PREFIX)/garage_sim
.PHONY: all clean
