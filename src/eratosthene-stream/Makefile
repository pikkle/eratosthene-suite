MAKE_NAME:=eratosthene-stream
MAKE_LANG:=c
MAKE_TYPE:=suite

MAKE_DBIN:=bin
MAKE_DDOC:=doc
MAKE_DLIB:=lib
MAKE_DSRC:=src
MAKE_DOBJ:=obj
MAKE_DBUILD:=build

MAKE_OSYS:=$(shell uname -s)

MAKE_CMKD:=mkdir -p
MAKE_CRMF:=rm -f
MAKE_CRMD:=rm -rf
MAKE_CFCP:=cp

MAKE_CCMP:=gcc

ifeq ($(MAKE_OSYS),Linux)
MAKE_CLNK:=gcc-ar rcs
else
ifeq ($(MAKE_OSYS),Darwin)
MAKE_CLNK:=ar -rv
endif
endif
MAKE_CDOC:=doxygen

MAKE_FCMP:=
MAKE_FLNK:=


.PHONY:build

all:build

build:make-directories
	cd $(MAKE_DBUILD) && cmake .. && make && mv $(MAKE_NAME) ../$(MAKE_DBIN)

make-directories:
	$(MAKE_CMKD) $(MAKE_DBIN) $(MAKE_DDOC) $(MAKE_DOBJ) $(MAKE_DBUILD)

clean:
	rm -f $(MAKE_DBIN)/*
	rm -rf $(MAKE_DBUILD)/*
