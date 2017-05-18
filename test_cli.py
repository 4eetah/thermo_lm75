import os, sys

if len(sys.argv) < 2:
    print("Usage: %s <cmd>" % sys.argv[0])
    print("Available commands:")
    print("TEMPERATURE - get the temperature in Celcius")
    exit(1);

fifo_client = "/var/run/imsg/rx_pipe"
fifo_server = "/var/run/thermo_lm75/rx_pipe"

if not os.path.exists(fifo_client):
    os.mkfifo(fifo_client)

pipeout = os.open(fifo_server, os.O_WRONLY)
if pipeout == None:
    print("can't open %s" % fifo_server)
    exit(1)

n = os.write(pipeout, sys.argv[1]+"\n")

if n != len(sys.argv[1] + "\n"):
    print("error writing to %s" % fifo_server)
    exit(1)

pipein = open(fifo_client, 'r')
if pipein == None:
    print("can't open %s" % fifo_client);
    exit(1)

line = pipein.readline()
print(line),

os.unlink(fifo_client)
