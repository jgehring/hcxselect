#
# hcxselect - A CSS selector engine for htmlcxx
#

SUBDIRS = src examples test
.PHONY: all $(SUBDIRS)

all: examples test

src:
	@$(MAKE) -wC src $(MFAGS)

examples: src
	@$(MAKE) -wC examples $(MFLAGS)

test: src
	@$(MAKE) -wC test $(MFLAGS)

clean:
	for d in $(SUBDIRS); do ($(MAKE) -wC $$d clean ); done
