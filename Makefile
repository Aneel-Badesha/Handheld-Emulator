PORT ?= COM4
TARGET ?= esp32s3

.PHONY: all build flash monitor flash-monitor clean menuconfig set-target

all: build

build:
	idf.py build

flash:
	idf.py -p $(PORT) flash

monitor:
	idf.py -p $(PORT) monitor

flash-monitor:
	idf.py -p $(PORT) flash monitor

clean:
	idf.py fullclean

menuconfig:
	idf.py menuconfig

set-target:
	idf.py set-target $(TARGET)
