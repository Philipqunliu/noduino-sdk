/*
 *  Copyright (c) 2015 - 2025 MaiKe Labs
 *
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
*/

#include "noduino.h"

#include "airkiss.h"
#include "espconn.h"
#include "smartconfig.h"
#include "mqtt/mqtt.h"

#define	DEBUG	1

// Controll pin is gpio2 for onboard blue led
// gpio12 for itead studio plug box
LOCAL uint8_t ctrl_pin = 12;

MQTT_Client mqttClient;
LOCAL uint8_t check_ip_count = 0;

os_timer_t time_serv;
os_timer_t client_timer;

esp_udp airkiss_udp;
struct espconn ptrairudpconn;

uint8_t lan_buf[200];
uint16_t lan_buf_len;

#define DEVICE_TYPE			"gh_95fae1ba6fa0"
#define DEVICE_ID			"gh_95fae1ba6fa0_8312db1c74a6d97d04063fb88d9a8e47"
#define DEFAULT_LAN_PORT	12476

char mqtt_uname[] = DEVICE_ID;
char mqtt_pass[] = "123456";

const airkiss_config_t akconf = {
	(airkiss_memset_fn) & memset,
	(airkiss_memcpy_fn) & memcpy,
	(airkiss_memcmp_fn) & memcmp,
	0
};

irom void plug_on()
{
#ifdef DEBUG
	serial_printf("Turn on the plug\n");
#endif
	// noduino falcon onboard led
	//digitalWrite(ctrl_pin, LOW);
	// itead studio plug board
	digitalWrite(ctrl_pin, HIGH);
}

irom void plug_off()
{
#ifdef DEBUG
	serial_printf("Turn off the plug\n");
#endif
	// noduino falcon onboard led
	//digitalWrite(ctrl_pin, HIGH);
	// itead studio plug board
	digitalWrite(ctrl_pin, LOW);
}

static irom void time_callback(void)
{
	airkiss_udp.remote_port = DEFAULT_LAN_PORT;
	airkiss_udp.remote_ip[0] = 255;
	airkiss_udp.remote_ip[1] = 255;
	airkiss_udp.remote_ip[2] = 255;
	airkiss_udp.remote_ip[3] = 255;
	lan_buf_len = sizeof(lan_buf);

	int ret = airkiss_lan_pack(AIRKISS_LAN_SSDP_NOTIFY_CMD,
				   DEVICE_TYPE, DEVICE_ID, 0, 0, lan_buf,
				   &lan_buf_len, &akconf);

	if (ret != AIRKISS_LAN_PAKE_READY) {
#ifdef DEBUG
		serial_printf("Pack lan packet error!\r\n");
#endif
		return;
	}
	ret = espconn_sent(&ptrairudpconn, lan_buf, lan_buf_len);
	if (ret != 0) {
		serial_printf("UDP send error!\r\n");
	}
#ifdef DEBUG
	serial_printf("Finish send notify!\r\n");
#endif
}

irom void airkiss_nff_stop(void)
{
	os_timer_disarm(&time_serv);
}

irom void wifilan_recv_callbk(void *arg, char *pdata, unsigned short len)
{
	airkiss_lan_ret_t ret = airkiss_lan_recv(pdata, len, &akconf);
	airkiss_lan_ret_t packret;

	switch (ret) {
	case AIRKISS_LAN_SSDP_REQ:

		airkiss_udp.remote_port = DEFAULT_LAN_PORT;
		lan_buf_len = sizeof(lan_buf);

		packret = airkiss_lan_pack(AIRKISS_LAN_SSDP_RESP_CMD,
					   DEVICE_TYPE, DEVICE_ID, 0, 0,
					   lan_buf, &lan_buf_len, &akconf);

		if (packret != AIRKISS_LAN_PAKE_READY) {
			serial_printf("Pack lan packet error!\r\n");
			return;
		}

		packret = espconn_sent(&ptrairudpconn, lan_buf, lan_buf_len);

		if (packret != 0) {
			serial_printf("LAN UDP Send err!\r\n");
		} else {
			airkiss_nff_stop();
		}
		break;
	default:
		serial_printf("Pack is not ssdq req!\r\n");
		break;
	}
}

irom void airkiss_nff_start(void)
{
	airkiss_udp.local_port = 12476;
	ptrairudpconn.type = ESPCONN_UDP;
	ptrairudpconn.proto.udp = &(airkiss_udp);

	espconn_create(&ptrairudpconn);
	espconn_regist_recvcb(&ptrairudpconn, wifilan_recv_callbk);

	os_timer_setfn(&time_serv, (os_timer_func_t *) time_callback, NULL);
	os_timer_arm(&time_serv, 5000, 1);	//5s定时器
}

irom void smartconfig_done(sc_status status, void *pdata)
{
	switch (status) {
	case SC_STATUS_WAIT:
		serial_printf("SC_STATUS_WAIT\n");
		break;
	case SC_STATUS_FIND_CHANNEL:
		serial_printf("SC_STATUS_FIND_CHANNEL\n");
		break;
	case SC_STATUS_GETTING_SSID_PSWD:
		serial_printf("SC_STATUS_GETTING_SSID_PSWD\n");
		sc_type *type = pdata;
		if (*type == SC_TYPE_ESPTOUCH) {
			serial_printf("SC_TYPE:SC_TYPE_ESPTOUCH\n");
		} else {
			serial_printf("SC_TYPE:SC_TYPE_AIRKISS\n");
		}
		break;
	case SC_STATUS_LINK:
		serial_printf("SC_STATUS_LINK\n");
		struct station_config *sta_conf = pdata;

		serial_printf("Store the ssid and password into flash\n");
		wifi_station_set_config(sta_conf);

		wifi_station_disconnect();
		wifi_station_connect();
		break;
	case SC_STATUS_LINK_OVER:
		serial_printf("SC_STATUS_LINK_OVER\n");
		if (pdata != NULL) {
			uint8 phone_ip[4] = { 0 };

			os_memcpy(phone_ip, (uint8 *) pdata, 4);
			serial_printf("Phone ip: %d.%d.%d.%d\n", phone_ip[0],
				  phone_ip[1], phone_ip[2], phone_ip[3]);
		}
		smartconfig_stop();
		break;
	}

}

irom void mqttConnectedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	serial_printf("MQTT: Connected\r\n");
	MQTT_Subscribe(client, "/app2dev/gh_95fae1ba6fa0_8312db1c74a6d97d04063fb88d9a8e47", 0);
	MQTT_Publish(client, "/dev2app/gh_95fae1ba6fa0_8312db1c74a6d97d04063fb88d9a8e47", "online", 6, 0, 0);
}

irom void mqttDisconnectedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	serial_printf("MQTT: Disconnected\r\n");
}

irom void mqttPublishedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	serial_printf("MQTT: Published\r\n");
}

irom void mqttDataCb (uint32_t *args, const char* topic,
	   	uint32_t topic_len, const char *data, uint32_t data_len)
{
	char *topicBuf = (char*)os_zalloc(topic_len+1),
			*dataBuf = (char*)os_zalloc(data_len+1);

	MQTT_Client* client = (MQTT_Client*)args;

	os_memcpy(topicBuf, topic, topic_len);
	topicBuf[topic_len] = 0;

	os_memcpy(dataBuf, data, data_len);
	dataBuf[data_len] = 0;

	if(os_strncmp(dataBuf, "on", 2) == 0)
	{
		plug_on();
	}
	if(os_strncmp(dataBuf, "off", 3) == 0)
	{
		plug_off();
	}

	serial_printf("Receive topic: %s, data: %s \r\n", topicBuf, dataBuf);
	os_free(topicBuf);
	os_free(dataBuf);
}

irom void cos_check_ip()
{
	struct ip_info ipconfig;

	os_timer_disarm(&client_timer);

	wifi_get_ip_info(STATION_IF, &ipconfig);

	if (wifi_station_get_connect_status() == STATION_GOT_IP
		&& ipconfig.ip.addr != 0) {
		// connect to router success
		// TODO: notify the cloud I'm online
		// cloud return the bind state

		// start broadcast airkiss-nff udp pkg
		// TODO: need to check the binding state
		airkiss_nff_start();
		MQTT_Connect(&mqttClient);
	} else {
		// idle or connecting
		os_timer_setfn(&client_timer, (os_timer_func_t *)cos_check_ip, NULL);
		os_timer_arm(&client_timer, 150, 0);

		if (check_ip_count++ > 50) {
			// delay 10s, need to start airkiss to reconfig the network
			// TODO: flash led to show wifi disconnect
			smartconfig_set_type(SC_TYPE_AIRKISS);
			wifi_set_opmode(STATION_MODE);
			smartconfig_start(smartconfig_done);

			// reset the count
			check_ip_count = 0;
		}

		MQTT_Disconnect(&mqttClient);
	}
}

irom void setup(void)
{
	char client_name[32] = "\0";
#ifdef DEBUG
	serial_begin(115200);
#endif

	//Set ctrl_pin to output mode
	pinMode(ctrl_pin, OUTPUT);

	sprintf(client_name, "noduino_falcon_%d", os_random()%10000);

	MQTT_InitConnection(&mqttClient, "101.200.202.247", 1883, 0);
	MQTT_InitClient(&mqttClient, client_name, mqtt_uname, mqtt_pass, 120, 1);
	MQTT_OnConnected(&mqttClient, mqttConnectedCb);
	MQTT_OnDisconnected(&mqttClient, mqttDisconnectedCb);
	MQTT_OnPublished(&mqttClient, mqttPublishedCb);
	MQTT_OnData(&mqttClient, mqttDataCb);

	serial_printf("\r\nSystem started ...\r\n");

	cos_check_ip();
}

irom void loop(void)
{

}
