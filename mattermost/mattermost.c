#include "mattermost.h"

#include <url.h>

#include "mattermost-lib.h"

GSList *mattermost_connections = NULL;


static void mattermost_init(account_t * acc)
{
	set_t *s;

	s = set_add(&acc->set, "url", NULL, NULL, acc);
	s->flags |= ACC_SET_OFFLINE_ONLY;
}

static void mattermost_login(account_t * acc)
{
	struct im_connection *ic = imcb_new(acc);
	url_t url;
	const char *conf_url = set_getstr(&ic->acc->set, "url");
	struct mattermost_data *mmd;

	if (ic->acc->pass == NULL ||
	    strcmp(PASSWORD_PENDING, ic->acc->pass) == 0) {
		imcb_error(ic, "No authentication token set!");
		imc_logout(ic, FALSE);
		return;
	}

	if (conf_url == NULL ||!url_set(&url, conf_url) ||
	    (url.proto != PROTO_HTTP && url.proto != PROTO_HTTPS) ||
	    strlen(url.file) <= 1) {
		imcb_error(ic, "Incorrect URL", conf_url);
		imc_logout(ic, FALSE);
		return;
	}

	imcb_log(ic, "Connecting...");

	mattermost_connections = g_slist_append(mattermost_connections, ic);
	mmd = g_new0(struct mattermost_data, 1);
	ic->proto_data = mmd;
	mmd->tls = url.proto == PROTO_HTTPS;
	mmd->port = url.port;
	mmd->host = g_strdup(url.host);
	mmd->api_url = g_strdup_printf("http%s://%s/api/v3/",
				       mmd->tls ? "s" : "", mmd->host);
	mmd->team = g_strdup(url.file + 1);

	mattermost_find_self(ic);
	mattermost_find_team(ic);
}

static void mattermost_logout(struct im_connection *ic)
{
	struct mattermost_data *mmd = ic->proto_data;

	// Set the status to logged out.
	ic->flags &= ~OPT_LOGGED_IN;

	if (mmd) {
		g_free(mmd->team);
		g_free(mmd->api_url);
		g_free(mmd->host);
		g_free(mmd);
	}
	mattermost_connections = g_slist_remove(mattermost_connections, ic);
}

void init_plugin()
{
	struct prpl *ret = g_new0(struct prpl, 1);

	ret->options = OPT_NOOTR;
	ret->name = "mattermost";
	ret->init = mattermost_init;
	ret->login = mattermost_login;
	ret->logout = mattermost_logout;
	ret->keepalive = NULL; /* We don't need it */

	ret->handle_cmp = g_strcasecmp;

	register_protocol(ret);
}
