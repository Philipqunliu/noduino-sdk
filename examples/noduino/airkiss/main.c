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
#include "smartconfig.h"

void smartconfig_done(sc_status status, void *pdata)
{
	switch (status) {
	case SC_STATUS_WAIT:
		os_printf("SC_STATUS_WAIT\n");
		break;
	case SC_STATUS_FIND_CHANNEL:
		os_printf("SC_STATUS_FIND_CHANNEL\n");
		break;
	case SC_STATUS_GETTING_SSID_PSWD:
		os_printf("SC_STATUS_GETTING_SSID_PSWD\n");
		sc_type *type = pdata;
		if (*type == SC_TYPE_ESPTOUCH) {
			os_printf("SC_TYPE:SC_TYPE_ESPTOUCH\n");
		} else {
			os_printf("SC_TYPE:SC_TYPE_AIRKISS\n");
		}
		break;
	case SC_STATUS_LINK:
		os_printf("SC_STATUS_LINK\n");
		struct station_config *sta_conf = pdata;

		os_printf("Store the ssid and password into flash\n");
		wifi_station_set_config(sta_conf);

		wifi_station_disconnect();
		wifi_station_connect();
		break;
	case SC_STATUS_LINK_OVER:
		os_printf("SC_STATUS_LINK_OVER\n");
		if (pdata != NULL) {
			uint8 phone_ip[4] = { 0 };

			os_memcpy(phone_ip, (uint8 *) pdata, 4);
			os_printf("Phone ip: %d.%d.%d.%d\n", phone_ip[0],
				  phone_ip[1], phone_ip[2], phone_ip[3]);
		}
		smartconfig_stop();
		break;
	}

}

void setup()
{
	//serial_begin(115200);
	uart_init(115200, 115200);

	os_printf("Noduino Airkiss Example!\r\n");

	//SC_TYPE_ESPTOUCH,SC_TYPE_AIRKISS,SC_TYPE_ESPTOUCH_AIRKISS
	smartconfig_set_type(SC_TYPE_AIRKISS);

	wifi_set_opmode(STATION_MODE);

	smartconfig_start(smartconfig_done);
}

void loop()
{

}
