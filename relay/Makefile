

ARCH = x64;
ifeq ($(shell getconf LONG_BIT), 32)
    ARCH = x86
else
    ARCH = x64
endif

CPPFLAGS  =   -ggdb -DCORE_DUMP  -pthread -Wall 
CPPFLAGS += -fno-strict-aliasing -D__STDC_FORMAT_MACROS

CPPFLAGS += -I./
CPPFLAGS += -I./netmgr/
CPPFLAGS += -I./config/
CPPFLAGS += -I../include
CPPFLAGS += -I../public/socks5lib/
CPPFLAGS += -I../public/xutil/include
CPPFLAGS += -I../public/netpool/include
CPPFLAGS += -I../public/netpool/outclass
CPPFLAGS += -I../public/xjson/

ifeq ($(ARCH), x86)
	CPPFLAGS += -I../public/curl/32/include/
else
	CPPFLAGS += -I../public/curl/64/include/
endif

CPPFLAGS += -rdynamic -fPIC

LDFLAGS += -lpthread
#LDFLAGS += -L../public/socks5lib -lsocks5common
#LDFLAGS += -L../public/netpool -lnetpool
#LDFLAGS += -L../public/xutil -lxutil
LDFLAGS += -L../public/xjson -lxjson

ifeq ($(ARCH), x86)
LDFLAGS += -L../public/curl/32/lib -lcurl
else
LDFLAGS += -L../public/curl/64/lib -lcurl
endif

LDFLAGS += -lrt -lcrypto -lssl

SOURCES = $(wildcard ./*.cpp ./netmgr/*.cpp ./config/*.cpp ../public/netpool/src/*.cpp  ../public/netpool/outclass/*.cpp  ../public/xutil/src/*.cpp ../public/socks5lib/*.cpp)
OBJECTS = $(patsubst %.cpp,%.o,$(SOURCES))
DEPENDS = $(patsubst %.cpp,%.d,$(SOURCES))
ASMFILE = $(patsubst %.cpp,%.s,$(SOURCES))

.PHONY: all clean

#target = libutil.so
target = socks_relay
all: $(target)

$(target): $(OBJECTS)	
	g++ -o $(target)  $(OBJECTS)  $(LDFLAGS)
#	mv $(target) $(LIB_LOCATION)

clean:
	@rm -fr $(OBJECTS) $(DEPENDS) $(ASMFILE) $(target)
	@rm -fr *.d *.o *.s 
