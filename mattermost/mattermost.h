#ifndef _BITLBEE_MATTERMOST_
#define _BITLBEE_MATTERMOST_

#include <bitlbee.h>

struct mattermost_data {
	gboolean tls;
	int port;
	char *host;
	char *api_url;
	char *auth_token;
	char *team;
	char *team_id;
	char *self_id;
};

struct mattermost_channel_data {
	char *path;
	char *id;
};

struct mattermost_user_data {
	char *id;
	char *nickname;
	char *firstname;
	char *lastname;
	char *username;
};

extern GSList *mattermost_connections;

#endif
