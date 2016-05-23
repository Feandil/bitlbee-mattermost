#include "mattermost.h"

#include "mattermost-lib.h"
#include "mattermost-http.h"

#include "json.h"

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


static void
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

static struct mattermost_user_data *
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

static void
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


static void mattermost_find_self_cb(struct http_request *req);

void
mattermost_find_self(struct im_connection *ic)
{
	mattermost_http(ic, NULL, "users/me", FALSE, NULL, NULL,
			&mattermost_find_self_cb, ic);
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
		mattermost_update_channels(ic);
}


static void mattermost_find_team_cb(struct http_request *req);

void
mattermost_find_team(struct im_connection *ic)
{
	mattermost_http(ic, NULL, "teams/all_team_listings", FALSE, NULL, NULL,
			mattermost_find_team_cb, ic);
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

	mmd->team_url = g_strdup_printf("%steams/%s/", mmd->api_url, mmd->team_id);
	if (mmd->self_id != NULL)
		mattermost_update_channels(ic);
}
