#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dbus/dbus.h>

#include <u.h>
#include <libc.h>

#define	SERVICE_NAME		"org.freedesktop.secrets"
#define	SERVICE_PATH		"/org/freedesktop/secrets"
#define	SERVICE_INTERFACE	"org.freedesktop.Secret.Service"

static int session_id_counter = 0;

DBusHandlerResult
opensession(DBusConnection *conn, DBusMessage *msg)
{
	DBusError e;
	char *alg;
	DBusMessage *reply;
	DBusMessageIter iter, iter2;

	dbus_error_init(&e);

	alg = NULL;
	if(!dbus_message_get_args(msg, &e, DBUS_TYPE_STRING, &alg, DBUS_TYPE_INVALID)){
		reply = dbus_message_new_error_printf(msg, DBUS_ERROR_INVALID_ARGS, "invalid args: %s", e.message);
		if(reply){
			dbus_connection_send(conn, reply, NULL);
			dbus_message_unref(reply);
		}
		return DBUS_HANDLER_RESULT_HANDLED;
	}

	dbus_message_iter_init(msg, &iter);
	if(dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_STRING)
		dbus_message_iter_next(&iter);
	if(dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_VARIANT){
		dbus_message_iter_recurse(&iter, &iter2);
		printf("Received OpenSession call with algorithm: %s, and variant input type: %c\n", alg, dbus_message_iter_get_arg_type(&iter2));
	}else{
		printf("Received OpenSession call with algorithm: %s, and no variant input.\n", alg);
	}

	reply = dbus_message_new_method_return(msg);
	if(reply == NULL){
		fprintf(stderr, "failed to create reply message for OpenSession\n");
		return DBUS_HANDLER_RESULT_NEED_MEMORY;
	}

	DBusMessageIter it;
	dbus_message_iter_init_append(reply, &it);
	DBusMessageIter sub_iter_output;
	dbus_message_iter_open_container(&it, DBUS_TYPE_VARIANT, DBUS_TYPE_STRING_AS_STRING, &sub_iter_output);
	char *empty_str = "";
	dbus_message_iter_append_basic(&sub_iter_output, DBUS_TYPE_STRING, &empty_str);
	dbus_message_iter_close_container(&it, &sub_iter_output);

	/* it's cheap */
	char session_path[256];
	snprintf(session_path, sizeof(session_path), "/org/freedesktop/secrets/session/%d", session_id_counter++);
	dbus_message_iter_append_basic(&it, DBUS_TYPE_OBJECT_PATH, &session_path);

	printf("  Returning session path: %s\n", session_path);

	if (!dbus_connection_send(conn, reply, NULL)) {
		fprintf(stderr, "Failed to send reply message for OpenSession\n");
		dbus_message_unref(reply);
		return DBUS_HANDLER_RESULT_NEED_MEMORY;
	}
	dbus_connection_flush(conn);

	dbus_message_unref(reply);
	return DBUS_HANDLER_RESULT_HANDLED;
}

DBusHandlerResult
handle(DBusConnection *conn, DBusMessage *msg, void *aux)
{
	DBusMessage *reply;

	// https://specifications.freedesktop.org/secret-service/latest/org.freedesktop.Secret.Service.html
	// methods: OpenSession, CreateCollection, SearchItems, Unlock, Lock, GetSecrets, ReadAlias, SetAlias
	if(dbus_message_is_method_call(msg, SERVICE_INTERFACE, "OpenSession"))
		return opensession(conn, msg);

	printf("Unhandled method call: %s.%s from %s\n",
		   dbus_message_get_interface(msg) ? dbus_message_get_interface(msg) : "(null)",
		   dbus_message_get_member(msg) ? dbus_message_get_member(msg) : "(null)",
		   dbus_message_get_sender(msg) ? dbus_message_get_sender(msg) : "(null)");
	reply = dbus_message_new_error_printf(msg, "org.freedesktop.Secret.Service.Error.NotImplemented", "Method %s not implemented.", dbus_message_get_member(msg));
	if(reply){
		dbus_connection_send(conn, reply, NULL);
		dbus_message_unref(reply);
	}
	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusObjectPathVTable vtab = {
	.message_function = handle,
};

int
main()
{
	DBusConnection *conn;
	DBusError e;
	int rv, flags;

	dbus_error_init(&e);
	conn = dbus_bus_get(DBUS_BUS_SESSION, &e);
	if(dbus_error_is_set(&e))
		sysfatal("failed to get connection: %s", e.message);
	if(conn == NULL)
		sysfatal("failed to connect to the D-Bus bus");

	flags = DBUS_NAME_FLAG_REPLACE_EXISTING | DBUS_NAME_FLAG_ALLOW_REPLACEMENT;
	rv = dbus_bus_request_name(conn, SERVICE_NAME, flags, &e);
	if(dbus_error_is_set(&e))
		sysfatal("failed to request dbus name: %s", e.message);
	if(rv != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
		sysfatal("failed to acquire bus name: %d\n", rv);

	if(!dbus_connection_register_object_path(conn, SERVICE_PATH, &vtab, NULL))
		sysfatal("failed to register object path: %s\n", SERVICE_PATH);
	while(dbus_connection_read_write_dispatch(conn, -1)) {
		// wait for
	}
	dbus_connection_unref(conn);
	return 0;
}
