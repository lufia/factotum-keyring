#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dbus/dbus.h>

#include <u.h>
#include <libc.h>

#include "lib.h"

#define	SERVICE_NAME			"org.lufia.factotum"
#define	SERVICE_PATH			"/org/lufia/factotum"
#define	SERVICE_INTERFACE		"org.lufia.factotum.Keyring"
#define	PROPERTIES_INTERFACE	"org.freedesktop.DBus.Properties"

static int nextid = 0;

DBusHandlerResult
opensessionreply(DBusConnection *conn, DBusMessage *reply)
{
	DBusMessageIter iter, sub;
	char *z = "";
	char path[100], *p;

	dbus_message_iter_init_append(reply, &iter);
	dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT, DBUS_TYPE_STRING_AS_STRING, &sub);
	dbus_message_iter_append_basic(&sub, DBUS_TYPE_STRING, &z);
	dbus_message_iter_close_container(&iter, &sub);

	snprintf(path, sizeof path, "%s/session/%d", SERVICE_PATH, nextid++);
	debugf("Path = '%s'\n", path);
	p = path;
	dbus_message_iter_append_basic(&iter, DBUS_TYPE_OBJECT_PATH, &p);
	if (!dbus_connection_send(conn, reply, NULL)) {
		fprintf(stderr, "Failed to send reply message for OpenSession\n");
		dbus_message_unref(reply);
		return DBUS_HANDLER_RESULT_NEED_MEMORY;
	}
	dbus_connection_flush(conn);
	return DBUS_HANDLER_RESULT_HANDLED;
}

DBusHandlerResult
opensession(DBusConnection *conn, DBusMessage *msg)
{
	DBusError e;
	char *alg;
	DBusMessage *reply;
	DBusMessageIter iter, iter2;
	DBusHandlerResult r;

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
		debugf("Received OpenSession call with algorithm: %s, and variant input type: %c\n", alg, dbus_message_iter_get_arg_type(&iter2));
	}else{
		debugf("Received OpenSession call with algorithm: %s, and no variant input.\n", alg);
	}

	reply = dbus_message_new_method_return(msg);
	if(reply == NULL){
		fprintf(stderr, "failed to create reply message for OpenSession\n");
		return DBUS_HANDLER_RESULT_NEED_MEMORY;
	}
	r = opensessionreply(conn, reply);
	dbus_message_unref(reply);
	return r;
}

DBusHandlerResult
handle_properties_get(DBusConnection *conn, DBusMessage *msg)
{
	DBusMessage *reply;
	DBusMessageIter iter, sub_iter, array_iter;
	DBusError e;
	char *interface_name, *property_name;

	dbus_error_init(&e);
	if (!dbus_message_get_args(msg, &e,
	                           DBUS_TYPE_STRING, &interface_name,
	                           DBUS_TYPE_STRING, &property_name,
	                           DBUS_TYPE_INVALID)) {
		reply = dbus_message_new_error_printf(msg, DBUS_ERROR_INVALID_ARGS, "Invalid arguments for Get: %s", e.message);
		goto fail;
	}

	reply = dbus_message_new_method_return(msg);
	if (!reply) {
		debugf("Failed to create reply message for Get\n");
		return DBUS_HANDLER_RESULT_NEED_MEMORY;
	}

	if (strcmp(interface_name, SERVICE_INTERFACE) == 0 && strcmp(property_name, "Collections") == 0) {
		dbus_message_iter_init_append(reply, &iter);
		dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT, "ao", &sub_iter); // "ao" for array of object paths
		dbus_message_iter_open_container(&sub_iter, DBUS_TYPE_ARRAY, DBUS_TYPE_OBJECT_PATH_AS_STRING, &array_iter);
		// Currently, no collections, so return an empty array.
		dbus_message_iter_close_container(&sub_iter, &array_iter);
		dbus_message_iter_close_container(&iter, &sub_iter);
	} else {
		// Property not found or not supported
		dbus_message_unref(reply);
		reply = dbus_message_new_error_printf(msg, DBUS_ERROR_UNKNOWN_PROPERTY, "Unknown property %s on interface %s", property_name, interface_name);
		goto fail;
	}

	if (!dbus_connection_send(conn, reply, NULL)) {
		debugf("Failed to send reply message for Get\n");
		dbus_message_unref(reply);
		return DBUS_HANDLER_RESULT_NEED_MEMORY;
	}
	dbus_connection_flush(conn);
	dbus_message_unref(reply);
	return DBUS_HANDLER_RESULT_HANDLED;

fail:
	if (reply) {
		dbus_connection_send(conn, reply, NULL);
		dbus_message_unref(reply);
	}
	return DBUS_HANDLER_RESULT_HANDLED;
}

DBusHandlerResult
handle_properties_getall(DBusConnection *conn, DBusMessage *msg)
{
	DBusMessage *reply;
	DBusMessageIter iter, dict_iter, entry_iter, var_iter, array_iter;
	DBusError e;
	char *interface_name;

	dbus_error_init(&e);
	if (!dbus_message_get_args(msg, &e,
	                           DBUS_TYPE_STRING, &interface_name,
	                           DBUS_TYPE_INVALID)) {
		reply = dbus_message_new_error_printf(msg, DBUS_ERROR_INVALID_ARGS, "Invalid arguments for GetAll: %s", e.message);
		goto fail;
	}

	reply = dbus_message_new_method_return(msg);
	if (!reply) {
		debugf("Failed to create reply message for GetAll\n");
		return DBUS_HANDLER_RESULT_NEED_MEMORY;
	}

	dbus_message_iter_init_append(reply, &iter);
	dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &dict_iter); // Array of dictionary entries (string, variant)

	if (strcmp(interface_name, SERVICE_INTERFACE) == 0) {
		// Collections property
		dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &entry_iter);
		dbus_message_iter_append_basic(&entry_iter, DBUS_TYPE_STRING, "Collections");
		dbus_message_iter_open_container(&entry_iter, DBUS_TYPE_VARIANT, "ao", &var_iter); // "ao" for array of object paths
		dbus_message_iter_open_container(&var_iter, DBUS_TYPE_ARRAY, DBUS_TYPE_OBJECT_PATH_AS_STRING, &array_iter);
		// Currently, no collections, so return an empty array.
		dbus_message_iter_close_container(&var_iter, &array_iter);
		dbus_message_iter_close_container(&entry_iter, &var_iter);
		dbus_message_iter_close_container(&dict_iter, &entry_iter);
	} else {
		// No properties for other interfaces on this object
		fprintf(stderr, "No properties found for interface %s\n", interface_name);
	}

	dbus_message_iter_close_container(&iter, &dict_iter);

	if (!dbus_connection_send(conn, reply, NULL)) {
		debugf("Failed to send reply message for GetAll\n");
		dbus_message_unref(reply);
		return DBUS_HANDLER_RESULT_NEED_MEMORY;
	}
	dbus_connection_flush(conn);
	dbus_message_unref(reply);
	return DBUS_HANDLER_RESULT_HANDLED;

fail:
	if (reply) {
		dbus_connection_send(conn, reply, NULL);
		dbus_message_unref(reply);
	}
	return DBUS_HANDLER_RESULT_HANDLED;
}

typedef struct Func Func;
struct Func {
	char *service;
	char *method;
	DBusHandlerResult (*run)(DBusConnection*, DBusMessage*);
};
static Func ftab[] = {
	SERVICE_INTERFACE, "OpenSession", opensession,
	PROPERTIES_INTERFACE, "Get", handle_properties_get,
	PROPERTIES_INTERFACE, "GetAll", handle_properties_getall,
};
// https://specifications.freedesktop.org/secret-service/latest/org.freedesktop.Secret.Service.html
// methods: OpenSession, CreateCollection, SearchItems, Unlock, Lock, GetSecrets, ReadAlias, SetAlias

DBusHandlerResult
handle(DBusConnection *conn, DBusMessage *msg, void *aux)
{
	DBusMessage *reply;
	Func *fp;

	debugf("handle method call: %s.%s from %s\n",
	       dbus_message_get_interface(msg) ? dbus_message_get_interface(msg) : "(null)",
	       dbus_message_get_member(msg) ? dbus_message_get_member(msg) : "(null)",
	       dbus_message_get_sender(msg) ? dbus_message_get_sender(msg) : "(null)");
	for(fp = ftab; fp < ftab+nelem(ftab); fp++){
		if(dbus_message_is_method_call(msg, fp->service, fp->method))
			return fp->run(conn, msg);
	}
	debugf("Unhandled method call: %s.%s from %s\n",
	       dbus_message_get_interface(msg) ? dbus_message_get_interface(msg) : "(null)",
	       dbus_message_get_member(msg) ? dbus_message_get_member(msg) : "(null)",
	       dbus_message_get_sender(msg) ? dbus_message_get_sender(msg) : "(null)");
	reply = dbus_message_new_error_printf(msg, "org.freedesktop.Secret.Service.Error.NotImplemented", "Method %s not implemented.", dbus_message_get_member(msg));
	if (reply) {
		dbus_connection_send(conn, reply, NULL);
		dbus_message_unref(reply);
	}
	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusObjectPathVTable vtab = {
	.message_function = handle,
};

static void
usage(void)
{
	fprintf(stderr, "usage: %s [-d]\n", argv0);
	exit(2);
}

int
main(int argc, char **argv)
{
	DBusConnection *conn;
	DBusError e;
	int rv, flags;

	ARGBEGIN {
	case 'd':
		debug++;
		break;
	default:
		usage();
	} ARGEND

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
