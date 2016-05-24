#include <bitlbee.h>

#include "mattermost-obj.h"

static int
mattermost_json_o_ck(const char * target, char ** target_store,
		     json_object_entry * obj)
{
	if (strcmp(target, obj->name) != 0)
		return 0;
	if (!obj->value || obj->value->type != json_string)
		return 0;
	if (obj->value->u.string.length == 0)
		return 0;
	*target_store = g_memdup(obj->value->u.string.ptr,
				 obj->value->u.string.length + 1);
	return 1;
}


void
mattermost_free_user(struct mattermost_user_data * ud) {
{
	if (ud)
		g_free(ud->username);
		g_free(ud->lastname);
		g_free(ud->firstname);
		g_free(ud->nickname);
		g_free(ud->id);
		g_free(ud);
	}
}

struct mattermost_user_data *
mattermost_parse_user(json_value * data)
{
	struct mattermost_user_data * ud;
	int i;

	if (!data || data->type != json_object) {
		return NULL;
	}

	ud = g_new0(struct mattermost_user_data, 1);

	for (i = 0; i < data->u.object.length; ++i) {
		do {
			if (mattermost_json_o_ck("username", &ud->username,
						 &data->u.object.values[i]))
				break;
			if (mattermost_json_o_ck("first_name", &ud->firstname,
						 &data->u.object.values[i]))
				break;
			if (mattermost_json_o_ck("last_name", &ud->lastname,
						 &data->u.object.values[i]))
				break;
			if (mattermost_json_o_ck("id", &ud->id,
						 &data->u.object.values[i]))
				break;
			if (mattermost_json_o_ck("nickname", &ud->nickname,
						 &data->u.object.values[i]))
				break;
		} while (0);
	}

	if ((ud->username == NULL) || (ud->id == NULL)) {
		// Invalid user
		mattermost_free_user(ud);
		return NULL;
	}

	return ud;
}

void
mattermost_user_alias(const struct mattermost_user_data * ud, char * buf)
{
	if (ud->nickname)
		g_strlcpy(buf, ud->nickname, BUDDY_ALIAS_MAXLEN);
	else if (ud->firstname != NULL && ud->lastname != NULL)
		g_snprintf(buf, BUDDY_ALIAS_MAXLEN, "%s %s",
			   ud->firstname, ud->lastname);
	else
	g_strlcpy(buf, ud->username, BUDDY_ALIAS_MAXLEN);
}


void
mattermost_free_channel(struct mattermost_channel_data * cd) {
{
	if (cd)
		g_free(cd->path);
		g_free(cd->topic);
		g_free(cd->name);
		g_free(cd->type);
		g_free(cd->id);
		g_free(cd);
	}
}

struct mattermost_channel_data *
mattermost_parse_channel(json_value * data)
{
	struct mattermost_channel_data * cd;
	int i;

	if (!data || data->type != json_object) {
		return NULL;
	}

	cd = g_new0(struct mattermost_channel_data, 1);

	for (i = 0; i < data->u.object.length; ++i) {
		do {
			if (mattermost_json_o_ck("id", &cd->id,
						 &data->u.object.values[i]))
				break;
			if (mattermost_json_o_ck("type", &cd->type,
						 &data->u.object.values[i]))
				break;
			if (mattermost_json_o_ck("name", &cd->name,
						 &data->u.object.values[i]))
				break;
			if (mattermost_json_o_ck("header", &cd->topic,
						 &data->u.object.values[i]))
				break;
		} while (0);
	}

	if (cd->id == NULL || cd->type == NULL || cd->name == NULL) {
		// Invalid channel
		mattermost_free_channel(cd);
		return NULL;
	}

	return cd;
}
