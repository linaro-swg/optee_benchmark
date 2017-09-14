CC      = $(CROSS_COMPILE)gcc
LD      = $(CROSS_COMPILE)ld
AR      = $(CROSS_COMPILE)ar
NM      = $(CROSS_COMPILE)nm
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump
READELF = $(CROSS_COMPILE)readelf

.PHONY: all
all: libyaml benchmark

.PHONY: clean
clean: libyaml-clean
	rm -f $(OBJS) benchmark
################################################################################
# libYAML
################################################################################
LIBYAML_FLAGS ?= CROSS_COMPILE=$(CROSS_COMPILE)
LIBYAML_SRC_DIR = $(CURDIR)/libyaml
LIBYAML_OUT_DIR = $(LIBYAML_SRC_DIR)/out

.PHONY: libyaml
libyaml:
	cd $(LIBYAML_SRC_DIR) && ./bootstrap && \
	./configure --host=$(MULTIARCH) \
	--prefix=$(LIBYAML_SRC_DIR)/out CC=$(CC) && \
	$(MAKE) && $(MAKE) install

.PHONY: libyaml-clean
libyaml-clean:
	cd $(LIBYAML_SRC_DIR) && \
	$(MAKE) clean

################################################################################
# benchmark_app
################################################################################
OBJS := main.o benchmark_aux.o

CFLAGS += -Wall -Wextra -Werror -I$(TEEC_EXPORT)/include \
		  -I$(TEEC_INTERNAL_INCLUDES)/include -I$(LIBYAML_OUT_DIR)/include
#Add/link other required libraries here
LDADD += -lm -lteec -lyaml -lpthread \
		 -L$(TEEC_EXPORT)/lib -L$(LIBYAML_OUT_DIR)/lib

benchmark: $(OBJS)
	$(CC) $(LDADD) -o $@ $^
