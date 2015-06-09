.PHONY: build

VERSION	:= $(shell cat ./VERSION)
GIT_COMMIT := $(shell git rev-parse --short HEAD)

ifeq (${RABBITMQC_HOME},)
	RABBITMQC_HOME = "/usr/local/src/rabbitmq/rabbitmq-c"
endif

ifeq (${QDECL_INSTALL_ROOT},)
	QDECL_INSTALL_ROOT := "/usr/local/bin"
endif

clean:
	@rm -f build/qdecl

build:
	@gcc main.c \
		-o build/qdecl \
		-DGIT_COMMIT=\"$(GIT_COMMIT)\" \
		-DVERSION=\"$(VERSION)\" \
		-I$(RABBITMQC_HOME)/librabbitmq $(RABBITMQC_HOME)/librabbitmq/.libs/librabbitmq.so
	@chmod a+x build/qdecl

install:
	@echo $(QDECL_INSTALL_ROOT)
