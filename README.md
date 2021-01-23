
JBD BMS MQTT Publisher

requires mybmm https://github.com/sshoecraft/mybmm.git for the modules; download that project then set MYBMM_SRC in the makefile

Bluetooth support requires gattlib https://github.com/labapart/gattlib
Then edit the Makefile and set BLUETOOTH=yes

Requires PAHO https://github.com/eclipse/paho.mqtt.c


MQTT specified as

	jbdmqtt -m brokerhost:port,ClientID,topic


MQTT example for IP(monitor pack with dns name of "pack_01", interval every 30s, run in the background, output to log):

	jbdmqtt -m localhost,pack_01,Powerwall/pack_01 -t ip:pack_01 -i 30 -b -l /var/log/jbdmqtt/pack_01.log


TRANSPORT EXAMPLES:

Transports specified exactly as in mybmm.conf

	jbdmqtt -t <transport:target,opt1[,optN]>


For CAN:

jbdmqtt -t can:<device>[,speed]

example:

	jbdmqtt -t can:can0,500000

For Serial:

jbdmqtt -t serial:<device>[,speed]

example:

	jbdmqtt -t serial:/dev/ttyS0,9600

For Bluetooth:

jbdmqtt -t bt:[mac addr][,desc]

exmples:

	jbdmqtt -t bt:01:02:03:04:05,06

	jbdmqtt -t bt:01:02:03:04:05:06,ff01

For IP/esplink:

jbdmqtt -t ip:<ip addr>[,port]

example:

	jbdmqtt -t ip:10.0.0.1,23

for CANServer/Can-over-ip

jbdmqtt -t can_ip:<ip addr>,[port],<interface>,[speed]

example:

	jbdmqtt -t can_ip:10.0.0.1,3930,can0,500000
