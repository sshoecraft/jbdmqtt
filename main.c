
/*

An MQTT publisher for the JBD BMS

Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "parson.h"
#include "mybmm.h"
#include "jbd.h"		/* For session info */
#include "jbd_info.h"		/* For info struct */
#include "MQTTClient.h"

int debug = 9;

FILE *outfp;
extern FILE *logfp;

JSON_Value *root_value;
JSON_Object *root_object;
char *serialized_string = NULL;

char *trim(char *);

extern mybmm_module_t ip_module;

#define dint(l,f,v) json_object_set_number(root_object, l, v)

#define dblool(l,f,v) json_object_set_boolean(root_object, l, v)

#define dfloat(l,f,v) json_object_set_number(root_object, l, v)

#define dstr(l,f,v) json_object_set_string(root_object, l, v)

void display_info(jbd_info_t *info) {
	char temp[256],*p;
	int i;

	dfloat("Voltage","%.3f",info->voltage);
	dfloat("Current","%.3f",info->current);
	dfloat("DesignCapacity","%.3f",info->fullcap);
	dfloat("RemainingCapacity","%.3f",info->capacity);
	dint("PercentCapacity","%d",info->pctcap);
	dint("CycleCount","%d",info->cycles);
	dint("Probes","%d",info->probes);
	p = temp;
	p += sprintf(p,"[ ");
	for(i=0; i < info->probes; i++) {
		if (i) p += sprintf(p,",");
		p += sprintf(p, "%.1f",info->temps[i]);
	}
	strcat(temp," ]");
	dprintf(1,"temp: %s\n", temp);
	json_object_dotset_value(root_object, "Temps", json_parse_string(temp));
	dint("Strings","%d",info->strings);
	p = temp;
	p += sprintf(p,"[ ");
	for(i=0; i < info->strings; i++) {
		if (i) p += sprintf(p,",");
		p += sprintf(p, "%.3f",info->cellvolt[i]);
	}
	strcat(temp," ]");
	dprintf(1,"temp: %s\n", temp);
	json_object_dotset_value(root_object, "Cells", json_parse_string(temp));
	dfloat("CellTotal","%.3f",info->cell_total);
	dfloat("CellMin","%.3f",info->cell_min);
	dfloat("CellMax","%.3f",info->cell_max);
	dfloat("CellDiff","%.3f",info->cell_diff);
	dfloat("CellAvg","%.3f",info->cell_avg);
	temp[0] = 0;
	p = temp;
	if (info->fetstate & JBD_MOS_CHARGE) p += sprintf(p,"Charge");
	if (info->fetstate & JBD_MOS_DISCHARGE) {
		if (info->fetstate & JBD_MOS_CHARGE) p += sprintf(p,",");
		p += sprintf(p,"Discharge");
	}
	dstr("FET","%s",temp);
#if 0
        unsigned long balancebits;
        /* the protection sign */
        unsigned short protectbits;
        struct {
                unsigned sover: 1;              /* Single overvoltage protection */
                unsigned sunder: 1;             /* Single undervoltage protection */
                unsigned gover: 1;              /* Whole group overvoltage protection */
                unsigned gunder: 1;             /* Whole group undervoltage protection */
                unsigned chitemp: 1;            /* Charge over temperature protection */
                unsigned clowtemp: 1;           /* Charge low temperature protection */
                unsigned dhitemp: 1;            /* Discharge over temperature protection */
                unsigned dlowtemp: 1;           /* Discharge low temperature protection */
                unsigned cover: 1;              /* Charge overcurrent protection */
                unsigned cunder: 1;             /* Discharge overcurrent protection */
                unsigned shorted: 1;            /* Short circuit protection */
                unsigned ic: 1;                 /* Front detection IC error */
                unsigned mos: 1;                /* Software lock MOS */
        } protect;
        unsigned short fetstat;                 /* for the MOS tube status */
        struct {
                unsigned charging: 1;
                unsigned discharging: 1;
        } fet;
#endif
}

int init_pack(mybmm_pack_t *pp, mybmm_config_t *c, char *type, char *transport, char *target, char *opts, mybmm_module_t *cp, mybmm_module_t *tp) {
	memset(pp,0,sizeof(*pp));
	strcpy(pp->type,type);
	if (transport) strcpy(pp->transport,transport);
	if (target) strcpy(pp->target,target);
	if (opts) strcpy(pp->opts,opts);
        pp->open = cp->open;
        pp->read = cp->read;
        pp->close = cp->close;
        pp->handle = cp->new(c,pp,tp);
        return 0;
}

enum JBDTOOL_ACTION {
	JBDTOOL_ACTION_INFO=0,
	JBDTOOL_ACTION_READ,
	JBDTOOL_ACTION_WRITE,
	JBDTOOL_ACTION_LIST
};

static char address[64];
static char clientid[64];
static char topic[64];
#define QOS 1
#define TIMEOUT     10000L

int mqtt_send(char *message) {
	MQTTClient client;
	MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
	MQTTClient_message pubmsg = MQTTClient_message_initializer;
	MQTTClient_deliveryToken token;
	int rc;

	MQTTClient_create(&client, address, clientid, MQTTCLIENT_PERSISTENCE_NONE, NULL);
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
	if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
		printf("Failed to connect, return code %d\n", rc);
		exit(-1);
	}
	dprintf(1,"message: %s\n", message);
	pubmsg.payload = message;
	pubmsg.payloadlen = strlen(message);
	pubmsg.qos = QOS;
	pubmsg.retained = 0;
	MQTTClient_publishMessage(client, topic, &pubmsg, &token);
//	printf("Waiting for up to %d seconds for publication of %s\n" "on topic %s for client with ClientID: %s\n", (int)(TIMEOUT/1000), PAYLOAD, TOPIC, CLIENTID);
	rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
//	printf("Message with delivery token %d delivered\n", token);
	dprintf(1,"delivered message.\n");
	MQTTClient_disconnect(client, 10000);
	MQTTClient_destroy(&client);
	return rc;
}

static pid_t pid;
static int term = 0;
void catch_alarm(int sig) {
	printf("timeout expired, killing pid %d\n", (int)pid);
        if (term)
                kill(pid,SIGKILL);
        else {
                kill(pid,SIGTERM);
                term++;
                alarm(3);
        }
}

int pretty;
int get_info(jbd_session_t *s) {
	jbd_info_t info;
	int r;

	dprintf(1,"s: %p\n",s);
	if (s->pp->open(s)) return 1;
	r = jbd_get_info(s,&info);
	dprintf(1,"r: %d\n", r);
	if (!r) {
		display_info(&info);
		if (pretty)
    			serialized_string = json_serialize_to_string_pretty(root_value);
		else
    			serialized_string = json_serialize_to_string(root_value);
		mqtt_send(serialized_string);
		json_free_serialized_string(serialized_string);
	}
	s->pp->close(s);
	return r;
}

int bgrun(jbd_session_t *s, int timeout) {
//	int pty;

	dprintf(1,"s: %p, timeout: %d\n", s, timeout);
#if 0
	if ((pty = getpt()) < 0) {
		perror("openpt");
		return -1;
	}
	if (grantpt(pty)) {
		perror("granpt");
		return -1;
	}
	if (unlockpt(pty)) {
		perror("unlockpt");
		return -1;
	}
#endif

	dprintf(1,"forking...\n");
	pid = fork();
	if (pid < 0) {
		/* Error */
		perror("fork");
		return -1;
	} else if (pid == 0) {
#if 0
		/* Child */
		int pts;
		struct termios old_tcattr,tcattr;

		/* Detach from current tty */
		setsid();

		/* Open pty slave */
		pts = open(ptsname(pty), O_RDWR );
		if (pts < 0) perror("open pts");

		/* Close master */
		close(pty);

		/* Set raw mode term attribs */
		tcgetattr(pts, &old_tcattr); 
		tcattr = old_tcattr;
		cfmakeraw (&tcattr);
		tcsetattr (pts, TCSANOW, &tcattr); 

		/* Set stdin/stdout/stderr with pts */
		dup2(pts, STDIN_FILENO);
		dup2(pts, STDOUT_FILENO);
		dup2(pts, STDERR_FILENO);
#endif

		/* Call func */
		dprintf(1,"updating...\n");
		get_info(s);
		_exit(1);
	} else {
		/* Parent */
		struct sigaction act1, oact1;
		int status;

		/* Set up timeout alarm */
		if (timeout) {
			act1.sa_handler = catch_alarm;
			sigemptyset(&act1.sa_mask);
			act1.sa_flags = 0;
#ifdef SA_INTERRUPT
			act1.sa_flags |= SA_INTERRUPT;
#endif
			if( sigaction(SIGALRM, &act1, &oact1) < 0 ){
				perror("sigaction");
				exit(1);
			}
			alarm(timeout);
		}

		dprintf(1,"waiting on pid...\n");
		waitpid(pid,&status,0);
		dprintf(1,"WIFEXITED: %d\n", WIFEXITED(status));
		if (WIFEXITED(status)) dprintf(1,"WEXITSTATUS: %d\n", WEXITSTATUS(status));
		status = (WIFEXITED(status) ? WEXITSTATUS(status) : 1);
		dprintf(1,"status: %d\n", status);
		return status;
	}
	return 0;
}

void usage() {
	printf("usage: jbdtool [-abcjJrwlh] [-f filename] [-t <module:target> [-o output file]\n");
	printf("arguments:\n");
#ifdef DEBUG
	printf("  -d <#>		debug output\n");
#endif
	printf("  -c		comma-delimited output\n");
	printf("  -J		preety-print JSON output\n");
	printf("  -l <logfile>	write output to logfile\n");
	printf("  -h		this output\n");
	printf("  -t <transport:target> transport & target\n");
	printf("  -m 		Send results to MQTT broker\n");
	printf("  -i 		Update interval\n");
	printf("  -b 		Run in background\n");
}

int become_daemon(void);

int main(int argc, char **argv) {
	int opt,action;
	char *transport,*target,*logfile;
	mybmm_config_t *conf;
	mybmm_module_t *cp,*tp;
	mybmm_pack_t pack;
	int back,interval;
	char *mqtt;
	time_t start,end,diff;

	log_open("mybmm",0,LOG_INFO|LOG_WARNING|LOG_ERROR|LOG_SYSERR|LOG_DEBUG);

	action = pretty = 0;
	transport = target = logfile = 0;
	interval = back = 0;
	mqtt = 0;
	while ((opt=getopt(argc, argv, "+d:bm:i:t:l:hJ")) != -1) {
		switch (opt) {
		case 'd':
			debug=atoi(optarg);
			break;
		case 'b':
			back = 1;
			break;
		case 'm':
			mqtt = optarg;
			break;
		case 'J':
			pretty = 1;
			break;
                case 'i':
			interval=atoi(optarg);
			break;
                case 't':
			transport = optarg;
			target = strchr(transport,':');
			if (!target) {
				printf("error: format is transport:target\n");
				usage();
				return 1;
			}
			*target = 0;
			target++;
			break;
                case 'l':
			logfile = optarg;
			break;
		case 'h':
		default:
			usage();
			exit(0);
                }
        }
	dprintf(2,"transport: %p, target: %p\n", transport, target);
	if (!transport && action != JBDTOOL_ACTION_LIST) {
		usage();
		return 1;
	}

	
	if (logfile) log_open("mybmm",logfile,LOG_TIME|LOG_INFO|LOG_WARNING|LOG_ERROR|LOG_SYSERR|LOG_DEBUG);

	/* If MQTT, output is compact JSON */
	if (mqtt) {
		strcpy(address,strele(0,",",mqtt));
		strcpy(clientid,strele(1,",",mqtt));
		strcpy(topic,strele(2,",",mqtt));
		dprintf(1,"address: %s, clientid: %s, topic: %s\n", address, clientid, topic);
		action = JBDTOOL_ACTION_INFO;
	} else {
		printf("error: MUST provide MQTT info!\n");
		return 1;
	}

	conf = calloc(sizeof(*conf),1);
	if (!conf) {
		perror("calloc conf");
		return 1;
	}
	conf->modules = list_create();

	dprintf(2,"transport: %s\n", transport);

	tp = mybmm_load_module(conf,transport,MYBMM_MODTYPE_TRANSPORT);
	if (!tp) return 1;
	cp = mybmm_load_module(conf,"jbd",MYBMM_MODTYPE_CELLMON);
	if (!cp) return 1;

	/* Init the pack */
	if (init_pack(&pack,conf,"jbd",transport,target,0,cp,tp)) return 1;

	if (back) {
		become_daemon();
		if (logfile) log_open("mybmm",logfile,LOG_TIME|LOG_INFO|LOG_WARNING|LOG_ERROR|LOG_SYSERR|LOG_DEBUG);
	}

// 	outfp = fdopen(1,"w");
	if (logfp) outfp = logfp;

	root_value = json_value_init_object();
	root_object = json_value_get_object(root_value);
	while(interval) {
		time(&start);
		bgrun(pack.handle,interval);
		time(&end);
		diff = end - start;
		dprintf(1,"start: %d, end: %d, diff: %d\n", (int)start, (int)end, (int)diff);
		if (diff < interval) sleep(interval-(int)diff);
	}
	json_value_free(root_value);
	fclose(outfp);
	return 0;
}
