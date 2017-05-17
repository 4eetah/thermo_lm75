all: thermo_lm75 test_cli

thermo_lm75: thermo_lm75.o
	$(CC) -o $@ $^ $(CFLAGS)

test_cli: test_cli.o
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	$(RM) *.o thermo_lm75 test_cli

install:
	cp ./thermo_lm75 /usr/bin/
	cp ./thermo_lm75.cfg /etc/default/
	cp ./thermo_lm75.sh /etc/init.d/
	chmod +x /etc/init.d/thermo_lm75.sh

remove:
	rm /etc/default/thermo_lm75.cfg
	rm /etc/init.d/thermo_lm75.sh
	rm /usr/bin/thermo_lm75
