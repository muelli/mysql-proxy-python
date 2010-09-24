
import os, sys
sys.path.append(os.environ['PYTHON_LIBPATH'])

import mysql.proto as proto
import mysql.tokenizer as tokenizer


def connect_server(proxy):
	#- emulate a server
	proxy.response = {
		'type' : proxy.MYSQLD_PACKET_RAW,
		'packets' : (
			proto.to_challenge_packet({}),
		)
	}
	return proxy.PROXY_SEND_RESULT


#-
#- returns the tokens of every query
#-

#- Uncomment the following line if using with MySQL command line.
#- Keep it commented if using inside the test suite
#- local counter = 0

def read_query(proxy, packet):
	if ord(packet[0]) != proxy.COM_QUERY :
		proxy.response = { 'type' : proxy.MYSQLD_PACKET_OK }
		return proxy.PROXY_SEND_RESULT

	query = packet[1:]
    #-
    #- Uncomment the following two lines if using with MySQL command line.
    #- Keep them commented if using inside the test suite
    #- counter = counter + 1
    #- if counter < 3 : return
	tokens = tokenizer.tokenize(query)
	rows = [(token.token_id, token.token_name, token.text) for token in tokens]

	proxy.response.type = proxy.MYSQLD_PACKET_OK
	proxy.response.resultset = {
		'fields' : (
			("id", proxy.MYSQL_TYPE_STRING),
			("name", proxy.MYSQL_TYPE_STRING),
			("text", proxy.MYSQL_TYPE_STRING),
		),
		'rows' : rows
	}

	return proxy.PROXY_SEND_RESULT
