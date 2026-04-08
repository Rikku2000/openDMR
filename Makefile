CPP      = g++
SRCS	 = $(wildcard *.cpp)
OBJ      = $(SRCS:.cpp=.o) $(RES)
LINKOBJ  = $(SRCS:.cpp=.o) $(RES)
LIBS     = -L/usr/lib -lsqlite3 -lpthread -lcrypto -lssl
CXXINCS  = -I/usr/include
BIN      = server
CXXFLAGS = -DSQLITE3 $(CXXINCS) -DUSE_SQLITE3 -DUSE_OPENSSL -DUSE_UPLINK -DHAVE_APRS -DHAVE_SMS -DHAVE_HTTPMODE -fno-for-scope -std=gnu++11 -Wreturn-type -O2 -Wno-write-strings
RM       = rm -f

all: $(OBJS)

.PHONY: all all-before all-after clean clean-custom

all: all-before $(BIN) all-after

clean: clean-custom
	${RM} $(OBJ) $(BIN)

$(BIN): $(OBJ)
	$(CPP) $(LINKOBJ) -o $(BIN) $(LIBS)

*.o: *.cpp
	$(CPP) -c *.cpp -o *.o $(CXXFLAGS)
