all: ipk-server ipk-client

ipk-server: server.c
	@gcc -o ipk-server server.c

ipk-client: client.c
	@gcc -o ipk-client client.c

remove:
	@rm -f ipk-client ipk-server