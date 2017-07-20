MYSQL_CONFIG ?= mysql_config
MYSQL_CFLAGS := $(shell mysql_config --cflags)
MYSQL_LDFLAGS := $(shell mysql_config --libs)

CXXFLAGS := -DRPI -std=c++11 $(MYSQL_CFLAGS) $(if $(DEBUG),-O0 -g)
LDFLAGS := -lwiringPi $(MYSQL_LDFLAGS)

build: plantlifed

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $^ -o $@

plantlifed: main.o database.o rcswitch.o
	$(CXX) $(LDFLAGS) $^ -o $@

clean:
	$(RM) *.o plantlifed

.PHONY: build clean
