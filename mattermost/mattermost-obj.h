#pragma once

#include "json.h"

struct mattermost_user_data {
	char *id;
	char *nickname;
	char *firstname;
	char *lastname;
	char *username;
};

void mattermost_free_user(struct mattermost_user_data * ud);
struct mattermost_user_data *mattermost_parse_user(json_value * data);
void mattermost_user_alias(const struct mattermost_user_data * ud, char * buf);


struct mattermost_channel_data {
	char *id;
	char *type;
	char *name;
	char *topic;
	char *path;
};

void mattermost_free_channel(struct mattermost_channel_data * cd);
struct mattermost_channel_data *mattermost_parse_channel(json_value * data);

