#include <bitlbee.h>

#include "mattermost-http.h"
#include "mattermost.h"
#include "mattermost-obj.h"

#include "json-builder.h"


/*
 * TODO: Handle rate limit
 */

struct http_request *
mattermost_http(struct im_connection *ic, struct groupchat *c, const char *url,
		gboolean is_post, json_value * post_data, const char *etag,
		http_input_function cb)
{
	struct mattermost_data *mmd = ic->proto_data;
	struct mattermost_channel_data *chand = c ? c->data : NULL;
	GString *request = g_string_new("");
	struct http_request *ret;

	// Build the base request
	g_string_printf(request, "%s %s%s%s HTTP/1.1\r\n"
			"Host: %s\r\n"
			"User-Agent: BitlBee " BITLBEE_VERSION "\r\n"
			"Authorization: BEARER %s\r\n",
			is_post ? "POST" : "GET",
			mmd->api_url,
			chand ? chand->path : "",
			url ? url : "",
			mmd->host,
			ic->acc->pass);

	// Add the etag if needed
	if (etag != NULL)
		g_string_append_printf(request,
				       "If-None-Match: %s\r\n",
				       etag);

	// Fill the rest
	if (is_post) {
		size_t data_len = json_measure(post_data);
		char * ser_data = g_malloc(data_len);
		json_serialize(ser_data, post_data);
		g_string_append_printf(request,
				       "Content-Type: application/json"
				       "Content-Length: %zd\r\n\r\n%s",
				       data_len, ser_data);
		g_free(ser_data);
	} else {
		g_string_append(request, "\r\n");
	}

	ret = http_dorequest(mmd->host, mmd->port, mmd->tls, request->str,
			     cb, c ? (void*) c : (void *) ic);

	g_string_free(request, TRUE);
	return ret;
}

int
mattermost_parse_response(struct im_connection *ic, struct http_request *req,
			  json_value ** value)
{
	json_value * ret;

	if (req->status_code != 200 && req->status_code != 304) {
		/* Un-handled error */
		if (getenv("BITLBEE_DEBUG"))
			imcb_error(ic, "Received error code %d: %.*s",
				   req->status_code, req->body_size,
				   req->reply_body);
		return req->status_code;
	}

	ret = json_parse(req->reply_body, req->body_size);
	if (ret == NULL) {
		imcb_error(ic, "Received error invalid json");
		if (getenv("BITLBEE_DEBUG"))
			imcb_error(ic, "Data: %.*s",
				   req->body_size, req->reply_body);
		return -1;
	}
	*value = ret;
	return req->status_code;
}

