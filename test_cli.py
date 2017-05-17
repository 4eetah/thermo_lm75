import os, sys

if len(sys.argv) < 2:
    print("Usage: %s <cmd>", sys.argv[0])

fifo_client = "/var/run/imsg/rx_pipe"
fifo_server = "/var/run/thermo_lm75/rx_pipe"

if not os.path.exists(fifo_client):
    os.mkfifo(fifo_client)

pipeout = os.open(fifo_server, os.O_WRONLY)
if pipeout == None:
    print("can't open %s" % fifo_server)
    exit(1)

n = os.write(pipeout, sys.argv[1]+"\n")

pipein = open(fifo_client, 'r')
if pipein == None:
    print("can't open %s" % fifo_client);

line = pipein.readline()
print(line),

os.unlink(fifo_client)
