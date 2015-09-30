SUBDIRS = splash

EXTRA_DIST = \
	splash/__init__.py \
	splash/operators.py \
	splash/ui.py

blenderSplash.zip:
	cd $(top_builddir)/addons/blender
	zip blenderSplash.zip -r splash -x *.swp splash/Makefile*

clean: clean-recursive
	rm -f blenderSplash.zip Makefile

distclean: distclean-recursive
	rm -f blenderSplash.zip Makefile

datasdir = $(datadir)/splash/
datas_DATA = \
	blenderSplash.zip
