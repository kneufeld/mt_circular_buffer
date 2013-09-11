## Target type.
## all is one of: all-exec  all-libraries  all-shared  all-static
all: all-exec

test: all
	LD_LIBRARY_PATH=. ./$(TARGET)

## Target name. Use base name if making a library.
## Destination is where the target should end up when 'make install'
TARGET=mt_circular_buffer_tests
DESTINATION=.

OBJECTS = mt_circular_buffer_tests.o

## None of these can be blank (fill with '.' if nothing)
## OBJ_DIR where to put object files when compiling
## SRC_DIR where the src files live
## INC_DIR include these other directories when looking for header files
OBJ_DIR=.obj
SRC_DIR=.
INC_DIR=-I../include -I/usr/local/Cellar/cppunit/1.12.1/include -I/usr/local/Cellar/boost/1.54.0/include

## DEFINES pass in these extra #defines to gcc (no -D required)
DEFINES=

LDFLAGS = -L/usr/local/Cellar/cppunit/1.12.1/lib -L/usr/local/Cellar/boost/1.54.0/lib
LIBS = -lcppunit -lboost_system-mt -lboost_thread-mt

## Run make command in these directories
SUBDIRS =

## un/comment for debug symbols in executable
DEBUG =
## optimize level 0(none) .. 3(all)
OPTIMIZE = 0

DEFS = $(addprefix -D ,$(DEFINES))
CPPFLAGS = -I$(SRC_DIR) $(INC_DIR) -std=c++11
CFLAGS = $(DEBUG) -O$(OPTIMIZE) 
CXXFLAGS = $(DEBUG) -O$(OPTIMIZE) -std=c++11

## Compiler/tools information
#CC = gcc
CC = clang
CXX = /usr/local/Cellar/gcc48/4.8.1/bin/g++-4.8
#CXX = clang++
THREADING =

### YOU PROBABLY DON'T NEED TO CHANGE ANYTHING BELOW HERE ###

## Shell to use
SHELL = /bin/sh

## Commands to generate dependency files
GEN_DEPS.c=		$(CC) -M -xc $(DEFS) $(CPPFLAGS)
GEN_DEPS.cc=	$(CXX) -M -xc++ $(DEFS) $(CPPFLAGS)

## Commands to compile
COMPILE.c=	$(CC) -fPIC $(THREADING) $(DEFS) $(CPPFLAGS) $(CFLAGS) -c
COMPILE.cc=	$(CXX) -fPIC $(THREADING) $(DEFS) $(CPPFLAGS) $(CXXFLAGS) -c

## Commands to link.
LINK= $(CXX) $(THREADING) $(DEFS) $(CXXFLAGS)
LINK_STATIC=ar

## Force removal [for make clean]
RMV = rm -f
## Extra files to remove for 'make clean'
CLEANFILES = *~

## convert OBJECTS list into $(OBJ_DIR)/$OBJECTS
REAL_OBJS=$(addprefix $(OBJ_DIR)/,$(OBJECTS))

## convert OBJECTS to dependencies
DEPS = $(REAL_OBJS:.o=.d)
# pull in dependency info
-include $(DEPS)

## Compilation rules
$(SRC_DIR)/%.c: $(SRC_DIR)/%.h
$(SRC_DIR)/%.cpp: $(SRC_DIR)/%.hpp

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@#echo "compiling $<"
	$(COMPILE.c) -o $@ $<

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@#echo "compiling $<"
	$(COMPILE.cc) -o $@ $<

$(TARGET) : $(REAL_OBJS)
	@#echo "linking $@: $^"
	$(LINK) $(LDFLAGS) $(LIBS) $^ -o $@

lib$(TARGET).so: $(REAL_OBJS)
	$(LINK) $(LDFLAGS) $(LIBS) $^ -shared -o $@

lib$(TARGET).a: $(REAL_OBJS)
	$(LINK_STATIC) ru  $@ $^
	ranlib $@

## Dependency rules
## modify the dependancy files to reflect the fact their in an odd directory
$(OBJ_DIR)/%.d : $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	@echo "generating dependency information for $<"
	@$(GEN_DEPS.c) $< > $@
	@mv -f $(OBJ_DIR)/$*.d $(OBJ_DIR)/$*.d.tmp
	@sed -e 's|.*:|$(OBJ_DIR)/$*.o:|' < $(OBJ_DIR)/$*.d.tmp > $(OBJ_DIR)/$*.d
	@rm -f $(OBJ_DIR)/$*.d.tmp

$(OBJ_DIR)/%.d : $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	@echo "generating dependency information for $<"
	@$(GEN_DEPS.cc) $< > $@
	@mv -f $(OBJ_DIR)/$*.d $(OBJ_DIR)/$*.d.tmp
	@sed -e 's|.*:|$(OBJ_DIR)/$*.o:|' < $(OBJ_DIR)/$*.d.tmp > $(OBJ_DIR)/$*.d
	@rm -f $(OBJ_DIR)/$*.d.tmp

## List of phony targets
.PHONY : all all-local install install-local clean clean-local	\
distclean distclean-local install-library install-headers dist	\
dist-local check check-local

## Clear suffix list
.SUFFIXES :

install: install-recursive pre-all
	cp -f lib$(TARGET)* $(DESTINATION)
	ldconfig

clean: clean-recursive
	$(RMV) $(OBJ_DIR)/$(CLEANFILES) $(OBJ_DIR)/*.o $(OBJ_DIR)/*.d $(TARGET) lib$(TARGET).*

first:
	@mkdir -p $(OBJ_DIR)

pre-all: all-deps

all-deps: first $(DEPS)

all-exec: pre-all $(TARGET)

all-libraries: pre-all lib$(TARGET).so lib$(TARGET).a

all-shared: pre-all lib$(TARGET).so

all-static: pre-all lib$(TARGET).a

## Recursive targets
all-recursive install-recursive clean-recursive:
	@target=`echo $@ | sed s/-recursive//`; \
	list='$(SUBDIRS)'; \
	for subdir in $$list; do \
	  echo "Making $$target in $$subdir"; \
	  (cd $$subdir && $(MAKE) $$target) || exit; \
	done; \
