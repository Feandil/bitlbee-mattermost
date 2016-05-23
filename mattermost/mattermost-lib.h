#include "mattermost.h"

void mattermost_find_self(struct im_connection *ic);
void mattermost_find_team(struct im_connection *ic);
void mattermost_find_users(struct im_connection *ic);
void mattermost_join_channels(struct im_connection *ic);
void mattermost_join_channel(struct groupchat *gic);
