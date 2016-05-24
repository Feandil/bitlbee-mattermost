libdir = $(plugindir)
lib_LTLIBRARIES = mattermost.la

mattermost_la_CFLAGS = \
	$(BITLBEE_CFLAGS) \
	$(GLIB_CFLAGS)

mattermost_la_LDFLAGS = \
	$(BITLBEE_LDFLAGS) \
	$(GLIB_LDFLAGS)

mattermost_la_SOURCES = \
	mattermost-obj.h \
	mattermost-obj.c \
	mattermost-bee.h \
	mattermost-bee.c \
	mattermost-lib.h \
	mattermost-lib.c \
	mattermost-http.h \
	mattermost-http.c \
	mattermost.h \
	mattermost.c \
	json.h \
	json.c \
	json-builder.h \
	json-builder.c

# Build the library as a module
mattermost_la_LDFLAGS += -module -avoid-version

