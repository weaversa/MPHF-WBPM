EXTRAS = Makefile LICENSE README.md test/test.c

HEADERS = include/mphf_blocks.h include/mphf_hashes.h	\
include/mphf_serial.h include/mphf.h

SOURCES = src/mphf_blocks.c src/mphf_build.c src/mphf_hashes.c	\
src/mphf_query.c src/mphf_serial.c

OBJECTS = $(SOURCES:src/%.c=obj/%.o)

MPHFLIB = mphfwbpm
CC = gcc
#DBG = -g -Wconversion -Wall -fstack-protector-all -pedantic
OPT = -march=native -O3 -DNDEBUG -ffast-math -fomit-frame-pointer -finline-functions
INCLUDES = -Iinclude
LIBS = -l$(MPHFLIB) -lm
LDFLAGS = -Llib
CFLAGS = -std=gnu99 $(DBG) $(OPT) $(INCLUDES) -fopenmp
AR = ar r
RANLIB = ranlib

all: depend lib/lib$(MPHFLIB).a

depend: .depend
.depend: $(SOURCES)
	@echo "Building dependencies" 
ifneq ($(wildcard ./.depend),)
	@rm -f "./.depend"
endif
	@$(CC) $(CFLAGS) -MM $^ > .depend
# Make .depend use the 'obj' directory
	@sed -i.bak -e :a -e '/\\$$/N; s/\\\n//; ta' .depend
	@sed -i.bak 's/^/obj\//' .depend
	@rm -f .depend.bak
-include .depend

OBJECTS_BITVECTOR = lib/XORSATFilter/lib/bitvector/obj/*.o
OBJECTS_XORSAT = lib/XORSATFilter/obj/*.o
OBJECTS_HUNGARIAN = lib/weighted-bipartite-perfect-matching/obj/*.o

lib/XORSATFilter/lib/libxorsatfilter.a:
	@cd lib/XORSATFilter && $(MAKE)

lib/weighted-bipartite-perfect-matching/lib/libhungarian.a:
	@cd lib/weighted-bipartite-perfect-matching && $(MAKE)

$(OBJECTS): obj/%.o : src/%.c Makefile
	@echo "Compiling "$<""
	@[ -d obj ] || mkdir -p obj
	@$(CC) $(CFLAGS) -c $< -o $@

lib/lib$(MPHFLIB).a: $(OBJECTS) Makefile lib/XORSATFilter/lib/libxorsatfilter.a lib/weighted-bipartite-perfect-matching/lib/libhungarian.a
	@echo "Creating "$@""
	@[ -d lib ] || mkdir -p lib
	@rm -f $@
#	@$(AR) -M < libar.mri
	@$(AR) $@ $(OBJECTS_BITVECTOR)
	@$(AR) $@ $(OBJECTS_XORSAT)
	@$(AR) $@ $(OBJECTS_HUNGARIAN)
	@$(AR) $@ $(OBJECTS)
	@$(RANLIB) $@

test/test: test/test.c lib/lib$(MPHFLIB).a
	$(CC) $(CFLAGS) $(LDFLAGS) test/test.c -o test/test $(LIBS)

clean: FORCE
	@cd lib/XORSATFilter && $(MAKE) clean
	@cd lib/weighted-bipartite-perfect-matching && $(MAKE) clean
	rm -rf *~ */*~ $(OBJECTS) ./.depend test/test *.dSYM test/test.dSYM
	rm -rf lib/lib$(MPHFLIB).a

edit: FORCE
	emacs -nw $(EXTRAS) $(HEADERS) $(SOURCES)

FORCE:
