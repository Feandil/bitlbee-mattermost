#pragma once

#include <bitlbee.h>
#include <http_client.h>
#include "json.h"

struct http_request * mattermost_http(struct im_connection *ic,
	struct groupchat *c, const char *url, gboolean is_post,
	json_value * post_data, const char *etag, http_input_function cb);

int mattermost_parse_response(struct im_connection *ic,
			      struct http_request *req, json_value ** value);

