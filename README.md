
JBD BMS MQTT Publisher

requires mybmm https://github.com/sshoecraft/mybmm.git for the modules; download that project then set MYBMM_SRC in the makefile

Bluetooth support requires gattlib https://github.com/labapart/gattlib
Then edit the Makefile and set BLUETOOTH=yes

Requires PAHO https://github.com/eclipse/paho.mqtt.c


Transports specified exactly as in mybmm.conf

jbdtool -t <transport:target,opt1[,optN]>


MQTT example for IP(monitor pack with dns name of "pack_01", interval every 30s, run in the background, output to log):

	jbdmqtt -m localhost,pack_01,Powerwall/pack_01 -t ip:pack_01 -i 30 -b -l /var/log/jbdmqtt/pack_01.log


TRANSPORT EXAMPLES:

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
