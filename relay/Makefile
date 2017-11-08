
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

CPPFLAGS  =   -ggdb -DCORE_DUMP  -pthread -Wall 
CPPFLAGS += -fno-strict-aliasing -D__STDC_FORMAT_MACROS

CPPFLAGS += -I./
CPPFLAGS += -I./netmgr/
CPPFLAGS += -I./config/
CPPFLAGS += -I../include
CPPFLAGS += -I../public/socks5lib/
CPPFLAGS += -I../public/xutil/include
CPPFLAGS += -I../public/netpool/include
CPPFLAGS += -I../public/xjson/src/

CPPFLAGS += -rdynamic -fPIC

LDFLAGS += -lpthread

ifeq ($(ARCH),x86)
CPPFLAGS += -I../public/curl/32/include/
LDFLAGS += -L../public/curl/32/lib -lcurl
LDFLAGS += -L../public/xjson/32/lib -lxjson

LDFLAGS += -L../public/netpool/32/lib -lnetpool
else ifeq ($(ARCH),x64)
CPPFLAGS += -I../public/curl/64/include/
LDFLAGS += -L../public/curl/64/lib -lcurl
LDFLAGS += -L../public/xjson/64/lib -lxjson

LDFLAGS += -L../public/netpool/64/lib -lnetpool
else
CPPFLAGS += -I../public/curl/mips_32/include/
LDFLAGS += -L../public/curl/mips_32/lib -lcurl
LDFLAGS += -L../public/xjson/mips_32/lib -lxjson

LDFLAGS += -L../public/netpool/mips_32/lib -lnetpool
endif

ifeq ($(SUPPORT_MULTHREAD), not-support)

else
CPPFLAGS += -D_SUPPORT_MULTITHRAD
ifeq ($(ARCH), x86)
	CPPFLAGS += -I../public/openssl/32/include/
	LDFLAGS += -L../public/openssl/32/lib -lssl
	LDFLAGS += -L../public/openssl/32/lib -lcrypto
else ifeq ($(ARCH),x64)
	CPPFLAGS += -I../public/openssl/64/include/
	LDFLAGS += -L../public/openssl/64/lib -lssl
	LDFLAGS += -L../public/openssl/64/lib -lcrypto
endif

endif

SOURCES = $(wildcard ./*.cpp ./netmgr/*.cpp ./config/*.cpp)
SOURCES += $(wildcard ../public/socks5lib/*.cpp)

OBJECTS = $(patsubst %.cpp,%.o,$(SOURCES))
DEPENDS = $(patsubst %.cpp,%.d,$(SOURCES))
ASMFILE = $(patsubst %.cpp,%.s,$(SOURCES))

.PHONY: all clean

target = socks_relay
all: $(target)

$(target): $(OBJECTS)
	g++ -o $(target)  $(OBJECTS)  $(LDFLAGS)
#	mv $(target) $(LIB_LOCATION)

clean:
	@rm -fr $(OBJECTS) $(DEPENDS) $(ASMFILE) $(target)
	@rm -fr ../public/socks5lib/*.d ../public/socks5lib/*.o ../public/socks5lib/*.s 
	@rm -fr *.d *.o *.s 
