#pragma once

#include <glib.h>

struct mattermost_data {
	gboolean tls;
	int port;
	char *host;
	char *api_url;
	char *auth_token;
	char *team;
	char *team_id;
	char *self_id;
	GSList *channels;
};

extern GSList *mattermost_connections;
extern GSList *mattermost_channels;
