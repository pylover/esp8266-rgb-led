
// Internal 
#include "user_config.h"
#include "partition.h"
#include "wifi.h"
#include "params.h" 
#include "debug.h"
#include "fotabtn.h"
#include "httpserver.h"
#include "querystring.h"

// SDK
#include <ets_sys.h>
#include <pwm.h>
#include <osapi.h>
#include <mem.h>
#include <user_interface.h>
#include <driver/uart.h>
#include <upgrade.h>
#include <c_types.h>
#include <ip_addr.h> 
#include <espconn.h>
#include <html.c>

#define MAXDUTY     (1000*1000/44)
static char currentcolor[8] = {0};

static Params params;
static struct mdns_info mdns;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} color_t;

static ICACHE_FLASH_ATTR
void set_duty(char *c, uint16_t v) {
    uint32_t val = (v) * MAXDUTY;
    val = val / 255;
    pwm_set_duty(val , c);
    pwm_start();
}

void _mdns_init() {
	struct ip_info ipconfig;
	wifi_set_broadcast_if(STATIONAP_MODE);
	wifi_get_ip_info(STATION_IF, &ipconfig);
	mdns.ipAddr = ipconfig.ip.addr; //ESP8266 Station IP
	mdns.host_name = params.device_name;
	mdns.server_name = "ESPWebAdmin";
	mdns.server_port = 80;
	mdns.txt_data[0] = "version = 0.1.0";
	espconn_mdns_init(&mdns);
}

void wifi_connect_cb(uint8_t status) {
    if(status == STATION_GOT_IP) {
		_mdns_init();
        INFO("Wifi connected.\r\n");
    } else {
		espconn_mdns_close();
        INFO("Wifi disconnected.\r\n");
    }
}

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} color_t;

static ICACHE_FLASH_ATTR
uint16_t htoi16(const char *h) {
    uint16_t value;
    char tmp[3];
    os_strncpy(tmp, h, 2);
    tmp[2] = 0;
    value = (uint16_t)strtol(tmp, NULL, 16); 
    return value;
}

static ICACHE_FLASH_ATTR
void updatecolor(const char *color) {
    color_t c;
    c.r = htoi16(color + 1);
    c.g = htoi16(color + 3);
    c.b = htoi16(color + 5);
    os_printf("%s: %d, %d, %d\n", color, c.r, c.g, c.b);
    
    set_duty(R, c.r);
    set_duty(G, c.g);
    set_duty(B, c.b);
    os_strncpy(currentcolor, color, 7);
}

static ICACHE_FLASH_ATTR
void _querystring_cb(const char *field, const char *value) {
	if (os_strcmp(field, "c") == 0) {
        updatecolor(value);
	}
}

static ICACHE_FLASH_ATTR
void webadj(Request *req, char *body, uint32_t body_length, 
		uint32_t more) {
    body[body_length] = 0; 
    querystring_parse(body, _querystring_cb);
	char buffer[1024];
	int len = os_sprintf(buffer, HTML_FORM, currentcolor);
	httpserver_response_html(req, HTTPSTATUS_OK, buffer, len);
}


static ICACHE_FLASH_ATTR
void webindex(Request *req, char *body, uint32_t body_length, 
		uint32_t more) {
	char buffer[1024];
	int len = os_sprintf(buffer, HTML_FORM, currentcolor);
	httpserver_response_html(req, HTTPSTATUS_OK, buffer, len);
}



static HttpRoute routes[] = {
	{"GET", "/", webindex},
	{"POST", "/", webadj},
	{ NULL }
};



void user_init(void) {
    uart_init(BIT_RATE_115200, BIT_RATE_115200);
    os_delay_us(60000);
	bool ok = params_load(&params);
	if (!ok) {
		ERROR("Cannot load Params\r\n");
#if !WIFI_ENABLE_SOFTAP
		return;
#endif
		if(!params_defaults(&params)) {
			ERROR("Cannot save params\r\n");
			return;
		}
	}

	INFO("\r\nParams: name: %s, ssid: %s psk: %s ap-psk: %s\r\n",
			params.device_name,
			params.station_ssid, 
			params.station_psk,
			params.ap_psk
		);
    
    fotabtn_init();

#if WIFI_ENABLE_SOFTAP
    wifi_start(STATIONAP_MODE, &params, wifi_connect_cb);
#else
    wifi_start(STATION_MODE, &params, wifi_connect_cb);
#endif
    INFO("System started ...\r\n");

    /* ATI */
    // PWM Init 
   	uint32 pwmioinfo[][3] = {
        {R_MUX, R_FUNC, R_NUM},
        {G_MUX, G_FUNC, G_NUM},
        {B_MUX, B_FUNC, B_NUM},
    };
 
    uint32 pwmdutyinit[3] = {0};
    pwm_init(1000, pwmdutyinit, 3, pwmioinfo);
    os_delay_us(1000);
    set_duty(R, 0);
    set_duty(G, 0);
    set_duty(B, 0);
    pwm_set_period(1000);
    pwm_start();

    // HTTP Server
    httpserver_init(80, routes);
}


void ICACHE_FLASH_ATTR user_pre_init(void)
{
    if(!system_partition_table_regist(at_partition_table, 
				sizeof(at_partition_table)/sizeof(at_partition_table[0]),
				SPI_FLASH_SIZE_MAP)) {
		FATAL("system_partition_table_regist fail\r\n");
		while(1);
	}
}

