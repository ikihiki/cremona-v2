build: 
	go build -C cmd/cremona_daemon  -tags netgo -ldflags '-extldflags "-static"'
	go build -C cmd/driver_test  -tags netgo -ldflags '-extldflags "-static"'
install:
	cp cmd/cremona_daemon/cremona_daemon /usr/local/bin/
	cmd/cremona_daemon/cremona_daemon -gen_config 
clean: 
	rm -f cmd/cremona_daemon/cremona_daemon
	rm -f cmd/driver_test/driver_test