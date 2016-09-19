About
-------

A simple client-server remote shell application written in C.


Build & Run
-----------

`cd /path/to/remote-shell`

`make`

* To run the server

```bash
./server -p <port>
      	 -c </full/path/to/commands_file>
		 -u </full/path/to/users_file>
```

where `port` is the portnumber to which the server listens to (default value is 5000),
`commands_file` is a file containing the allowed commands that the client can
execute on this server and `users_file` is a file containig a list of
username:password fields specifying the users that can access this server.

* To run the client

```bash
./client -h <host>
         -p <port>
         -usr <username>
         -psw <password>
```

where `host` is a hostname or IP address of the server, `port` is the port
the server listens to (dafault value is 5000), `username` and `password`
are the username and password of the user that wants to connect to the server.
