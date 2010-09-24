
import re
#--
#- automatic configure of modules
#-
#- SHOW CONFIG

#module("proxy.auto-config", package.seeall)

#local l = require("lpeg")

#--
#- transform a table into loadable string
def tbl2str(tbl, indent=""):
	s = ""
	for k, v in tbl.items():
		s += indent + ("[%s] : " % k)
		if type(v) == "table" :
			s += "{\n" + tbl2str(v, indent + "  ") + indent + "}"
		elif type(v) == "string" :
			s += "%s" % v
		else:
			s += repr(v)
		s += ",\n"

	return s


#--
#- turn a string into a case-insensitive lpeg-pattern
#-

#WS     = re.compile("[ \t\n]")

#PROXY  = re.compile("PROXY[ \t\n]+", re.I)
#SHOW   = re.compile("SHOW[ \t\n]+", re.I)
#SET    = re.compile("SET[ \t\n]+", re.I)
#GLOBAL = re.compile("GLOBAL[ \t\n]+", re.I)
#CONFIG = re.compile("CONFIG", re.I)
#SAVE   = re.compile("SAVE[ \t\n]+", re.I)
#LOAD   = re.compile("LOAD[ \t\n]+", re.I)
#INTO   = re.compile("INTO[ \t\n]+", re.I)
#FROM   = re.compile("FROM[ \t\n]+", re.I)
#DOT    = re.compile("\.", re.I)
#EQ     = '[ \t\n]*=[ \t\n]*'
#literal = re.compile('[a-zA-Z]+', re.I)
#string_quoted  = re.compile('\"[^\"]*\"', re.I)
#digit  = re.compile("[0-9]", re.I)       #- [0-9]
#number = re.compile('-?\d+', re.I)
#bool   = re.compile("true|false", re.I)


#'PROXY SHOW CONFIG'
#'PROXY SET GLOBAL a.b=value'
#'PROXY SAVE CONFIG INTO "conf"'
#'PROXY LOAD CONFIG FROM "conf"'
#lang =\
#re.compile('\s*proxy\s+((show)\s+config)|((set)\s+global\s+(\w+)\.(\w+)\s*=\s*((true)|(false)|(\"\w+\")|(\d+)))|((save)\s+config\s+into\s+(\".*\"))|((load)\s+config\s+from\s+(\".*\"))\s*', re.I)
lang =\
re.compile('\s*proxy\s+(((show)\s+config)|((set)\s+global\s+(\w+)\.(\w+)\s*=\s*(?P<bool_true>(true)|(?P<bool_false>false)|(?P<string>\"\w+\")|(?P<number>\d+)))|((save)\s+config\s+into\s+(\".*\"))|((load)\s+config\s+from\s+(\".*\")))\s*', re.I)

#local l_proxy = l.Ct(PROXY *
#	((SHOW / "SHOW" * CONFIG) +
#	 (SET  / "SET"  * GLOBAL * l.C(literal) * DOT * l.C(literal) * EQ *\
#	 	l.Ct( l.Cc("string") * l.C(string_quoted) +\
#		      l.Cc("number") * l.C(number) +\
#		      l.Cc("boolean") * l.C(bool) )) +
#	 (SAVE / "SAVE" * CONFIG * WS^1 * INTO * l.C(string_quoted)) +
#	 (LOAD / "LOAD" * CONFIG * WS^1 * FROM * l.C(string_quoted))) * -1)

SET_VALUE_PARSE = {
	'bool_true' : lambda x:True,
	'bool_false' : lambda x:False,
	'string' : str,
	'number' : long,
}

def get_tokens(m, string):
	try:
		groups = m.match(string).groups()
	except AttributeError:
		return None

	groupdict = m.match(string).groupdict()
	start = seq_find(groups[1:], groups[0])
	start += 2
	if groups[start].lower() == 'set':
		for k, v in groupdict.items():
			if v is not None:
				return list(groups[start:start+3]) + [SET_VALUE_PARSE[k](v)]
	return groups[start:]

def seq_find(seq, item):
	for i,it in enumerate(seq):
		if it == item:
			return i

def handle(tbl, proxy, cmd=None):
	#--
	#- support old, deprecated API:
	#-
	#-   auto_config.handle(cmd)
	#-
	#- and map it to
	#-
	#-   proxy.globals.config:handle(cmd)
	if cmd == None and type(tbl.type) in (int, long, float):
		cmd = tbl
		tbl = proxy.globals.config

	#- handle script-options first
	if cmd.type != proxy.COM_QUERY :
		return None

	#- don't try to tokenize log SQL queries
	if len(cmd.query) > 128 :
		return None

	tokens = get_tokens(lang, cmd.query)

	if not tokens :
		return None

	#- print(tbl2str(tokens))

	if tokens[0].upper() == "SET" :
		if not tbl.has_key(tokens[1]) :
			proxy.response = {
				'type' : proxy.MYSQLD_PACKET_ERR,
				'errmsg' : "module not known",
			}
		elif not tbl[tokens[1]].has_key(tokens[2]) :
			proxy.response = {
				'type' : proxy.MYSQLD_PACKET_ERR,
				'errmsg' : "option not known"
			}
		else:
			tbl[tokens[1]][tokens[2]] = tokens[3]
			proxy.response = {
				'type' : proxy.MYSQLD_PACKET_OK,
				'affected_rows' : 1
			}

	elif tokens[0] == "SHOW" :
		rows = []

		for mod, options in tbl.items():
			for option, val in options.items():
				rows.append((mod, options, str(val), type(val)))

		proxy.response = {
			'type' : proxy.MYSQLD_PACKET_OK,
			'resultset' : {
				'fields' : (
					( "module", proxy.MYSQL_TYPE_STRING ),
					( "option", proxy.MYSQL_TYPE_STRING ),
					( "value", proxy.MYSQL_TYPE_STRING ),
					( "type", proxy.MYSQL_TYPE_STRING ),
				),
				'rows' : rows,
			}
		}
	elif tokens[0] == "SAVE" :
		#- save the config into this filename
		filename = tokens[1]
		ret, errmsg = save(tbl, filename)
		if ret:
			proxy.response = {
				'type' : proxy.MYSQLD_PACKET_OK,
				'affected_rows' : 0,
			}
		else:
			proxy.response = {
				'type' : proxy.MYSQLD_PACKET_ERR,
				'errmsg' : errmsg
			}

	elif tokens[0] == "LOAD" :
		filename = tokens[2]

		ret, errmsg =  load(tbl, filename)

		if ret :
			proxy.response = {
				'type' : proxy.MYSQLD_PACKET_OK,
				'affected_rows' : 0
			}
		else:
			proxy.response = {
				'type' : proxy.MYSQLD_PACKET_ERR,
				'errmsg' : errmsg
			}
	else:
		assert(False)

	return proxy.PROXY_SEND_RESULT


def save(tbl, filename):
	content = "{" + tbl2str(tbl, "  ") + "}"

	f = open(filename, "w")

	if not f:
		return False, errmsg

	f.write(content)
	f.close()

	return [True]


def load(tbl, filename):
	func = open(filename).read()

	if not func :
		return False, errmsg

	v = eval(func)

	for mod, options in v.items():
		if tbl[mod]:
			#- weave the loaded options in
			for option, value in options.items():
				tbl[mod][option] = value
		else:
			tbl[mod] = options

	return [True]
