libexec_PROGRAMS = ibus-engine-zinnia

ibus_engine_zinnia_SOURCES = \
	main.c \
	engine.c \
	engine.h \
	zinnia_component.c \
	zinnia_component.h \
	$(NULL)
ibus_engine_zinnia_CPPFLAGS = \
	$(PREDEFINED_MACROS) \
	$(NULL)
ibus_engine_zinnia_CFLAGS = \
	@IBUS_CFLAGS@ \
	$(NULL)
ibus_engine_zinnia_LDFLAGS = \
	@IBUS_LIBS@ \
	-lzinnia \
	$(NULL)

component_DATA = \
	zinnia.xml \
	$(NULL)
componentdir = @datadir@/ibus/component

EXTRA_DIST = \
	zinnia.xml.in \
	$(NULL)

CLEANFILES = \
	zinnia.xml \
	$(NULL)

zinnia.xml: zinnia.xml.in
	$(AM_V_GEN) \
	( \
		libexecdir=${libexecdir}; \
		pkgdatadir=${pkgdatadir}; \
		s=`cat $<`; \
		eval "echo \"$${s}\""; \
	) > $@

