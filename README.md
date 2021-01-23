
JBD BMS MQTT Publisher

requires mybmm https://github.com/sshoecraft/mybmm.git for the modules; download that project then set MYBMM_SRC in the makefile

Bluetooth support requires gattlib https://github.com/labapart/gattlib
Then edit the Makefile and set BLUETOOTH=yes

Requires PAHO https://github.com/eclipse/paho.mqtt.c


Transports specified exactly as in mybmm.conf

jbdtool -t <transport:target,opt1[,optN]>


For CAN:

jbdtool -t can:<device>[,speed]

example:

	jbdtool -t can:can0,500000

For Serial:

jbdtool -t serial:<device>[,speed]

example:

	jbdtool -t serial:/dev/ttyS0,9600

For Bluetooth:

jbdtool -t bt:[mac addr][,desc]

exmples:

	jbdtool -t bt:01:02:03:04:05,06

	jbdtool -t bt:01:02:03:04:05:06,ff01

For IP/esplink:

jbdtool -t ip:<ip addr>[,port]

example:

	jbdtool -t ip:10.0.0.1,23

for CANServer/Can-over-ip

jbdtool -t can_ip:<ip addr>,[port],<interface>,[speed]

example:

	jbdtool -t can_ip:10.0.0.1,3930,can0,500000


>>> CAN bus cannot read/write parameters


to read all parameters using bluetooth:

jbdtool -t bt:01:02:03:04:06 -r -a

to list the params the program supports, use -l

to specify single params, specify them after -r

jbdtool -t bt:01:02:03:04:06 -r BalanceStartVoltage BalanceWindow

to read a list of parameters using a file use -f:

jbdtool -t serial:/dev/ttyS0,9600 -r -f jbd_settings.fig

use -j and -J (pretty) to specify filename is a json file


to write parameters, specify a key value pair after -w

jbdtool -t ip:10.0.0.1 -w BalanceStartVoltage 4050 BalanceWindow 20


to send all output to a file, use -o.   If the filename ends with .json, file will be written in JSON format:

jbdtool -t can:can0,500000 -j -o pack_1.json
