.PHONY: build

VERSION	:= $(shell cat ./VERSION)
GIT_COMMIT := $(shell git rev-parse --short HEAD)
MANPATH := "/usr/share/man/man1"

ifeq (${RABBITMQC_HOME},)
	RABBITMQC_HOME := "/usr/local/src/rabbitmq/rabbitmq-c"
endif

ifeq (${QDECL_INSTALL_ROOT},)
	QDECL_INSTALL_ROOT := "/usr/local/bin"
endif

clean:
	@rm -f build/qdecl

install: build
	@install -D -m0755 build/qdecl $(QDECL_INSTALL_ROOT)/qdecl
	@mkdir -p $(MANPATH)
	@cp man/qdecl.1 $(MANPATH)

uninstall:
	@rm -f $(QDECL_INSTALL_ROOT)/qdecl

build:
	@gcc main.c \
		-Os -s \
		-o build/qdecl \
		-DGIT_COMMIT=\"$(GIT_COMMIT)\" \
		-DVERSION=\"$(VERSION)\" \
		-I$(RABBITMQC_HOME)/librabbitmq $(RABBITMQC_HOME)/librabbitmq/.libs/librabbitmq.so
