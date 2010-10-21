/* LICENSE BEGIN

Copyright (c) 2010 Ysj.Ray

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

 LICENSE END */


#include "pytypes.h"

#include <mysql.h>
#include <glib.h>
#include <arpa/inet.h>
#include <mysqld_error.h>
#include "network-mysqld-proto.h"
#include "glib-ext.h"
#include "network-mysqld-packet.h"

#include "pyproxy-plugin.h"
#include "network-mysqld-python.h"

/*--------------------------------------------------------------------*/

/**
 * Note: this function return 0 on success, -1 on error,
 * this is different from PyErr_Format()!
 * Format objects are ended by 'NULL'.
 */
int
PyErr_FormatObject(PyObject *exception, const char *format, ...){
	va_list vargs;

#ifdef HAVE_STDARG_PROTOTYPES
	va_start(vargs, format);
#else
	va_start(vargs);
#endif

	PyObject *item;
	PyObject *arg_list = PyList_New(0);
	do{
		item = va_arg(vargs, PyObject *);
		if(!item)
			break;
		if(PyList_Append(arg_list, item))
			goto failed1;
	}while(item);
	va_end(vargs);

	PyObject *arg_tuple = PyTuple_New(PyList_Size(arg_list));
	if(!arg_tuple)
		goto failed1;

	int i = 0;
	for(i = 0; i < PyList_Size(arg_list); i++){
		PyObject *list_item = PyList_GetItem(arg_list, i);
		if(!list_item)
			goto failed2;
		if(PyTuple_SetItem(arg_tuple, i, list_item))
			goto failed2;
	}

	PyObject *format_obj = PyString_FromString(format);
	if(!format_obj)
		goto failed2;

	PyObject *format_result = PyString_Format(format_obj, arg_tuple);
	if(!format_result){
		Py_DECREF(format_obj);
		goto failed2;
	}

	PyErr_SetString(exception, PyString_AsString(format_result));
	Py_DECREF(arg_list);
	Py_DECREF(arg_tuple);
	return 0;

failed2:
	Py_DECREF(arg_tuple);
failed1:
	Py_DECREF(arg_list);
	return -1;
}


//-----------------------------------------------------------------
#define PY_TYPE_DEF(name) \
PyTypeObject name ## _Type = {\
    PyObject_HEAD_INIT(&PyType_Type)\
    0,                         /*ob_size*/\
	#name,           /*tp_name*/\
    sizeof(name),           /*tp_basicsize*/\
    0,                         /*tp_itemsize*/\
    0,			  /*tp_dealloc*/\
    0,                         /*tp_print*/\
    0,                         /*tp_getattr*/\
    0,                         /*tp_setattr*/\
    0,                         /*tp_compare*/\
    0,    /*tp_repr*/\
    0,                         /*tp_as_number*/\
    0,                         /*tp_as_sequence*/\
    0,                         /*tp_as_mapping*/\
    0,                         /*tp_hash */\
    0,                         /*tp_call*/\
    0,                         /*tp_str*/\
	0,		/* tp_getattro */\
	0,		/* tp_setattro */\
    0,                         /*tp_as_buffer*/\
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/\
    0,			/* tp_doc */\
    0,		               /* tp_traverse */\
    0,		               /* tp_clear */\
    0,		               /* tp_richcompare */\
    0,		               /* tp_weaklistoffset */\
    0,		               /* tp_iter */\
    0,		               /* tp_iternext */\
    name ## _methods,					    /* tp_methods */\
    name ## _members,			             /* tp_members */\
    name ## _getsets,         /* tp_getset */\
    0,                         /* tp_base */\
    0,                         /* tp_dict */\
    0,                         /* tp_descr_get */\
	0,					/* tp_descr_set */\
	0,	/* tp_dictoffset */\
	0,					/* tp_init */\
	PyType_GenericAlloc,	/* tp_alloc */\
	0			/* tp_new */\
};


#define GETTER_CONST_INT_DEF(type, name) \
static PyObject *\
type ## _get_ ## name(type *obj, void *closure){\
	return (PyObject *)PyInt_FromLong(name);\
}

#define GETTER_CONST_STRING_DEF(type, name) \
static PyObject *\
type ## _get_ ## name(type *obj, void *closure){\
	return (PyObject *)PyString_FromString(name);\
}

#define GETTER_MEMBER_INT_DEF(type, name) \
static PyObject *\
type ## _get_ ## name(type *obj, void *closure){\
	return (PyObject *)PyInt_FromLong(obj->name);\
}

#define GETTER_MEMBER_STRING_DEF(type, name) \
static PyObject *\
type ## _get_ ## name(type *obj, void *closure){\
	return (PyObject *)PyString_FromString(obj->name);\
}

#define GETTER_MEMBER_DEF(type, name) \
static PyObject *\
type ## _get_ ## name(type *obj, void *closure){\
	Py_INCREF(obj->name);\
	return (PyObject *)(obj->name);\
}

#define GETTER_DECLEAR(type, name)\
	{#name, (getter) type ## _get_ ## name, NULL, NULL},

#define GETSET_DECLEAR(type, name)\
	{#name, (getter) type ## _get_ ## name, (setter) type ## _set_ ## name, NULL},

//---------------------------------Proxy------------------------------
GETTER_CONST_STRING_DEF(Proxy, PROXY_VERSION)

//GETTER_CONST_INT_DEF(Proxy, PROXY_NO_DECISION)
GETTER_CONST_INT_DEF(Proxy, PROXY_SEND_QUERY)
GETTER_CONST_INT_DEF(Proxy, PROXY_SEND_RESULT)
//GETTER_CONST_INT_DEF(Proxy, PROXY_SEND_INJECTION)
GETTER_CONST_INT_DEF(Proxy, PROXY_IGNORE_RESULT)

GETTER_CONST_INT_DEF(Proxy, MYSQLD_PACKET_OK)
GETTER_CONST_INT_DEF(Proxy, MYSQLD_PACKET_ERR)
GETTER_CONST_INT_DEF(Proxy, MYSQLD_PACKET_RAW)

GETTER_CONST_INT_DEF(Proxy, BACKEND_STATE_UNKNOWN)
GETTER_CONST_INT_DEF(Proxy, BACKEND_STATE_UP)
GETTER_CONST_INT_DEF(Proxy, BACKEND_STATE_DOWN)

GETTER_CONST_INT_DEF(Proxy, BACKEND_TYPE_UNKNOWN)
GETTER_CONST_INT_DEF(Proxy, BACKEND_TYPE_RW)
GETTER_CONST_INT_DEF(Proxy, BACKEND_TYPE_RO)

GETTER_CONST_INT_DEF(Proxy, COM_SLEEP)
GETTER_CONST_INT_DEF(Proxy, COM_QUIT)
GETTER_CONST_INT_DEF(Proxy, COM_INIT_DB)
GETTER_CONST_INT_DEF(Proxy, COM_QUERY)
GETTER_CONST_INT_DEF(Proxy, COM_FIELD_LIST)
GETTER_CONST_INT_DEF(Proxy, COM_CREATE_DB)
GETTER_CONST_INT_DEF(Proxy, COM_DROP_DB)
GETTER_CONST_INT_DEF(Proxy, COM_REFRESH)
GETTER_CONST_INT_DEF(Proxy, COM_SHUTDOWN)
GETTER_CONST_INT_DEF(Proxy, COM_STATISTICS)
GETTER_CONST_INT_DEF(Proxy, COM_PROCESS_INFO)
GETTER_CONST_INT_DEF(Proxy, COM_CONNECT)
GETTER_CONST_INT_DEF(Proxy, COM_PROCESS_KILL)
GETTER_CONST_INT_DEF(Proxy, COM_DEBUG)
GETTER_CONST_INT_DEF(Proxy, COM_PING)
GETTER_CONST_INT_DEF(Proxy, COM_TIME)
GETTER_CONST_INT_DEF(Proxy, COM_DELAYED_INSERT)
GETTER_CONST_INT_DEF(Proxy, COM_CHANGE_USER)
GETTER_CONST_INT_DEF(Proxy, COM_BINLOG_DUMP)
GETTER_CONST_INT_DEF(Proxy, COM_TABLE_DUMP)
GETTER_CONST_INT_DEF(Proxy, COM_CONNECT_OUT)
GETTER_CONST_INT_DEF(Proxy, COM_REGISTER_SLAVE)
GETTER_CONST_INT_DEF(Proxy, COM_STMT_PREPARE)
GETTER_CONST_INT_DEF(Proxy, COM_STMT_EXECUTE)
GETTER_CONST_INT_DEF(Proxy, COM_STMT_SEND_LONG_DATA)
GETTER_CONST_INT_DEF(Proxy, COM_STMT_CLOSE)
GETTER_CONST_INT_DEF(Proxy, COM_STMT_RESET)
GETTER_CONST_INT_DEF(Proxy, COM_SET_OPTION)
#if MYSQL_VERSION_ID >= 50000
GETTER_CONST_INT_DEF(Proxy, COM_STMT_FETCH)
#if MYSQL_VERSION_ID >= 50100
GETTER_CONST_INT_DEF(Proxy, COM_DAEMON)
#endif
#endif

GETTER_CONST_INT_DEF(Proxy, MYSQL_TYPE_DECIMAL)
#if MYSQL_VERSION_ID >= 50000
GETTER_CONST_INT_DEF(Proxy, MYSQL_TYPE_NEWDECIMAL)
#endif
GETTER_CONST_INT_DEF(Proxy, MYSQL_TYPE_TINY)
GETTER_CONST_INT_DEF(Proxy, MYSQL_TYPE_SHORT)
GETTER_CONST_INT_DEF(Proxy, MYSQL_TYPE_LONG)
GETTER_CONST_INT_DEF(Proxy, MYSQL_TYPE_FLOAT)
GETTER_CONST_INT_DEF(Proxy, MYSQL_TYPE_DOUBLE)
GETTER_CONST_INT_DEF(Proxy, MYSQL_TYPE_NULL)
GETTER_CONST_INT_DEF(Proxy, MYSQL_TYPE_TIMESTAMP)
GETTER_CONST_INT_DEF(Proxy, MYSQL_TYPE_LONGLONG)
GETTER_CONST_INT_DEF(Proxy, MYSQL_TYPE_INT24)
GETTER_CONST_INT_DEF(Proxy, MYSQL_TYPE_DATE)
GETTER_CONST_INT_DEF(Proxy, MYSQL_TYPE_TIME)
GETTER_CONST_INT_DEF(Proxy, MYSQL_TYPE_DATETIME)
GETTER_CONST_INT_DEF(Proxy, MYSQL_TYPE_YEAR)
GETTER_CONST_INT_DEF(Proxy, MYSQL_TYPE_NEWDATE)
GETTER_CONST_INT_DEF(Proxy, MYSQL_TYPE_ENUM)
GETTER_CONST_INT_DEF(Proxy, MYSQL_TYPE_SET)
GETTER_CONST_INT_DEF(Proxy, MYSQL_TYPE_TINY_BLOB)
GETTER_CONST_INT_DEF(Proxy, MYSQL_TYPE_MEDIUM_BLOB)
GETTER_CONST_INT_DEF(Proxy, MYSQL_TYPE_LONG_BLOB)
GETTER_CONST_INT_DEF(Proxy, MYSQL_TYPE_BLOB)
GETTER_CONST_INT_DEF(Proxy, MYSQL_TYPE_VAR_STRING)
GETTER_CONST_INT_DEF(Proxy, MYSQL_TYPE_STRING)
GETTER_CONST_INT_DEF(Proxy, MYSQL_TYPE_GEOMETRY)
#if MYSQL_VERSION_ID >= 50000
GETTER_CONST_INT_DEF(Proxy, MYSQL_TYPE_BIT)
#endif

static PyObject *
Proxy_get_response(Proxy *p, void *closure){
	Py_INCREF(p->response);
	return (PyObject*)p->response;
}
static PyObject *
Proxy_get_queries(Proxy *p, void *closure){
	network_mysqld_con_python_t * con_state = p->con->plugin_con_state;
	if(con_state)
		p->queries = Queries_New(con_state->injected.queries);
	Py_XINCREF(p->queries);
	return (PyObject *)p->queries;
}

static int
Proxy_set_response(Proxy *p, PyObject *value, void *closure){
	assert(value);
	if(!PyDict_Check(value)){
		PyErr_SetString(PyExc_ValueError, "response can only be set to a dict");
		return -1;
	}
	PyObject *key, *v;
	Py_ssize_t pos = 0;

	PyDict_Clear(((Response *)p->response)->dict);
	while(PyDict_Next(value, &pos, &key, &v)){
		/* do something interesting with the values... */
		if(!PyString_Check(key)){
			PyErr_SetString(PyExc_ValueError, "response dict's keys can only "
						"be strings");
			return -1;
		}
		PyObject_SetAttr(p->response, key, v);
	}
	return 0;
}

static PyGetSetDef Proxy_getsets[] = {
	GETTER_DECLEAR(Proxy, PROXY_VERSION)
	//GETTER_DECLEAR(Proxy, PROXY_NO_DECISION)
	GETTER_DECLEAR(Proxy, PROXY_SEND_QUERY)
	GETTER_DECLEAR(Proxy, PROXY_SEND_RESULT)
	//GETTER_DECLEAR(Proxy, PROXY_SEND_INJECTION)
	GETTER_DECLEAR(Proxy, PROXY_IGNORE_RESULT)

	GETTER_DECLEAR(Proxy, MYSQLD_PACKET_OK)
	GETTER_DECLEAR(Proxy, MYSQLD_PACKET_ERR)
	GETTER_DECLEAR(Proxy, MYSQLD_PACKET_RAW)

	GETTER_DECLEAR(Proxy, BACKEND_STATE_UNKNOWN)
	GETTER_DECLEAR(Proxy, BACKEND_STATE_UP)
	GETTER_DECLEAR(Proxy, BACKEND_STATE_DOWN)

	GETTER_DECLEAR(Proxy, BACKEND_TYPE_UNKNOWN)
	GETTER_DECLEAR(Proxy, BACKEND_TYPE_RW)
	GETTER_DECLEAR(Proxy, BACKEND_TYPE_RO)

	GETTER_DECLEAR(Proxy, COM_SLEEP)
	GETTER_DECLEAR(Proxy, COM_QUIT)
	GETTER_DECLEAR(Proxy, COM_INIT_DB)
	GETTER_DECLEAR(Proxy, COM_QUERY)
	GETTER_DECLEAR(Proxy, COM_FIELD_LIST)
	GETTER_DECLEAR(Proxy, COM_CREATE_DB)
	GETTER_DECLEAR(Proxy, COM_DROP_DB)
	GETTER_DECLEAR(Proxy, COM_REFRESH)
	GETTER_DECLEAR(Proxy, COM_SHUTDOWN)
	GETTER_DECLEAR(Proxy, COM_STATISTICS)
	GETTER_DECLEAR(Proxy, COM_PROCESS_INFO)
	GETTER_DECLEAR(Proxy, COM_CONNECT)
	GETTER_DECLEAR(Proxy, COM_PROCESS_KILL)
	GETTER_DECLEAR(Proxy, COM_DEBUG)
	GETTER_DECLEAR(Proxy, COM_PING)
	GETTER_DECLEAR(Proxy, COM_TIME)
	GETTER_DECLEAR(Proxy, COM_DELAYED_INSERT)
	GETTER_DECLEAR(Proxy, COM_CHANGE_USER)
	GETTER_DECLEAR(Proxy, COM_BINLOG_DUMP)
	GETTER_DECLEAR(Proxy, COM_TABLE_DUMP)
	GETTER_DECLEAR(Proxy, COM_CONNECT_OUT)
	GETTER_DECLEAR(Proxy, COM_REGISTER_SLAVE)
	GETTER_DECLEAR(Proxy, COM_STMT_PREPARE)
	GETTER_DECLEAR(Proxy, COM_STMT_EXECUTE)
	GETTER_DECLEAR(Proxy, COM_STMT_SEND_LONG_DATA)
	GETTER_DECLEAR(Proxy, COM_STMT_CLOSE)
	GETTER_DECLEAR(Proxy, COM_STMT_RESET)
	GETTER_DECLEAR(Proxy, COM_SET_OPTION)
#if MYSQL_VERSION_ID >= 50000
	GETTER_DECLEAR(Proxy, COM_STMT_FETCH)
#if MYSQL_VERSION_ID >= 50100
	GETTER_DECLEAR(Proxy, COM_DAEMON)
#endif
#endif

 GETTER_DECLEAR(Proxy, MYSQL_TYPE_DECIMAL)
#if MYSQL_VERSION_ID >= 50000
 GETTER_DECLEAR(Proxy, MYSQL_TYPE_NEWDECIMAL)
#endif
 GETTER_DECLEAR(Proxy, MYSQL_TYPE_TINY)
 GETTER_DECLEAR(Proxy, MYSQL_TYPE_SHORT)
 GETTER_DECLEAR(Proxy, MYSQL_TYPE_LONG)
 GETTER_DECLEAR(Proxy, MYSQL_TYPE_FLOAT)
 GETTER_DECLEAR(Proxy, MYSQL_TYPE_DOUBLE)
 GETTER_DECLEAR(Proxy, MYSQL_TYPE_NULL)
 GETTER_DECLEAR(Proxy, MYSQL_TYPE_TIMESTAMP)
 GETTER_DECLEAR(Proxy, MYSQL_TYPE_LONGLONG)
 GETTER_DECLEAR(Proxy, MYSQL_TYPE_INT24)
 GETTER_DECLEAR(Proxy, MYSQL_TYPE_DATE)
 GETTER_DECLEAR(Proxy, MYSQL_TYPE_TIME)
 GETTER_DECLEAR(Proxy, MYSQL_TYPE_DATETIME)
 GETTER_DECLEAR(Proxy, MYSQL_TYPE_YEAR)
 GETTER_DECLEAR(Proxy, MYSQL_TYPE_NEWDATE)
 GETTER_DECLEAR(Proxy, MYSQL_TYPE_ENUM)
 GETTER_DECLEAR(Proxy, MYSQL_TYPE_SET)
 GETTER_DECLEAR(Proxy, MYSQL_TYPE_TINY_BLOB)
 GETTER_DECLEAR(Proxy, MYSQL_TYPE_MEDIUM_BLOB)
 GETTER_DECLEAR(Proxy, MYSQL_TYPE_LONG_BLOB)
 GETTER_DECLEAR(Proxy, MYSQL_TYPE_BLOB)
 GETTER_DECLEAR(Proxy, MYSQL_TYPE_VAR_STRING)
 GETTER_DECLEAR(Proxy, MYSQL_TYPE_STRING)
 GETTER_DECLEAR(Proxy, MYSQL_TYPE_GEOMETRY)
#if MYSQL_VERSION_ID >= 50000
 GETTER_DECLEAR(Proxy, MYSQL_TYPE_BIT)
#endif

	GETSET_DECLEAR(Proxy, response)
	GETTER_DECLEAR(Proxy, queries)
	{0}
};

static PyMethodDef Proxy_methods[] = {{0}};
#define OFF(x) offsetof(Proxy, x)
static PyMemberDef Proxy_members[] = {
	{"connection", T_OBJECT, OFF(connection), RO, ""},
	{"globals", T_OBJECT, OFF(globals), RO, ""},
	{NULL}
};
#undef OFF

PY_TYPE_DEF(Proxy)

static void
Proxy_dealloc(Proxy* self){
	Py_XDECREF(self->response);
    Py_XDECREF(self->queries);
    Py_XDECREF(self->connection);
    self->ob_type->tp_free((PyObject*)self);
}

static void Proxy_Type_Ready(void){
	Proxy_Type.tp_dealloc = (destructor)Proxy_dealloc;
}

PyObject *Proxy_New(network_mysqld_con *con){
	Proxy *proxy = (Proxy*)PyObject_New(Proxy, &Proxy_Type);
	if(!proxy)
		return NULL;
	proxy->con = con;
	proxy->response = Response_New();
	if(!proxy->response)
		goto failed;
	proxy->queries = NULL;
	proxy->globals = NULL;
	proxy->connection = NULL;
	if(!con)
		return (PyObject *)proxy;

	proxy->globals = Globals_New(con->config, con->srv->priv->backends);
	proxy->connection = Connection_New(con);
	if(!proxy->connection)
		goto failed;
	return (PyObject *)proxy;
failed:
	Py_DECREF(proxy);
	return NULL;
}

//------------------------------------Queue------------------------------
static PyObject *
Queue_get_cur_idle_connections(Queue *queue, void *closure){
	return (PyObject *)PyInt_FromLong(queue->queue?queue->queue->length:0);
}

static PyGetSetDef Queue_getsets[] = {
	GETTER_DECLEAR(Queue, cur_idle_connections)
	{0}
};

static PyMemberDef Queue_members[] = {{0}};
static PyMethodDef Queue_methods[] = {{0}};

PY_TYPE_DEF(Queue)

PyObject *
Queue_New(GQueue *q){
	Queue *queue = (Queue *)PyObject_New(Queue, &Queue_Type);
	if(!queue)
		return NULL;
	queue->queue = q;
	return (PyObject*)queue;
}
//-----------------------------Users--------------------------------------
static PyMethodDef Users_methods[] = {{0}};
static PyMemberDef Users_members[] = {{0}};
static PyGetSetDef Users_getsets[] = {{0}};

PY_TYPE_DEF(Users)

static Py_ssize_t
users_length(Users* us){
	return g_hash_table_size(us->pool->users);
}
static int
users_contains(Users* us, PyObject *key){
	if(!PyString_Check(key)){
		PyErr_SetString(PyExc_TypeError, "key must be string");
		return -1;
	}
	GQueue *queue = network_connection_pool_get_conns(us->pool,
				g_string_new(PyString_AsString(key)), NULL);
	if(queue)
		return 1;
	else
		return 0;
}
static PySequenceMethods users_as_sequence = {
	(lenfunc)users_length,
	0,		/* sq_concat */
	0,		/* sq_repeat */
	0,		/* sq_item */
	0,		/* sq_slice */
	0,		/* sq_ass_item */
	0,	/* sq_ass_slice */
	(objobjproc)users_contains  /* sq_contains */
};
static PyObject *
users_subscript(Users *us, PyObject *item){
	if(!PyString_Check(item)){
		PyErr_SetString(PyExc_TypeError, "index must be string");
		return NULL;
	}
	GQueue *queue = network_connection_pool_get_conns(us->pool,
				g_string_new(PyString_AsString(item)), NULL);
	if(queue)
		return Queue_New(queue);
	PyErr_SetString(PyExc_ValueError, "No such user.");
	return NULL;
}
void users_traverse_func(GString *key, GQueue *entries, void *user_data){
	PyObject *list = (PyObject *)user_data;
	assert(PyList_Check(list));
	PyObject *name = PyString_FromStringAndSize(key->str, key->len);
	if(!name)
		return;
	if(PyList_Append(list, name)){
		Py_DECREF(name);
		return;
	}
	Py_DECREF(name);
}

static PyObject *
users_repr(Users *us){
	PyObject *list = PyList_New(0);
	if(!list)
		return NULL;
	g_hash_table_foreach(us->pool->users, (GHFunc)users_traverse_func, list);
	PyObject *result = PyObject_Str(list);
	if(!result){
		Py_DECREF(list);
		return NULL;
	}
	Py_DECREF(list);
	return result;
}
static PyMappingMethods users_as_mapping = {
	0,
	(binaryfunc)users_subscript
};
static void Users_Type_Ready(void){
	Users_Type.tp_as_sequence = &users_as_sequence;
	Users_Type.tp_as_mapping = &users_as_mapping;
	Users_Type.tp_repr = (reprfunc)users_repr;
	Users_Type.tp_flags |= Py_TPFLAGS_HAVE_SEQUENCE_IN;
}
PyObject *Users_New(network_connection_pool *pool){
	Users *users = (Users*)PyObject_New(Users, &Users_Type);
	if(!users)
		return NULL;
	users->pool = pool;
	return (PyObject *)users;
}
//-------------------------------ConnectionPool-----------------------------
static PyObject *
ConnectionPool_get_min_idle_connections(ConnectionPool *pool, void *closure){
	return PyInt_FromLong(pool->pool->min_idle_connections);
}
static PyObject *
ConnectionPool_get_max_idle_connections(ConnectionPool *pool, void *closure){
	return PyInt_FromLong(pool->pool->max_idle_connections);
}
static int
ConnectionPool_set_min_idle_connections(ConnectionPool *pool,
			PyObject *value, void *closure){
	if(!value){
		PyErr_SetString(PyExc_ValueError, "Cannot delete ConnectionPool."
					"min_idle_connections");
		return -1;
	}
	if(!PyInt_Check(value)){
		PyErr_SetString(PyExc_ValueError, "ConnectionPool.min_idle_connections "
					"can only be set to int values");
		return -1;
	}
	pool->pool->min_idle_connections = PyInt_AsLong(value);
	return 0;
}

static int
ConnectionPool_set_max_idle_connections(ConnectionPool *pool,
			PyObject *value, void *closure){
	if(!value){
		PyErr_SetString(PyExc_ValueError, "Cannot delete ConnectionPool."
					"max_idle_connections");
		return -1;
	}
	if(!PyInt_Check(value)){
		PyErr_SetString(PyExc_ValueError, "ConnectionPool.max_idle_connections "
					"can only be set to int values");
		return -1;
	}
	pool->pool->max_idle_connections = PyInt_AsLong(value);
	return 0;
}

static PyGetSetDef ConnectionPool_getsets[] = {
	GETSET_DECLEAR(ConnectionPool, min_idle_connections)
	GETSET_DECLEAR(ConnectionPool, max_idle_connections)
	{0}
};

static PyMemberDef ConnectionPool_members[] = {
	{"users", T_OBJECT, offsetof(ConnectionPool, users), RO, ""},
	{0}
};
static PyMethodDef ConnectionPool_methods[] = {{0}};

PY_TYPE_DEF(ConnectionPool)
PyObject *ConnectionPool_New(network_connection_pool *pool){
	ConnectionPool *cp = PyObject_New(ConnectionPool, &ConnectionPool_Type);
	if(!cp)
		return NULL;
	cp->pool = pool;
	cp->users = Users_New(pool);
	if(!cp->users){
		Py_DECREF(cp);
		return NULL;
	}
	return (PyObject *)cp;
}
//------------------------------------Backend------------------------------
static PyObject *
Backend_get_connected_clients(Backend *b, void *closure){
	return PyInt_FromLong(b->backend->connected_clients);
}

static PyObject *
Backend_get_state(Backend *b, void *closure){
	return PyInt_FromLong(b->backend->state);
}
static int
Backend_set_state(Backend *b, PyObject *value, void *closure){
	assert(value);
	if(!PyInt_Check(value)){
		PyErr_SetString(PyExc_ValueError, "backend.state can only be assigned"
					" to integer.");
		return -1;
	}
	b->backend->state = PyInt_AsLong(value);
	return 0;
}

static PyObject *
Backend_get_type(Backend *b, void *closure){
	return PyInt_FromLong(b->backend->type);
}

static PyObject *
Backend_get_uuid(Backend *b, void *closure){
	if(b->backend->uuid->len)
		return PyString_FromString(b->backend->uuid->str);
	Py_RETURN_NONE;
}
static int
Backend_set_uuid(Backend *b, PyObject *value, void *closure){
	assert(value);
	if(!PyString_Check(value)){
		PyErr_SetString(PyExc_ValueError, "backend.uuid can only be assigned "
					"to string.");
		return -1;
	}
	g_string_assign_len(b->backend->uuid, PyString_AsString(value),
				PyString_Size(value));
	return 0;
}

static PyGetSetDef Backend_getsets[] = {
	GETTER_DECLEAR(Backend, connected_clients)
	GETSET_DECLEAR(Backend, state)
	GETTER_DECLEAR(Backend, type)
	GETSET_DECLEAR(Backend, uuid)
	{0}
};

static PyMethodDef Backend_methods[] = {{0}};
#define OFF(x) offsetof(Backend, x)
static PyMemberDef Backend_members[] = {
	{"dst", T_OBJECT, OFF(dst), RO, ""},
	{"pool", T_OBJECT, OFF(pool), RO, ""},
	{0}
};
#undef OFF

PY_TYPE_DEF(Backend)

PyObject *Backend_New(network_backend_t *backend){
	Backend *b = (Backend*)PyObject_New(Backend, &Backend_Type);
	if(!b)
		return NULL;
	b->backend = backend;
	b->dst = Address_New(backend->addr);
	if(!b->dst)
		goto failed;
	b->pool = ConnectionPool_New(b->backend->pool);
	if(!b->pool)
		goto failed;
	return (PyObject *)b;
failed:
	Py_DECREF(b);
	return NULL;
}
//------------------------------------Address------------------------------
static PyObject *
Address_get_name(Address *addr, void *closure){
	return PyString_FromString(addr->address->name->str);
}

static PyObject *
Address_get_type(Address *addr, void *closure){
	return PyInt_FromLong(addr->address->addr.common.sa_family);
}
static PyObject *
Address_get_port(Address *addr, void *closure){
	switch(addr->address->addr.common.sa_family){
	case AF_INET:
		return PyInt_FromLong(ntohs(addr->address->addr.ipv4.sin_port));
		break;
	case AF_INET6:
		return PyInt_FromLong(ntohs(addr->address->addr.ipv6.sin6_port));
		break;
	default:
		Py_RETURN_NONE;
	}
}
static PyObject *
Address_get_address(Address *address, void *closure){
	network_address *addr = address->address;
#ifdef HAVE_INET_NTOP
	char dst_addr[INET6_ADDRSTRLEN];
#endif
	const char *str = NULL;

	switch (addr->addr.common.sa_family) {
	case AF_INET:
		str = inet_ntoa(addr->addr.ipv4.sin_addr);
		break;
#ifdef HAVE_INET_NTOP
	case AF_INET6:
		str = inet_ntop(addr->addr.common.sa_family, &addr->addr.ipv6.sin6_addr,
					dst_addr, sizeof(dst_addr));
		break;
#endif
#ifndef WIN32
	case AF_UNIX:
		str = addr->addr.un.sun_path;
		break;
#endif
	default:
		break;
	}
	if (NULL == str)
		Py_RETURN_NONE;
	return PyString_FromString(str);
}
static PyGetSetDef Address_getsets[] = {
	GETTER_DECLEAR(Address, name)
	GETTER_DECLEAR(Address, type)
	GETTER_DECLEAR(Address, port)
	GETTER_DECLEAR(Address, address)
	{0}
};
static PyMemberDef Address_members[] = {{0}};
static PyMethodDef Address_methods[] = {{0}};

PY_TYPE_DEF(Address)

PyObject *Address_New(network_address *addr){
	Address *address = (Address *)PyObject_New(Address, &Address_Type);
	if(!address)
		return NULL;
	address->address = addr;
	return (PyObject *)address;
}

//------------------------------------Config------------------------------

#define CONFIG_GETTER_MEMBER_STRING_DEF(name) \
static PyObject *\
Config_get_ ## name(Config *obj, void *closure){\
	return (PyObject *)PyString_FromString(obj->config->name);\
}

#define CONFIG_GETTER_MEMBER_INT_DEF(name) \
static PyObject *\
Config_get_ ## name(Config *obj, void *closure){\
	return (PyObject *)PyInt_FromLong(obj->config->name);\
}

CONFIG_GETTER_MEMBER_STRING_DEF(address)
//GETTER_MEMBER_DEF(Config, backend_addresses)
//GETTER_MEMBER_DEF(Config, read_only_backend_addresses)
CONFIG_GETTER_MEMBER_INT_DEF(fix_bug_25371)
CONFIG_GETTER_MEMBER_INT_DEF(profiling)
CONFIG_GETTER_MEMBER_STRING_DEF(python_script)
CONFIG_GETTER_MEMBER_INT_DEF(pool_change_user)
CONFIG_GETTER_MEMBER_INT_DEF(start_proxy)

static PyGetSetDef Config_getsets[] = {
	GETTER_DECLEAR(Config, address)
	//GETTER_DECLEAR(Config, backend_addresses)
	//GETTER_DECLEAR(Config, read_only_backend_addresses)
	GETTER_DECLEAR(Config, fix_bug_25371)
	GETTER_DECLEAR(Config, profiling)
	GETTER_DECLEAR(Config, python_script)
	GETTER_DECLEAR(Config, pool_change_user)
	GETTER_DECLEAR(Config, start_proxy)
	{0}
};
#define OFF(x) offsetof(Config, x)
static PyMemberDef Config_members[] = {
	{"backend_addresses", T_OBJECT, OFF(backend_addresses), RO, ""},
	{"read_only_backend_addresses", T_OBJECT, OFF(read_only_backend_addresses),
		RO, ""},
	{0}
};
#undef OFF
static PyMethodDef Config_methods[] = {{0}};

static void
Config_dealloc(Config* self){
    Py_XDECREF(self->backend_addresses);
    Py_XDECREF(self->read_only_backend_addresses);
    Py_XDECREF(self->dict);
    self->ob_type->tp_free((PyObject*)self);
}

PY_TYPE_DEF(Config);

static void Config_Type_Ready(void){
	Config_Type.tp_dealloc = (destructor)Config_dealloc;
	Config_Type.tp_getattro = PyObject_GenericGetAttr;
	Config_Type.tp_setattro = PyObject_GenericSetAttr;
	Config_Type.tp_dictoffset = offsetof(Config, dict);
}

PyObject *Config_New(chassis_plugin_config *config){
	Config *conf = (Config *)PyObject_New(Config, &Config_Type);
	if(!conf)
		return NULL;
	conf->config = config;
	int i = 0, j = 0;
	if(config->backend_addresses)
		for (; config->backend_addresses[i]; i++) {}
	conf->backend_addresses = PyTuple_New(i);
	if(!conf->backend_addresses){
		Py_DECREF(conf);
		return NULL;
	}
	for(j = 0; j < i; j++)
		PyTuple_SetItem(conf->backend_addresses, j,
					PyString_FromString(config->backend_addresses[j]));
	i = j = 0;
	if(config->read_only_backend_addresses)
		for (; config->read_only_backend_addresses[i]; i++) {}
	conf->read_only_backend_addresses = PyTuple_New(i);
	if(!conf->backend_addresses)
		goto failed;
	for(j = 0; j < i; j++)
		PyTuple_SetItem(conf->read_only_backend_addresses, j,
				PyString_FromString(config->read_only_backend_addresses[j]));
	conf->dict = PyDict_New();
	if(!conf->dict)
		goto failed;
	return (PyObject *)conf;
failed:
	Py_DECREF(conf);
	return NULL;
}
//------------------------------------Globals---------------------------------
static PyGetSetDef Globals_getsets[] = {{0}};
#define OFF(x) offsetof(Globals, x)
static PyMemberDef Globals_members[] = {
	{"backends", T_OBJECT, OFF(backends), RO, ""},
	{"config", T_OBJECT, OFF(config), RO, ""},
	{0}
};
#undef OFF
static PyMethodDef Globals_methods[] = {{0}};

static void
Globals_dealloc(Globals* self){
    Py_XDECREF(self->backends);
    Py_XDECREF(self->config);
    Py_XDECREF(self->dict);
    self->ob_type->tp_free((PyObject*)self);
}

PY_TYPE_DEF(Globals)
static void Globals_Type_Ready(void){
	Globals_Type.tp_dealloc = (destructor)Globals_dealloc;
	Globals_Type.tp_getattro = PyObject_GenericGetAttr;
	Globals_Type.tp_setattro = PyObject_GenericSetAttr;
	Globals_Type.tp_dictoffset = offsetof(Globals, dict);
}
PyObject *Globals_New(chassis_plugin_config* config,
			network_backends_t *backends){
	int i;
	Globals *global = PyObject_New(Globals, &Globals_Type);
	if(!global)
		return NULL;
	global->backends = PyTuple_New(network_backends_count(backends));
	if(!global->backends)
		return NULL;
	for(i = 0; i < network_backends_count(backends); i++)
		if(PyTuple_SetItem(global->backends, i,
						Backend_New(network_backends_get(backends, i))))
			return NULL;
	global->config = (PyObject *)Config_New(config);
	if(!global->config)
		goto failed;
	global->dict = PyDict_New();
	if(!global->dict)
		goto failed;
	return (PyObject *)global;
failed:
	Py_DECREF(global);
	return NULL;
}
//------------------------------------Connection------------------------------
static PyObject *
Connection_get_backend_ndx(Connection *c, void *closure){
	network_mysqld_con_python_t *con_state = c->con->plugin_con_state;
	return PyInt_FromLong(con_state->backend_ndx);
}
static int
Connection_set_backend_ndx(Connection *c, PyObject *value, void *closure){
	assert(value);
	if(!PyInt_Check(value)){
		PyErr_SetString(PyExc_ValueError, "connection.backend_ndx can only be "
					"assigned to a int.");
		return -1;
	}
	int be_ndx = PyInt_AsLong(value);
	network_socket *send_sock;
	network_mysqld_con_python_t * st = c->con->plugin_con_state;
	if(be_ndx == -1)
		network_connection_pool_python_add_connection(c->con);
	else if((send_sock = network_connection_pool_python_swap(c->con, be_ndx)))
		c->con->server = send_sock;
	else
		st->backend_ndx = be_ndx;
	return 0;
}

static PyObject *
Connection_get_connection_close(Connection *conn, void *closure){
	PyErr_SetString(PyExc_ValueError,
				"connection.connection_close can only be set");
	return NULL;
}

static int
Connection_set_connection_close(Connection *conn, PyObject *value,
			void *closure){
	assert(value);
	if(!PyInt_Check(value)){
		PyErr_SetString(PyExc_ValueError, "connection.connection_close can "
					"only be assigned to True or False");
		return -1;
	}
	int value_i = PyInt_AsLong(value);
	network_mysqld_con_python_t *st = conn->con->plugin_con_state;
	st->connection_close = value_i ? 1 : 0;
	return 0;
}

static PyObject *
Connection_get_server(Connection *conn, void *closuer){
	return Socket_New(conn->con->server);
}

static PyObject *
Connection_get_client(Connection *conn, void *closuer){
	return Socket_New(conn->con->client);
}

static PyGetSetDef Connection_getsets[] = {
	GETSET_DECLEAR(Connection, backend_ndx)
	GETSET_DECLEAR(Connection, connection_close)
	GETTER_DECLEAR(Connection, server)
	GETTER_DECLEAR(Connection, client)
	{0}
};
#define OFF(x) offsetof(Connection, x)
static PyMemberDef Connection_members[] = {{0}};
#undef OFF
static PyMethodDef Connection_methods[] = {{0}};

PY_TYPE_DEF(Connection)
PyObject *Connection_New(network_mysqld_con *con){
	Connection *conn = (Connection *)PyObject_New(Connection, &Connection_Type);
	if(!conn)
		return NULL;
	conn->con = con;
	return (PyObject *)conn;
}
//--------------------------Socket-----------------------------
static PyObject *
Socket_get_src(Socket *s, void *closure){
	if(s->socket->src)
		return Address_New(s->socket->src);
	Py_RETURN_NONE;
}
static PyObject *
Socket_get_dst(Socket *s, void *closure){
	if(s->socket->dst)
		return Address_New(s->socket->dst);
	Py_RETURN_NONE;
}
static PyObject *
Socket_get_default_db(Socket *s, void *closure){
	char *db = s->socket->default_db->str;
	return PyString_FromString(db?db:"");
}
static PyObject *
Socket_get_mysqld_version(Socket *s, void *closure){
	if(s->socket->challenge){
		return PyInt_FromLong(s->socket->challenge->server_version);
	}
	PyErr_SetString(PyExc_ValueError, "Only the server side has "
				"socket.mysqld_version");
	return NULL;
}
static PyObject *
Socket_get_thread_id(Socket *s, void *closure){
	if(s->socket->challenge)
		return PyInt_FromLong(s->socket->challenge->thread_id);
	PyErr_SetString(PyExc_ValueError, "Only the server side has "
				"socket.thread_id");
	return NULL;
}
static PyObject *
Socket_get_scramble_buffer(Socket *s, void *closure){
	if(s->socket->challenge)
		return PyString_FromString(s->socket->challenge->challenge->str);
	PyErr_SetString(PyExc_ValueError, "Only the server side has "
				"socket.scramble_buffer");
	return NULL;
}
static PyObject *
Socket_get_username(Socket *s, void *closure){
	if(s->socket->response)
		return PyString_FromString(s->socket->response->username->str);
	PyErr_SetString(PyExc_ValueError, "Only the response has "
				"socket.username");
	return NULL;
}
static PyObject *
Socket_get_scrambled_password(Socket *s, void *closure){
	if(s->socket->response)
		return PyString_FromString(s->socket->response->response->str);
	PyErr_SetString(PyExc_ValueError, "Only the response has "
				"socket.scrambled_password");
	return NULL;
}

static PyGetSetDef Socket_getsets[] = {
	GETTER_DECLEAR(Socket, src)
	GETTER_DECLEAR(Socket, dst)
	GETTER_DECLEAR(Socket, default_db)
	GETTER_DECLEAR(Socket, mysqld_version)
	GETTER_DECLEAR(Socket, thread_id)
	GETTER_DECLEAR(Socket, scramble_buffer)
	GETTER_DECLEAR(Socket, username)
	GETTER_DECLEAR(Socket, scrambled_password)
	{0}
};
#define OFF(x) offsetof(Socket, x)
static PyMemberDef Socket_members[] = {
	{0}
};
#undef OFF
static PyMethodDef Socket_methods[] = {{0}};

PY_TYPE_DEF(Socket)

PyObject *Socket_New(network_socket *socket){
	Socket *s = (Socket *)PyObject_New(Socket, &Socket_Type);
	if(!s)
		return NULL;
	s->socket = socket;
	return (PyObject *)s;
}

//-------------------------Backends----------------------
static PyMemberDef Backends_members[] = {{0}};
static PyGetSetDef Backends_getsets[] = {{0}};
static PyMethodDef Backends_methods[] = {{0}};

PY_TYPE_DEF(Backends)
static Py_ssize_t
backends_length(Backends* b){
	return network_backends_count(b->backends);
}
static PySequenceMethods backends_as_sequence = {
	(lenfunc)backends_length
};
static PyObject *
backends_subscript(Backends *b, PyObject *item){
	if(!PyInt_Check(item)){
		PyErr_SetString(PyExc_TypeError, "index must be integers");
		return NULL;
	}
	int i = PyInt_AsLong(item);
	if(i < 0 || i >= network_backends_count(b->backends)){
		PyErr_SetString(PyExc_TypeError, "index out of range");
		return NULL;
	}
	return Backend_New(network_backends_get(b->backends, i));
}
static PyMappingMethods backends_as_mapping = {
	0,
	(binaryfunc)backends_subscript
};
static void Backends_Type_Ready(void){
	Backends_Type.tp_as_sequence = &backends_as_sequence;
	Backends_Type.tp_as_mapping = &backends_as_mapping;
}
PyObject *
Backends_New(network_backends_t *backends){
	Backends *b = (Backends*)PyObject_New(Backends, &Backends_Type);
	if(!b)
		return NULL;
	b->backends = backends;
	return (PyObject*)b;
}
//----------------------------ResponseResultset-----------------------
GETTER_MEMBER_DEF(ResponseResultset, fields)
GETTER_MEMBER_DEF(ResponseResultset, rows)
static int
ResponseResultset_set_fields(ResponseResultset *rr, PyObject *value,
			void *closure){
	assert(value);
	if(!PySequence_Check(value)){
		PyErr_SetString(PyExc_ValueError,
					"resultset.fields can only be set to sequence");
		return -1;
	}
	Py_INCREF(value);
	rr->fields = (PyObject *)value;
	return 0;
}
static int
ResponseResultset_set_rows(ResponseResultset *rr, PyObject *value,
			void *closure){
	assert(value);
	if(!PySequence_Check(value)){
		PyErr_SetString(PyExc_ValueError,
					"resultset.rows can only be set to sequence");
		return -1;
	}
	Py_INCREF(value);
	rr->rows = (PyObject *)value;
	return 0;
}
static PyGetSetDef ResponseResultset_getsets[] = {
	GETSET_DECLEAR(ResponseResultset, fields)
	GETSET_DECLEAR(ResponseResultset, rows)
	{0}
};
static PyMethodDef ResponseResultset_methods[] = {{0}};
static PyMemberDef ResponseResultset_members[] = {{0}};

PY_TYPE_DEF(ResponseResultset)

PyObject *
ResponseResultset_New(PyObject *fields, PyObject *rows){
	ResponseResultset *rr = (ResponseResultset *)PyObject_New(ResponseResultset,
				&ResponseResultset_Type);
	if(!rr)
		return NULL;
	if(!PySequence_Check(fields)){
		PyErr_SetString(PyExc_ValueError, "resultset.fields must be sequence");
		return NULL;
	}
	if(!PySequence_Check(rows)){
		PyErr_SetString(PyExc_ValueError, "resultset.rows must be sequence");
		return NULL;
	}
	Py_INCREF(fields);
	Py_INCREF(rows);
	rr->fields = fields;
	rr->rows = rows;
	return (PyObject *)rr;
}
//-------------------------Response----------------------

/*
#define RESPONSE_GETTER_MEMBER_DEF(name) \
static PyObject *\
Response_get_ ## name(Response *r, void *closure){\
	PyObject *res = PyDict_GetItemString(r->dict, #name);\
	if(!res){\
		PyErr_SetString(PyExc_AttributeError,\
					"Response object has no attribute " #name);\
		return NULL;\
	}\
	Py_INCREF(res);\
	return res;\
}
*/

static PyGetSetDef Response_getsets[] = {{0}};
static PyMemberDef Response_members[] = {{0}};
static PyMethodDef Response_methods[] = {{0}};

PY_TYPE_DEF(Response)

static void
Response_dealloc(Response* self){
    Py_DECREF(self->dict);
    self->ob_type->tp_free((PyObject*)self);
}

static void Response_Type_Ready(void){
	Response_Type.tp_dealloc = (destructor)Response_dealloc;
	Response_Type.tp_getattro = PyObject_GenericGetAttr;
	Response_Type.tp_setattro = PyObject_GenericSetAttr;
	Response_Type.tp_dictoffset = offsetof(Response, dict);
}

PyObject *Response_New(void){
	Response *response = (Response *)PyObject_New(Response, &Response_Type);
	if(!response)
		return NULL;
	response->dict = PyDict_New();
	if(!response->dict){
		Py_DECREF(response);
		return NULL;
	}
	return (PyObject *)response;
}
//--------------------------------Auth---------------------------
GETTER_MEMBER_DEF(Auth, packet)

static PyMethodDef Auth_methods[] = {{0}};
static PyMemberDef Auth_members[] = {{0}};
static PyGetSetDef Auth_getsets[] = {
	GETTER_DECLEAR(Auth, packet)
	{0}
};
PY_TYPE_DEF(Auth)
PyObject *
Auth_New(const char* packet, int len){
	Auth *auth = (Auth*)PyObject_New(Auth, &Auth_Type);
	if(!auth)
		return NULL;
	auth->packet = PyString_FromStringAndSize(packet, len);
	if(!auth->packet){
		Py_DECREF(auth);
		return NULL;
	}
	return (PyObject *)auth;
}
//--------------------------------ProxyField---------------------------
static PyObject *
ProxyField_get_type(ProxyField *f, void* closure){
	return PyInt_FromLong(f->field->type);
}
static PyObject *
ProxyField_get_name(ProxyField *f, void* closure){
	return PyString_FromString(f->field->name);
}
static PyObject *
ProxyField_get_org_name(ProxyField *f, void* closure){
	return PyString_FromString(f->field->org_name);
}
static PyObject *
ProxyField_get_table(ProxyField *f, void* closure){
	return PyString_FromString(f->field->table);
}
static PyObject *
ProxyField_get_org_table(ProxyField *f, void* closure){
	return PyString_FromString(f->field->org_table);
}
static PyMemberDef ProxyField_members[] = {{0}};
static PyMethodDef ProxyField_methods[] = {{0}};
static PyGetSetDef ProxyField_getsets[] = {
	GETTER_DECLEAR(ProxyField, type)
	GETTER_DECLEAR(ProxyField, name)
	GETTER_DECLEAR(ProxyField, org_name)
	GETTER_DECLEAR(ProxyField, table)
	GETTER_DECLEAR(ProxyField, org_table)
	{0}
};
PY_TYPE_DEF(ProxyField)
PyObject *
ProxyField_New(MYSQL_FIELD *field){
	ProxyField *f = (ProxyField*)PyObject_New(ProxyField, &ProxyField_Type);
	if(!f)
		return NULL;
	f->field = field;
	return (PyObject*)f;
}
//--------------------------------ProxyRows----------------------------
static PyMemberDef ProxyRows_members[] = {{0}};
static PyMethodDef ProxyRows_methods[] = {{0}};
static PyGetSetDef ProxyRows_getsets[] = {{0}};
PY_TYPE_DEF(ProxyRows)
static PyObject *
ProxyRows_Iter(ProxyRows *pr){
	return ProxyRowIter_New(pr->res);
}
static void ProxyRows_Type_Ready(void){
	ProxyRows_Type.tp_iter = (getiterfunc)ProxyRows_Iter;
}
PyObject *
ProxyRows_New(proxy_resultset_t *res){
	ProxyRows *pr = (ProxyRows*)PyObject_New(ProxyRows, &ProxyRows_Type);
	if(!pr)
		return NULL;
	pr->res = res;
	return (PyObject *)pr;
}
//--------------------------------ProxyRowIter------------------------
static PyMemberDef ProxyRowIter_members[] = {{0}};
static PyMethodDef ProxyRowIter_methods[] = {{0}};
static PyGetSetDef ProxyRowIter_getsets[] = {{0}};

PY_TYPE_DEF(ProxyRowIter)
static PyObject *
ProxyRowIter_iternext(ProxyRowIter *pri){
	proxy_resultset_t *res = pri->res;
	network_packet packet;
	GPtrArray *fields = res->fields;
	gsize i;
	int err = 0;
	network_mysqld_lenenc_type lenenc_type;
	GList *chunk = res->row;
	if(chunk == NULL){
		PyErr_SetString(PyExc_ValueError, "No next rows");
		return NULL;
	}

	packet.data = chunk->data;
	packet.offset = 0;

	err = err || network_mysqld_proto_skip_network_header(&packet);
	err = err || network_mysqld_proto_peek_lenenc_type(&packet, &lenenc_type);
	if(err){
		PyErr_SetString(PyExc_ValueError, "Protocol error");
		return NULL;
	}

	switch (lenenc_type) {
	case NETWORK_MYSQLD_LENENC_TYPE_ERR:
		/* a ERR packet instead of real rows
		 *
		 * like "explain select fld3 from t2 ignore index (fld3,not_existing)"
		 *
		 * see mysql-test/t/select.test
		 */
	case NETWORK_MYSQLD_LENENC_TYPE_EOF:
		/* if we find the 2nd EOF packet we are done */
		PyErr_SetNone(PyExc_StopIteration);
		return NULL;
	case NETWORK_MYSQLD_LENENC_TYPE_INT:
	case NETWORK_MYSQLD_LENENC_TYPE_NULL:
		break;
	}

	PyObject *result = PyTuple_New(fields->len);
	if(!result)
		return NULL;

	for (i = 0; i < fields->len; i++) {
		guint64 field_len;

		err = err || network_mysqld_proto_peek_lenenc_type(&packet,
					&lenenc_type);
		if(err){
			PyErr_SetString(PyExc_ValueError, "protocol error!");
			return NULL;
		}

		switch (lenenc_type) {
		case NETWORK_MYSQLD_LENENC_TYPE_NULL:
			Py_INCREF(Py_None);
			PyTuple_SetItem(result, i, Py_None);
			break;
		case NETWORK_MYSQLD_LENENC_TYPE_INT:
			err = err || network_mysqld_proto_get_lenenc_int(&packet,
						&field_len);
			/* just to check that we don't overrun by the addition */
			err = err || !(field_len <= packet.data->len);
			/* check that we have enough string-bytes for the
			 * length-encoded string
			 */
			err = err || !(packet.offset + field_len <= packet.data->len);
			if(err){
				PyErr_SetString(PyExc_ValueError, "row-data is invalid.");
				return NULL;
			}
            PyTuple_SetItem(result, i, PyString_FromStringAndSize(
							packet.data->str + packet.offset, field_len));

			err = err || network_mysqld_proto_skip(&packet, field_len);
			break;
		default:
			/* EOF and ERR should come up here */
			err = 1;
			break;
		}

		if(err){
			PyErr_SetString(PyExc_ValueError, "row-data unknown error.");
			return NULL;
		}
	}

	res->row = res->row->next;
	return result;
}
static void ProxyRowIter_Type_Ready(void){
	ProxyRowIter_Type.tp_iter = PyObject_SelfIter;
	ProxyRowIter_Type.tp_iternext = (iternextfunc)ProxyRowIter_iternext;
}
PyObject *
ProxyRowIter_New(proxy_resultset_t *res){
	ProxyRowIter *pri = (ProxyRowIter*)PyObject_New(ProxyRowIter,
				&ProxyRowIter_Type);
	if(!pri)
		return NULL;
	pri->res = res;
	return (PyObject*)pri;
}
//--------------------------------Flags-----------------------------
static PyObject *
Flags_get_in_trans(Flags *f, void* closure){
	return PyInt_FromLong(f->status & SERVER_STATUS_IN_TRANS);
}
static PyObject *
Flags_get_auto_commit(Flags *f, void* closure){
	return PyInt_FromLong(f->status & SERVER_STATUS_AUTOCOMMIT);
}
static PyObject *
Flags_get_no_good_index_used(Flags *f, void* closure){
	return PyInt_FromLong(f->status & SERVER_QUERY_NO_GOOD_INDEX_USED);
}
static PyObject *
Flags_get_no_index_used(Flags *f, void* closure){
	return PyInt_FromLong(f->status & SERVER_QUERY_NO_INDEX_USED);
}
static PyMemberDef Flags_members[] = {{0}};
static PyMethodDef Flags_methods[] = {{0}};
static PyGetSetDef Flags_getsets[] = {
	GETTER_DECLEAR(Flags, in_trans)
	GETTER_DECLEAR(Flags, auto_commit)
	GETTER_DECLEAR(Flags, no_good_index_used)
	GETTER_DECLEAR(Flags, no_index_used)
	{0}
};
PY_TYPE_DEF(Flags)
PyObject *Flags_New(guint16 status){
	Flags *f = (Flags*)PyObject_New(Flags, &Flags_Type);
	if(!f)
		return NULL;
	f->status = status;
	return (PyObject*)f;
}
//--------------------------------InjectionResultset--------------------
static int parse_resultset_fields(proxy_resultset_t *res) {
	GList *chunk;

	g_return_val_if_fail(res->result_queue != NULL, -1);
	if(NULL == res->result_queue->head)
		return -1;

	if (res->fields) return 0;

   	/* parse the fields */
	res->fields = network_mysqld_proto_fielddefs_new();

	if (!res->fields) return -1;

	chunk = network_mysqld_proto_get_fielddefs(res->result_queue->head,
				res->fields);

	/* no result-set found */
	if (!chunk){
		g_critical("no result set found");
		return -1;
	}

	/* skip the end-of-fields chunk */
	res->rows_chunk_head = chunk->next;

	return 0;
}

static PyObject *
InjectionResultset_get_rows(InjectionResultset *ir, void* closure){
	if(!ir->res->result_queue){
		PyErr_SetString(PyExc_ValueError, ".resultset.rows isn't available if "
					"resultset_is_needed ~= true");
		return NULL;
	}
	else if(ir->res->qstat.binary_encoded){
		PyErr_SetString(PyExc_ValueError, ".resultset.rows isn't available "
					"for prepared statements");
		return NULL;
	}
	if(0 != parse_resultset_fields(ir->res)){
		PyErr_SetString(PyExc_ValueError, "Parse resultset failed!");
		return NULL;
	}
	if(ir->res->rows_chunk_head){
		ir->res->row = ir->res->rows_chunk_head;
		PyObject *proxy_rows = ProxyRows_New(ir->res);
		if(!proxy_rows)
			return NULL;
		return proxy_rows;
	}
	else {
		PyErr_SetString(PyExc_ValueError, "Create rows failed!");
		return NULL;
	}
}

static PyObject *
InjectionResultset_get_row_count(InjectionResultset *ir, void* closure){
	return PyInt_FromLong(ir->res->rows);
}
static PyObject *
InjectionResultset_get_bytes(InjectionResultset *ir, void* closure){
	return PyInt_FromLong(ir->res->bytes);
}
static PyObject *
InjectionResultset_get_raw(InjectionResultset *ir, void* closure){
	if(!ir->res->result_queue){
		PyErr_SetString(PyExc_ValueError, "resultset.raw isn't available if "
					"resultset_is_need ~= true");
		return NULL;
	}
	GString *s = ir->res->result_queue->head->data;
	return PyString_FromStringAndSize(s->str + 4, s->len - 4);
}
static PyObject *
InjectionResultset_get_warning_count(InjectionResultset *ir, void* closure){
	return PyInt_FromLong(ir->res->qstat.warning_count);
}
static PyObject *
InjectionResultset_get_affected_rows(InjectionResultset *ir, void* closure){
	if(ir->res->qstat.was_resultset)
		Py_RETURN_NONE;
	return PyLong_FromLong(ir->res->qstat.affected_rows);
}
static PyObject *
InjectionResultset_get_insert_id(InjectionResultset *ir, void* closure){
	if(ir->res->qstat.was_resultset)
		Py_RETURN_NONE;
	return PyLong_FromLong(ir->res->qstat.insert_id);
}
static PyObject *
InjectionResultset_get_query_status(InjectionResultset *ir, void* closure){
	if(ir->res->qstat.query_status == MYSQLD_PACKET_NULL)
		Py_RETURN_NONE;
	return PyInt_FromLong(ir->res->qstat.query_status);
}
static PyObject *
InjectionResultset_get_fields(InjectionResultset *ir, void *closure){
	proxy_resultset_t *res = ir->res;
	if(!res->result_queue){
		PyErr_SetString(PyExc_AttributeError,
					"resultset.fields isn't available");
		return NULL;
	}
	if(0 != parse_resultset_fields(res)){
		PyErr_SetString(PyExc_ValueError, "parse resultset failed");
		return NULL;
	}
	//TODO Now set the fields and rows;
	if(res->fields){
		PyObject *fields = PyTuple_New(res->fields->len);
		if(!fields)
			return NULL;
		int i;
		for(i = 0; i < res->fields->len; i++)
			PyTuple_SetItem(fields, i, ProxyField_New(
							(MYSQL_FIELD*)g_ptr_array_index(res->fields, i)));
		return fields;
	}
	else {
		PyErr_SetString(PyExc_ValueError, "Create fields failed");
		return NULL;
	}
}

static PyObject *
InjectionResultset_get_flags(InjectionResultset *ir, void *closure){
	return Flags_New(ir->res->qstat.server_status);
}
static PyMemberDef InjectionResultset_members[] = {{0}};
static PyMethodDef InjectionResultset_methods[] = {{0}};
static PyGetSetDef InjectionResultset_getsets[] = {
	GETTER_DECLEAR(InjectionResultset, fields)
	GETTER_DECLEAR(InjectionResultset, rows)
	GETTER_DECLEAR(InjectionResultset, row_count)
	GETTER_DECLEAR(InjectionResultset, bytes)
	GETTER_DECLEAR(InjectionResultset, raw)
	GETTER_DECLEAR(InjectionResultset, flags)
	GETTER_DECLEAR(InjectionResultset, warning_count)
	GETTER_DECLEAR(InjectionResultset, affected_rows)
	GETTER_DECLEAR(InjectionResultset, insert_id)
	GETTER_DECLEAR(InjectionResultset, query_status)
	{0}
};
PY_TYPE_DEF(InjectionResultset)
PyObject *
InjectionResultset_New(proxy_resultset_t *res){
	InjectionResultset *ir = (InjectionResultset*)PyObject_New(InjectionResultset,
				&InjectionResultset_Type);
	if(!ir)
		return NULL;
	ir->res = res;
	return (PyObject*)ir;
}
//--------------------------------Injection-------------------
static PyObject *
Injection_get_id(Injection *i){
	injection *inj = i->inj;
	return PyInt_FromLong(inj->id);
}
static PyObject *
Injection_get_query(Injection *i){
	injection *inj = i->inj;
	return PyString_FromStringAndSize(inj->query->str, inj->query->len);
}
static PyObject *
Injection_get_query_time(Injection *i){
	injection *inj = i->inj;
	return PyInt_FromLong(chassis_calc_rel_microseconds(inj->ts_read_query,
					inj->ts_read_query_result_first));
}
static PyObject *
Injection_get_response_time(Injection *i){
	injection *inj = i->inj;
	return PyInt_FromLong(chassis_calc_rel_microseconds(inj->ts_read_query,
					inj->ts_read_query_result_last));
}
static PyObject *
Injection_get_resultset(Injection *i){
	proxy_resultset_t *res = proxy_resultset_new();
	/* only expose the resultset if really needed
	   FIXME: if the resultset is encoded in binary form, we can't provide it either.
	 */
	if (i->inj->resultset_is_needed && !i->inj->qstat.binary_encoded)
		res->result_queue = i->inj->result_queue;
	res->qstat = i->inj->qstat;
	res->rows  = i->inj->rows;
	res->bytes = i->inj->bytes;
	return InjectionResultset_New(res);
}

static PyMemberDef Injection_members[] = {{0}};
static PyMethodDef Injection_methods[] = {{0}};
static PyGetSetDef Injection_getsets[] = {
	GETTER_DECLEAR(Injection, resultset)
	GETTER_DECLEAR(Injection, id)
	GETTER_DECLEAR(Injection, query)
	GETTER_DECLEAR(Injection, query_time)
	GETTER_DECLEAR(Injection, response_time)
	{0}
};

PY_TYPE_DEF(Injection)

PyObject *
Injection_New(injection *inj){
	Injection *injection = (Injection*)PyObject_New(Injection, &Injection_Type);
	if(!injection)
		return NULL;
	injection->inj = inj;
	return (PyObject *)injection;
}
//-----------------------------Queries--------------------------------
static PyMemberDef Queries_members[] = {{0}};
static PyGetSetDef Queries_getsets[] = {{0}};

static PyObject *
Queries_append(Queries *queries, PyObject *args){
	int resp_type, str_len;
	char *str;
	int rin = FALSE; //resultset_is_needed
	if(!PyArg_ParseTuple(args, "is#|i", &resp_type, &str, &str_len, &rin)){
		return NULL;
	}
	GQueue *q = queries->queries;
	GString *query = g_string_sized_new(str_len);
	g_string_append_len(query, str, str_len);
	injection *inj = injection_new(resp_type, query);
	inj->resultset_is_needed = rin ? 1 : 0;
	network_injection_queue_append(q, inj);
	Py_RETURN_NONE;
}

static PyObject *
Queries_prepend(Queries *queries, PyObject *args){
	int resp_type, str_len;
	char *str;
	int rin = FALSE; //resultset_is_needed
	if(!PyArg_ParseTuple(args, "is#|i", &resp_type, &str, &str_len, &rin))
		return NULL;
	GQueue *q = queries->queries;
	GString *query = g_string_sized_new(str_len);
	g_string_append_len(query, str, str_len);
	injection *inj = injection_new(resp_type, query);
	inj->resultset_is_needed = rin ? 1 : 0;
	network_injection_queue_prepend(q, inj);
	Py_RETURN_NONE;
}

static PyObject *
Queries_reset(Queries *queries){
	GQueue *q = queries->queries;
	network_injection_queue_reset(q);
	Py_RETURN_NONE;
}

static PyObject *
Queries_len(Queries *queries){
	GQueue *q = queries->queries;
	return PyInt_FromLong(q->length);
}

static PyMethodDef Queries_methods[] = {
	{"append", (PyCFunction)Queries_append, METH_VARARGS, ""},
	{"prepend", (PyCFunction)Queries_prepend, METH_VARARGS, ""},
	{"reset", (PyCFunction)Queries_reset, METH_NOARGS, ""},
	{"len", (PyCFunction)Queries_len, METH_NOARGS, ""},
	{0}
};
PY_TYPE_DEF(Queries)
PyObject *
Queries_New(network_injection_queue *queries){
	Queries *q = (Queries*)PyObject_New(Queries, &Queries_Type);
	if(!q)
		return NULL;
	q->queries = queries;
	return (PyObject*)q;
}

//-----------------------------------------------------------------------
#define PY_TYPE_READY(name) \
    if(PyType_Ready(& name ## _Type) < 0)\
        return -1;\
    Py_INCREF(& name ## _Type);\

int init_python_types(void){
	//TODO init_python_types
	Proxy_Type_Ready();
	PY_TYPE_READY(Proxy)
	PY_TYPE_READY(Connection)
	Globals_Type_Ready();
	PY_TYPE_READY(Globals)
	Response_Type_Ready();
	PY_TYPE_READY(Response)
	Users_Type_Ready();
	PY_TYPE_READY(Users)
	Config_Type_Ready();
	PY_TYPE_READY(Config)
	PY_TYPE_READY(Socket)
	PY_TYPE_READY(Backend)
	PY_TYPE_READY(Address)
	PY_TYPE_READY(ConnectionPool)
	PY_TYPE_READY(Queue)
	PY_TYPE_READY(ResponseResultset)
	PY_TYPE_READY(ProxyField)
	PY_TYPE_READY(InjectionResultset)
	PY_TYPE_READY(Auth)
	PY_TYPE_READY(Injection)
	PY_TYPE_READY(ProxyField)
	PY_TYPE_READY(Flags)
	ProxyRows_Type_Ready();
	PY_TYPE_READY(ProxyRows)
	ProxyRowIter_Type_Ready();
	PY_TYPE_READY(ProxyRowIter)
	Backends_Type_Ready();
	PY_TYPE_READY(Backends)
	PY_TYPE_READY(Queries)

	PyObject *proxy_constants = Proxy_New(NULL);
	if(proxy_constants == NULL)
		return -1;

	PyThreadState *tstate = PyThreadState_GET();
	PyObject *sd = tstate->interp->builtins;
	//What is this?
	PyDict_SetItemString(sd, "proxy", proxy_constants);
	Py_DECREF(proxy_constants);

	return 0;
}
