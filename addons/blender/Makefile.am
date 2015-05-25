EXTRA_DIST = \
	splash/__init__.py \
	splash/operators.py \
	splash/ui.py

blenderSplash.zip:
	zip blenderSplash.zip -r $(top_srcdir)/addons/blender/splash

clean:
	rm -f blenderSplash.zip Makefile

distclean: clean

datasdir = $(datadir)/splash/
datas_DATA = \
	blenderSplash.zip
