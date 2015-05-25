ACLOCAL_AMFLAGS = -I m4
SUBDIRS = addons \
          src \
          data \
		  tests \
		  tools \
		  osx

if HAVE_MARKDOWN
dist_man_MANS = README.md
$(top_builddir): markdown.stamp

markdown.stamp:
	$(MARKDOWN) README.md > README.html

CLEANFILES = README.html

doc: markdown.stamp
clean-local:
	rm -rf $(top_builddir)/README.html
	rm -rf $(top_builddir)/osx/Info.plist
endif
