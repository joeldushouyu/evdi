#
# Copyright (c) 2022 DisplayLink (UK) Ltd.
#

INSTALL ?= install
CP ?= cp
LN ?= ln
RM ?= rm

LIBABI := 1

INCL_DIRS := -I../library -I/usr/include


# -fvisibility=hidden https://pybind11.readthedocs.io/en/stable/faq.html#someclass-declared-with-greater-visibility-than-the-type-of-its-field-someclass-member-wattributes
CXXFLAGS := $(INCL_DIRS) -std=c++11 -fPIC  -g  $(CXXFLAGS) 
CFLAGS := $(INCL_DIRS) -g
LDFLAGS := -L../library -L/usr/include 

NAME := PyEvdi$(NAME_SUFFIX)

SRCS := $(wildcard *.cpp)
OBJS := $(patsubst %.cpp,%.o,$(SRCS))
LIBS := -levdi

C_SRCS := $(wildcard *.c)
C_OBJS := $(patsubst %.c,%.o,$(C_SRCS))

default:  main 

clean:
	rm -rf $(NAME)
	rm -rf $(OBJS)
	rm -rf main
	rm -rf qtTest
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<  



# $(NAME): $(OBJS)
# 	$(CXX) $^ $(LDFLAGS) $(LIBS) -Wl,-soname,$(NAME).so.$(LIBABI) -o $(NAME)

main: $(OBJS)
	$(CXX) $^ $(LDFLAGS) $(LIBS) -Wl,-soname,$(NAME).so.$(LIBABI) -o main -lm -lpthread -lX11 

# tests:
# 	LD_LIBRARY_PATH=../library python3 -m pytest test/

# LIBDIR ?= $$(python3 -c "from distutils.sysconfig import get_python_lib; print(get_python_lib())")

# install:
# 	$(INSTALL) -d $(DESTDIR)$(LIBDIR)
# 	$(INSTALL) -m 755 $(NAME
