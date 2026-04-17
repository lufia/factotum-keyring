typedef struct Session Session;
struct Session {
	int ref;
};

extern	void	debugf(char *, ...);

extern	dbus_bool_t	v_appendstr(DBusMessageIter *, char *, ...);

extern	int	debug;
