.PHONY: build

VERSION	:= $(shell cat ./VERSION)
GIT_COMMIT := $(shell git rev-parse --short HEAD)

# cmake -DBUILD_STATIC_LIBS=ON -DBUILD_EXAMPLES=OFF -DBUILD_TOOLS_DOCS=OFF -DBUILD_TOOLS=OFF ..
# cmake --build .
ifeq (${RABBITMQC_HOME},)
	RABBITMQC_HOME = "/usr/local/src/rabbitmq/rabbitmq-c"
endif

ifeq (${QDECL_INSTALL_ROOT},)
	QDECL_INSTALL_ROOT := "/usr/local/bin"
endif

clean:
	@rm -f build/qdecl

install: build
	@install -D -m0755 build/qdecl $(QDECL_INSTALL_ROOT)/qdecl

uninstall:
	@rm -f $(QDECL_INSTALL_ROOT)/qdecl

build-static:
	# glibc `--enable-static-nss`
	@gcc -L$(RABBITMQC_HOME)/librabbitmq/.libs \
		-static main.c \
		-o build/qdecl \
		-DGIT_COMMIT=\"$(GIT_COMMIT)\" \
		-DVERSION=\"$(VERSION)\" \
		-Wl,-Bstatic \
		-L/usr/lib/gcc/x86_64-redhat-linux/4.8.3 \
		-L$(RABBITMQC_HOME)/build/librabbitmq \
		-L. \
		-I$(RABBITMQC_HOME)/librabbitmq \
		-lrabbitmq

build:
	@gcc main.c \
		-o build/qdecl \
		-DGIT_COMMIT=\"$(GIT_COMMIT)\" \
		-DVERSION=\"$(VERSION)\" \
		-I$(RABBITMQC_HOME)/librabbitmq $(RABBITMQC_HOME)/librabbitmq/.libs/librabbitmq.so
