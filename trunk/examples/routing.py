# LICENSE BEGIN
#
# Copyright (c) 2010 Ysj.Ray
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
## LICENSE END

#--
#- print the socket information to all IP based routing
#-
#- * setup one virtual IP per backend server
#- * let the proxy to all those IPs
#- * create a lua-table that maps a "client.dst.name" to a backend

#--
#- print a address 
#-
def address_print(prefix, address):
	print "%s: %s (type = %d, address = %s, port = %d" % (prefix,\
		address.name or None,
		address.type or -1,
		address.address or None,
		address.port or -1)  #- unix-sockets don't have a port

#--
#- print the address of the client side
#-
def connect_server(proxy):
	address_print("client src", proxy.connection.client.src)
	address_print("client dst", proxy.connection.client.dst)

#--
#- print the address of the connected side and trigger a close of the connection
#-
def read_handshake(proxy):
	address_print("server src", proxy.connection.server.src)
	address_print("server dst", proxy.connection.server.dst)

	#- tell the client the server denies us
	proxy.response = {
		'type' : proxy.MYSQLD_PACKET_ERR,
		'errmsg' : "done"
	}

	return proxy.PROXY_SEND_RESULT
