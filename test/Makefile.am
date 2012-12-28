AUTOMAKE_OPTIONS = 1.2 foreign no-installman no-installinfo

AM_CPPFLAGS = @CPPFLAGS@  -I/usr/include/python2.7 -fno-strict-aliasing -O2 -pipe -fwrapv -DNDEBUG
AM_CPPFLAGS += -I$(top_srcdir)/common/include
AM_CPPFLAGS += -I$(top_srcdir)/lib/ts -fPIC
DEFT_ROOT = $(HOME)/ats_deft

#LIBS += @TBD_1_LDADD@ @TBD_2_LDADD@ @TBD_3_LDADD@

noinst_PROGRAMS = proc_manager test_exec test_log_collate

proc_manager_SOURCES = proc_manager.cc \
		       log_sender.cc \
		       raf_cmd.cc \
		       rafencode.cc \
		       sio_buffer.cc \
		       sio_loop.cc \
		       sio_raf_server.cc \
		       test_utils.cc

proc_manager_LDFLAGS = @EXTRA_CXX_LDFLAGS@
proc_manager_LDADD = $(top_builddir)/lib/ts/libtsutil.la \
		     @LIBSOCKET@ @LIBNSL@ \
		     @LIBRESOLV@ @LIBRT@ -lm @LIBDL@

test_log_collate_SOURCES = test_log_collate.cc \
			   log_sender.cc \
			   rafencode.cc \
			   raf_cmd.cc \
			   sio_buffer.cc \
			   sio_loop.cc \
			   sio_raf_server.cc \
			   test_utils.cc

test_log_collate_LDFLAGS = @EXTRA_CXX_LDFLAGS@
test_log_collate_LDADD = $(top_builddir)/lib/ts/libtsutil.la \
		     	 @LIBSOCKET@ @LIBNSL@ \
		     	 @LIBRESOLV@ @LIBRT@ -lm @LIBDL@

test_exec_SOURCES = test_exec.cc \
		    rafencode.cc \
		    raf_cmd.cc \
		    remote_start.cc \
		    sio_buffer.cc \
		    sio_loop.cc \
		    test_interp_glue_wrap.c \
		    test_exec_python.c \
		    test_group.cc \
		    test_results.cc \
		    test_utils.cc

test_exec_LDFLAGS = @EXTRA_CXX_LDFLAGS@ -I/usr/include/python2.7 -fno-strict-aliasing -O2 -pipe -fwrapv -DNDEBUG -lpthread -ldl -lutil -lm -lpython2.7 -Xlinker -export-dynamic
test_exec_LDADD = $(top_builddir)/lib/ts/libtsutil.la\
		     @LIBSOCKET@ @LIBNSL@ \
		     @LIBRESOLV@ @LIBRT@ @LIBTCL@ @LIBDL@ \
		     @LIBCRYPT@ -lm

test_interp_glue_wrap.c: test_interp_glue.i test_interp_glue.h
	swig -python -Wall -I$(top_srcdir)/test test_interp_glue.i

## test_interp_glue_wrap does not like -Wall
#CFLAGS:=$(filter-out -Wall,$(CFLAGS))
## AM_CFLAGS:=$(filter-out -Wall,$(AM_CFLAGS))

install-deft: proc_manager test_exec test_log_collate
	$(mkinstalldirs) $(DEFT_ROOT)
	$(mkinstalldirs) $(DEFT_ROOT)/packages
	$(mkinstalldirs) $(DEFT_ROOT)/bin
	$(mkinstalldirs) $(DEFT_ROOT)/tmp
	$(mkinstalldirs) $(DEFT_ROOT)/log
	-$(RM) $(DEFT_ROOT)/scripts
	$(LN_S) `realpath ../test/scripts` $(DEFT_ROOT)/scripts
	-$(RM) $(DEFT_ROOT)/parsers
	$(LN_S) `realpath ../test/parsers` $(DEFT_ROOT)/parsers
	-$(RM) $(DEFT_ROOT)/defs
	$(LN_S) `realpath ../test/defs` $(DEFT_ROOT)/defs
	$(LIBTOOL) --mode=install install proc_manager $(DEFT_ROOT)/bin/proc_manager
	$(LIBTOOL) --mode=install install test_exec $(DEFT_ROOT)/bin/test_exec
	$(LIBTOOL) --mode=install install test_log_collate $(DEFT_ROOT)/bin/test_log_collate
	cp TestExec.py $(DEFT_ROOT)
	cp $(top_srcdir)/test/deft_profile.sh $(DEFT_ROOT)
	##perl $(top_srcdir)/test/setup_pkg_links.pl $(top_srcdir)/TestToolPackages $(DEFT_ROOT)/packages

clean-local:
	-$(RM) -rf Templates.DB cxx_repository
	-$(RM) -rf test_interp_glue_wrap.c TestExec.py

