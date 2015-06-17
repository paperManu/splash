EXTRA_DIST = \
	splash/__init__.py \
	splash/operators.py \
	splash/ui.py

blenderSplash.zip:
	cd $(top_srcdir)/addons/blender
	zip blenderSplash.zip -r splash

clean:
	rm -f blenderSplash.zip Makefile

distclean: clean

datasdir = $(datadir)/splash/
datas_DATA = \
	blenderSplash.zip
