#
# hcxselect - A CSS selector engine for htmlcxx
#

SUBDIRS = src examples
.PHONY: all $(SUBDIRS)

all: examples

src:
	@$(MAKE) -wC src $(MFAGS)

examples: src
	@$(MAKE) -wC examples $(MFLAGS)

clean:
	for d in $(SUBDIRS); do ($(MAKE) -wC $$d clean ); done
