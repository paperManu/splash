SUBDIRS = \
	splash \
	templates

blenderSplash.zip:
	cd $(top_builddir)/addons/blender
	zip blenderSplash.zip -r splash -x *.swp splash/Makefile*
	rm -f templates/*.blend1
	tar cvjf blender_splash_addon.tar.bz2 --exclude='Makefile*' --exclude='*.swp' blenderSplash.zip templates

clean: clean-recursive
	rm -f *.zip *.tar.bz2

distclean: distclean-recursive
	rm -f *.zip *.tar.bz2 Makefile

datasdir = $(datadir)/splash/
datas_DATA = \
	blenderSplash.zip
