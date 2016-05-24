#include <bitlbee.h>

#include "mattermost-lib.h"
#include "mattermost-http.h"
#include "mattermost-obj.h"
#include "mattermost-bee.h"
#include "mattermost.h"

#include "json.h"

static void mattermost_find_self_cb(struct http_request *req);

void
mattermost_find_self(struct im_connection *ic)
{
	mattermost_http(ic, NULL, "users/me", FALSE, NULL, NULL,
			&mattermost_find_self_cb);
}

static void
mattermost_find_self_cb(struct http_request *req)
{
	struct im_connection *ic = req->data;
	struct mattermost_data *mmd;
	json_value * data;
	int ret;
	struct mattermost_user_data *ud;
	char self_alias[BUDDY_ALIAS_MAXLEN];

	/* Check if we didn't logout in the mean time */
	if (!g_slist_find(mattermost_connections, ic))
		return;
	mmd = ic->proto_data;

	ret = mattermost_parse_response(ic, req, &data);
	/* No etag set: no 304 */
	if (ret != 200) {
		imcb_error(ic, "Early failure: could not get self: %d", ret);
		imc_logout(ic, FALSE);
		return;
	}

	ud = mattermost_parse_user(data);
	json_value_free(data);
	if (ud == NULL) {
		imcb_error(ic, "Early failure: invalid self json");
		imc_logout(ic, FALSE);
		return;
	}

	mmd->self_id = g_strdup(ud->id);
	mattermost_user_alias(ud, self_alias);
	imcb_log(ic, "Hi, %s", self_alias);
	mattermost_free_user(ud);

	if (mmd->team_id != NULL)
		mattermost_find_users(ic);
}


static void mattermost_find_team_cb(struct http_request *req);

void
mattermost_find_team(struct im_connection *ic)
{
	mattermost_http(ic, NULL, "teams/all_team_listings", FALSE, NULL, NULL,
			mattermost_find_team_cb);
}

static void
mattermost_find_team_cb(struct http_request *req)
{
	struct im_connection *ic = req->data;
	struct mattermost_data *mmd;
	json_value * data;
	int i, ret;

	/* Check if we didn't logout in the mean time */
	if (!g_slist_find(mattermost_connections, ic))
		return;
	mmd = ic->proto_data;

	ret = mattermost_parse_response(ic, req, &data);
	/* No etag set: no 304 */
	if (ret != 200) {
		imcb_error(ic, "Early failure: could not get team: %d", ret);
		imc_logout(ic, FALSE);
		return;
	}

	if (!data || data->type != json_object) {
		json_value_free(data);
		imcb_error(ic, "Early failure: invalid team json");
		imc_logout(ic, FALSE);
		return;
	}
	for (i = 0; i < data->u.object.length; ++i) {
		char * id = data->u.object.values[i].name;
		json_value * team = data->u.object.values[i].value;
		int ti;
		if (!team || team->type != json_object) {
			/* Invalid team */
			continue;
		}
		for (ti = 0; ti < team->u.object.length; ++ti) {
			char * name = team->u.object.values[ti].name;
			json_value * value = team->u.object.values[ti].value;
			if (strcmp("name", name) == 0 &&
			    value && value->type == json_string &&
			    strcmp(mmd->team, value->u.string.ptr) == 0) {
				/* Found our team */
				mmd->team_id = g_strdup(id);
				break;
			}
		}
		if (mmd->team_id != NULL)
			break;
	}
	json_value_free(data);
	if (mmd->team_id == NULL) {
		imcb_error(ic, "Early failure: team not found!");
		imc_logout(ic, FALSE);
		return;
	}

	if (mmd->self_id != NULL)
		mattermost_find_users(ic);
}

static void mattermost_find_users_cb(struct http_request *req);

void
mattermost_find_users(struct im_connection *ic)
{
	struct mattermost_data *mmd;
	char *url;

	/* Check if we didn't logout in the mean time */
	if (!g_slist_find(mattermost_connections, ic))
		return;
	mmd = ic->proto_data;

	url = g_strdup_printf("users/profiles/%s", mmd->team_id);
	mattermost_http(ic, NULL, url, FALSE, NULL, NULL,
			&mattermost_find_users_cb);
	g_free(url);
}

static void
mattermost_find_users_cb(struct http_request *req)
{
	struct im_connection *ic = req->data;
	struct mattermost_data *mmd;
	json_value * data;
	int i, ret;

	/* Check if we didn't logout in the mean time */
	if (!g_slist_find(mattermost_connections, ic))
		return;
	mmd = ic->proto_data;

	ret = mattermost_parse_response(ic, req, &data);
	/* No etag set: no 304 */
	if (ret != 200) {
		imcb_error(ic, "Early failure: could not get users: %d", ret);
		imc_logout(ic, FALSE);
		return;
	}

	if (!data || data->type != json_object) {
		json_value_free(data);
		imcb_error(ic, "Early failure: invalid users json");
		imc_logout(ic, FALSE);
		return;
	}
	for (i = 0; i < data->u.object.length; ++i) {
		struct mattermost_user_data * ud;

		ud = mattermost_parse_user(data->u.object.values[i].value);
		if (ud != NULL)
			mattermost_add_user(ud, ic);
		mattermost_free_user(ud);
	}
	mattermost_join_channels(ic);
}

static void mattermost_join_channels_cb(struct http_request *req);

void
mattermost_join_channels(struct im_connection *ic)
{
	struct mattermost_data *mmd;
	char *url;

	/* Check if we didn't logout in the mean time */
	if (!g_slist_find(mattermost_connections, ic))
		return;
	mmd = ic->proto_data;

	url = g_strdup_printf("teams/%s/channels/", mmd->team_id);
	mattermost_http(ic, NULL, url, FALSE, NULL, NULL,
			&mattermost_join_channels_cb);
	g_free(url);
}

static void
mattermost_join_channels_cb(struct http_request *req)
{
	struct im_connection *ic = req->data;
	struct mattermost_data *mmd;
	json_value *data, *channels = NULL;
	int i, ret;

	/* Check if we didn't logout in the mean time */
	if (!g_slist_find(mattermost_connections, ic))
		return;
	mmd = ic->proto_data;

	ret = mattermost_parse_response(ic, req, &data);
	/* No etag set: no 304 */
	if (ret != 200) {
		imcb_error(ic, "Early failure: could not get channels: %d", ret);
		imc_logout(ic, FALSE);
		return;
	}

	if (!data || data->type != json_object) {
		json_value_free(data);
		imcb_error(ic, "Early failure: invalid channels json");
		imc_logout(ic, FALSE);
		return;
	}
	for (i = 0; i < data->u.object.length; ++i) {
		if (strcmp(data->u.object.values[i].name, "channels") == 0) {
			channels = data->u.object.values[i].value;
			break;
		}
	}
	if (channels == NULL ||channels->type != json_array) {
		imcb_error(ic, "Early failure: missing channels");
		imc_logout(ic, FALSE);
		return;
	}
	for (i = 0; i < channels->u.array.length; ++i) {
		struct mattermost_channel_data * cd;

		cd = mattermost_parse_channel(channels->u.array.values[i]);
		if (cd != NULL) {
			struct groupchat * chat;

			chat = mattermost_create_channel(cd, ic);
			if (chat == NULL)
				mattermost_free_channel(cd);
			else
				mattermost_join_channel(chat);
		}
	}
}

static void mattermost_join_channel_cb(struct http_request *req);

void
mattermost_join_channel(struct groupchat *gic)
{
	imcb_chat_log(gic, "Joining....");
	mattermost_http(gic->ic, gic, "extra_info/-1", FALSE, NULL, NULL,
			mattermost_join_channel_cb);
}

static void
mattermost_join_channel_cb(struct http_request *req)
{
	struct groupchat *gic = req->data;
	struct im_connection *ic;
	struct mattermost_data *mmd;
	json_value *data, *members = NULL;
	int i, ret;

	/* Check if we didn't logout in the mean time */
	if (!g_slist_find(mattermost_channels, gic))
		return;
	ic = gic->ic;
	mmd = ic->proto_data;

	ret = mattermost_parse_response(ic, req, &data);
	/* No etag set: no 304 */
	if (ret != 200) {
		imcb_error(ic, "Early failure: could not get detail: %d", ret);
		imc_logout(ic, FALSE);
		return;
	}

	if (!data || data->type != json_object) {
		json_value_free(data);
		imcb_error(ic, "Early failure: invalid channels json");
		imc_logout(ic, FALSE);
		return;
	}
	for (i = 0; i < data->u.object.length; ++i) {
		if (strcmp(data->u.object.values[i].name, "members") == 0) {
			members = data->u.object.values[i].value;
			break;
		}
	}
	if (members == NULL ||members->type != json_array) {
		imcb_error(ic, "Early failure: missing members");
		imc_logout(ic, FALSE);
		return;
	}
	for (i = 0; i < members->u.array.length; ++i) {
		int ci;
		char *uid = NULL;
		json_value *obj = members->u.array.values[i];

		if (!obj || obj->type != json_object) {
			imcb_error(ic, "Bad object");
			continue;
		}
		for (ci = 0; ci < obj->u.object.length; ++ci) {
			json_object_entry *d = &obj->u.object.values[ci];
			if (strcmp(d->name, "id") == 0 &&
			    d->value->type == json_string) {
				uid = d->value->u.string.ptr;
				break;
			}
		}

		if (uid != NULL)
			imcb_chat_add_buddy(gic, uid);
		else
			imcb_error(ic, "Missed one user");
	}
	mattermost_sync_channel(gic);
}

void
mattermost_sync_channel(struct groupchat *gic)
{
	imcb_chat_log(gic, "Syncing messages...");

}
