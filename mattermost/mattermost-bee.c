#include <bitlbee.h>

#include "mattermost-bee.h"
#include "mattermost.h"

bee_user_t *
user_by_id(struct im_connection *ic, const char *handle)
{
	GSList *l;
	bee_t *bee = ic->bee;

	for (l = bee->users; l; l = l->next) {
		bee_user_t *bu = l->data;

		if (bu->ic == ic && ic->acc->prpl->handle_cmp(bu->handle, handle) == 0)
			return bu;
	}

	return NULL;
}

void
mattermost_add_user(const struct mattermost_user_data * ud,
		    struct im_connection *ic)
{
	char user_alias[BUDDY_ALIAS_MAXLEN];
	struct mattermost_data *mmd = ic->proto_data;

	if (ic->acc->prpl->handle_cmp(ud->id, mmd->self_id) == 0)
		return;

	if (!bee_user_by_handle(ic->bee, ic, ud->id)) {
		mattermost_user_alias(ud, user_alias);
		imcb_add_buddy(ic, ud->id, NULL);
		imcb_rename_buddy(ic, ud->id, user_alias);
		imcb_buddy_nick_hint(ic, ud->id, ud->username);
	}
}

struct groupchat *
chat_by_id(struct im_connection *ic, const char *handle)
{
	struct mattermost_data *mmd = ic->proto_data;
	GSList *l;

	for (l = mmd->channels; l != NULL; l = l->next) {
		struct groupchat *c = l->data;
		struct mattermost_channel_data *cd = c->data;

		if (strcmp(cd->id, handle) == 0)
			return c;
	}

	return NULL;
}

void
mattermost_close_channel(struct groupchat *chat)
{
	struct mattermost_channel_data *cd = chat->data;

	mattermost_free_channel(cd);
	imcb_chat_free(chat);
}

struct groupchat *
mattermost_create_channel(struct mattermost_channel_data * cd,
			  struct im_connection *ic)
{
	struct groupchat *chat;
	struct mattermost_data *mmd = ic->proto_data;

	if (chat_by_id(ic, cd->id) != NULL)
		return NULL;

	switch (*cd->type) {
	case 'D':
		//TODO: set buddy online
		break;
	case 'P':
	case 'O':
		chat = imcb_chat_new(ic, cd->id);
		imcb_chat_name_hint(chat, cd->name);
		if (cd->topic)
			imcb_chat_topic(chat, NULL, cd->topic, 0);
		cd->path = g_strdup_printf("teams/%s/channels/%s/",
					   mmd->team_id, cd->id);
		chat->data = cd;
		mmd->channels = g_slist_append(mmd->channels, chat);
		mattermost_channels = g_slist_append(mattermost_channels, chat);
		return chat;
	default:
		imcb_error(ic, "Unsupported channel type: %s", cd->type);
		break;
	}

	return NULL;
}

