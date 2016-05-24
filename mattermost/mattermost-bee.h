#pragma once

#include "mattermost-obj.h"

#include <bitlbee.h>

bee_user_t * user_by_id(struct im_connection *ic, const char *handle);
void mattermost_add_user(const struct mattermost_user_data * ud, struct im_connection *ic);


struct groupchat * chat_by_id(struct im_connection *ic, const char *handle);
void mattermost_close_channel(struct groupchat *chat);
struct groupchat * mattermost_create_channel(struct mattermost_channel_data * cd, struct im_connection *ic);
