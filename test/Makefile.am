AUTOMAKE_OPTIONS = 1.2 foreign no-installman no-installinfo

#INCLUDES += @TBD_1_INCDIR@
#INCLUDES += @TBD_2_INCDIR@
#INCLUDES += @TBD_3_INCDIR@
AM_CPPFLAGS = @CPPFLAGS@ -I/usr/lib64/perl5/5.16.1/x86_64-linux-thread-multi/CORE/
AM_CPPFLAGS += -I$(top_srcdir)/common/include
AM_CPPFLAGS += -I$(top_srcdir)/lib/ts
DEFT_INSTALL = $(HOME)/ats_deft

#AM_CPPFLAGS += -I$(top_builddir)/f_pkg/misc/perl5.6.1/include

#LIBS += @TBD_1_LDADD@ @TBD_2_LDADD@ @TBD_3_LDADD@

bin_PROGRAMS = proc_manager test_exec test_log_collate

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
		    test_exec_perl.c \
		    test_group.cc \
		    test_results.cc \
		    test_utils.cc

test_exec_LDFLAGS = @EXTRA_CXX_LDFLAGS@
test_exec_LDADD = $(top_builddir)/lib/ts/libtsutil.la\
		     -L/usr/local/lib  \
		     -lperl \
		     @LIBSOCKET@ @LIBNSL@ \
		     @LIBRESOLV@ @LIBRT@ @LIBTCL@ @LIBDL@ \
		     @LIBCRYPT@ -lm 

test_interp_glue_wrap.c: test_interp_glue.i test_interp_glue.h
	swig -perl5 -static -Wall -I$(top_srcdir)/test test_interp_glue.i


install-deft: proc_manager test_exec test_log_collate
	$(mkinstalldirs) $(DEFT_INSTALL)
	$(mkinstalldirs) $(DEFT_INSTALL)/packages
	$(mkinstalldirs) $(DEFT_INSTALL)/bin
	-$(RM) $(DEFT_INSTALL)/scripts
	$(LN_S) `realpath ../test/scripts` $(DEFT_INSTALL)/scripts
	-$(RM) $(DEFT_INSTALL)/parsers
	$(LN_S) `realpath ../test/parsers` $(DEFT_INSTALL)/parsers
	-$(RM) $(DEFT_INSTALL)/defs
	$(LN_S) `realpath ../test/defs` $(DEFT_INSTALL)/defs
	$(LIBTOOL) --mode=install install proc_manager $(DEFT_INSTALL)/bin/proc_manager
	$(LIBTOOL) --mode=install install test_exec $(DEFT_INSTALL)/bin/test_exec
	$(LIBTOOL) --mode=install install test_log_collate $(DEFT_INSTALL)/bin/test_log_collate
	cp TestExec.pm $(DEFT_INSTALL)
	cp $(top_srcdir)/test/deft_profile.sh $(DEFT_INSTALL)
	perl $(top_srcdir)/test/setup_pkg_links.pl $(top_srcdir)/TestToolPackages $(DEFT_INSTALL)/packages

clean-local:
	-$(RM) -rf Templates.DB cxx_repository .deps
	-$(RM) -rf test_interp_glue_wrap.c TestExec.pm

