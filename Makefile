CC      = $(CROSS_COMPILE)gcc
LD      = $(CROSS_COMPILE)ld
AR      = $(CROSS_COMPILE)ar
NM      = $(CROSS_COMPILE)nm
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump
READELF = $(CROSS_COMPILE)readelf

OBJS := main.o benchmark_aux.o

CFLAGS += -Wall -Wextra -Werror -I$(TEEC_EXPORT)/include -I$(TEEC_INTERNAL_INCLUDES)/include
#Add/link other required libraries here
LDADD += -lm -lteec -lpthread -L$(TEEC_EXPORT)/lib

BINARY = benchmark

.PHONY: all
all: $(BINARY)

$(BINARY): $(OBJS)
	$(CC) $(LDADD) -o $@ $^

.PHONY: clean
clean:
	rm -f $(OBJS) $(BINARY)
