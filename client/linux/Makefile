
ARCH = x64
SUPPORT_MULTHREAD = support

getconf_file = /usr/bin/getconf
exist = $(shell if [ -f $(getconf_file) ]; then echo "exist"; else echo "not exist"; fi;)

ifeq ($(exist),exist)

ifeq ($(shell getconf LONG_BIT), 32)
    ARCH = x86
else
    ARCH = x64
endif

else
	ARCH = MIPS_32
endif

$(info  "file getconf >>> $(exist)")
$(info  "support mutilthread >>> $(SUPPORT_MULTHREAD)")
$(info  "cpu bits >>> $(ARCH)")


CPPFLAGS  =   -ggdb -DCORE_DUMP -pthread -Wall
CPPFLAGS += -fno-strict-aliasing -D__STDC_FORMAT_MACROS


PUBLIC_DIR=../../public

CPPFLAGS += -I./
CPPFLAGS += -I../remote/
CPPFLAGS += -I../
CPPFLAGS += -I$(PUBLIC_DIR)/socks5lib/
CPPFLAGS += -I$(PUBLIC_DIR)/xutil/include
CPPFLAGS += -I$(PUBLIC_DIR)/netpool/include
CPPFLAGS += -I$(PUBLIC_DIR)/xjson/src/

ifeq ($(ARCH), x86)
	CPPFLAGS += -I$(PUBLIC_DIR)/curl/32/include/
else
	CPPFLAGS += -I$(PUBLIC_DIR)/curl/64/include/
endif


LDFLAGS += -lpthread

ifeq ($(ARCH),x86)
CPPFLAGS += -I$(PUBLIC_DIR)/curl/32/include/
LDFLAGS += -L$(PUBLIC_DIR)/curl/32/lib -lcurl
LDFLAGS += -L$(PUBLIC_DIR)/xjson/32/lib -lxjson

LDFLAGS += -L$(PUBLIC_DIR)/netpool/32/lib -lnetpool
LDFLAGS += -L$(PUBLIC_DIR)/xutil/32/lib -lxutil
else ifeq ($(ARCH),x64)
CPPFLAGS += -I$(PUBLIC_DIR)/curl/64/include/
LDFLAGS += -L$(PUBLIC_DIR)/curl/64/lib -lcurl
LDFLAGS += -L$(PUBLIC_DIR)/xjson/64/lib -lxjson

LDFLAGS += -L$(PUBLIC_DIR)/netpool/64/lib -lnetpool
LDFLAGS += -L$(PUBLIC_DIR)/xutil/64/lib -lxutil
else
CPPFLAGS += -I$(PUBLIC_DIR)/curl/mips_32/include/
LDFLAGS += -L$(PUBLIC_DIR)/curl/mips_32/lib -lcurl
LDFLAGS += -L$(PUBLIC_DIR)/xjson/mips_32/lib -lxjson

LDFLAGS += -L$(PUBLIC_DIR)/netpool/mips_32/lib -lnetpool
LDFLAGS += -L$(PUBLIC_DIR)/xutil/mips_32/lib -lxutil
endif

ifeq ($(SUPPORT_MULTHREAD), not-support)

else
CPPFLAGS += -D_SUPPORT_MULTITHRAD
ifeq ($(ARCH), x86)
	CPPFLAGS += -I$(PUBLIC_DIR)/openssl/32/include/
	LDFLAGS += -L$(PUBLIC_DIR)/openssl/32/lib -lssl
	LDFLAGS += -L$(PUBLIC_DIR)/openssl/32/lib -lcrypto
else ifeq ($(ARCH),x64)
	CPPFLAGS += -I$(PUBLIC_DIR)/openssl/64/include/
	LDFLAGS += -L$(PUBLIC_DIR)/openssl/64/lib -lssl
	LDFLAGS += -L$(PUBLIC_DIR)/openssl/64/lib -lcrypto
endif

endif


SOURCES = $(wildcard ./*.cpp)
SOURCES += $(wildcard ../*.cpp)
SOURCES += $(wildcard ../remote/*.cpp)
SOURCES += $(wildcard $(PUBLIC_DIR)/socks5lib/*.cpp)
OBJECTS = $(patsubst %.cpp,%.o,$(SOURCES))
DEPENDS = $(patsubst %.cpp,%.d,$(SOURCES))
ASMFILE = $(patsubst %.cpp,%.s,$(SOURCES))

.PHONY: all clean

target = socks_client
all: $(target)

$(target): $(OBJECTS)	
	g++ -o $(target)  $(OBJECTS)  $(LDFLAGS)
#	mv $(target) $(LIB_LOCATION)

clean:
	@rm -fr $(OBJECTS) $(DEPENDS) $(ASMFILE) $(target)
	@rm -fr $(PUBLIC_DIR)/socks5lib/*.d $(PUBLIC_DIR)/socks5lib/*.o $(PUBLIC_DIR)/socks5lib/*.s 
	@rm -fr *.d *.o *.s 
