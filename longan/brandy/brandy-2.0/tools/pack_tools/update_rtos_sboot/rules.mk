#
# (C) Copyright 2006
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
#
# See file CREDITS for list of people who contributed to this
# project.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#

#########################################################################

_depend:	$(obj).depend

$(obj).depend:	$(src)Makefile $(TOPDIR)/config.mk $(SRCS) $(HOSTSRCS)
		@rm -f $@
		@touch $@
		@for f in $(SRCS); do \
			g=`basename $$f | sed -e 's/\(.*\)\.[[:alnum:]_]/\1.o/'`; \
			$(CC) -M $(CPPFLAGS) -MQ $(obj)$$g $$f >> $@ ; \
		done
		@for f in $(HOSTSRCS); do \
			g=`basename $$f | sed -e 's/\(.*\)\.[[:alnum:]_]/\1.o/'`; \
			$(HOSTCC) -M $(HOSTCPPFLAGS) -MQ $(obj)$$g $$f >> $@ ; \
		done

$(HOSTOBJS): $(obj)%.o: %.c
	$(HOSTCC) $(HOSTCFLAGS) $(HOSTCFLAGS_$(@F)) $(HOSTCFLAGS_$(BCURDIR)) -o $@ $< -c
$(NOPEDOBJS): $(obj)%.o: %.c
	$(HOSTCC) $(HOSTCFLAGS_NOPED) $(HOSTCFLAGS_$(@F)) $(HOSTCFLAGS_$(BCURDIR)) -o $@ $< -c

#########################################################################
