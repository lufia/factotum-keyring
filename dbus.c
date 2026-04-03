#include <stdarg.h>
#include <dbus/dbus.h>

dbus_bool_t
v_appendstr(DBusMessageIter *it, char *fmt, ...)
{
	va_list arg;
	char s[512];

	va_start(arg, fmt);
	vsnprintf(s, sizeof s, fmt, arg);
	va_end(arg);
	return dbus_message_iter_append_basic(it, DBUS_TYPE_STRING, s);
}
