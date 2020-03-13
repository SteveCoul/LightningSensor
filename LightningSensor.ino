
#include <ESP8266WiFi.h>
#include <coredecls.h>                  // settimeofday_cb()
#include <Wire.h>
#include <ESP8266WebServer.h>
#include <time.h>                       // time() ctime()
#include <sys/time.h>                   // struct timeval
#include <sntp.h>
#include <TZ.h>

#include "LightningSensor.h"

#define TIME_ZONE   TZ_America_Chicago

#include "SECRETS_DO_NOT_CHECKIN.h"
#ifndef WIFI_SSID
#error you need to define WIFI_SSID and WIFI_PASS in the SECRETS_DO_NOT_CHECKIN.h header
#endif

#define SDA_PIN 4
#define SCL_PIN 5
#define IRQ_PIN 13

const int16_t I2C_MASTER = 0x42;
const int16_t I2C_LIGHTNING = 0x3;

static LightningSensor ls( I2C_LIGHTNING, IRQ_PIN );
static ESP8266WebServer server(80);

static IPAddress    server_ip(192,168,0,163);
static IPAddress    gateway(192,168,0,1);
static IPAddress    subnet(255,255,255,0);
static IPAddress    dns(8,8,8,8);

/* ******************************************************************************************************** *
 *  
 * ******************************************************************************************************** */

#define   STORE_MESSAGES      32

class Message {
public:

	enum {
		NOISE,
		DISTURBANCE,
		STRIKE,
		DISTANCE_UPDATE
	};

	Message( int type, int distance=-1, int power=-1 ) : m_type( type ), m_distance(distance), m_power( power )  {
		if ( m_head == NULL ) {
			m_head = m_tail = this;
			m_prev = NULL;
			m_next = NULL;
			m_count = 1;
		} else {
			if ( m_count == STORE_MESSAGES ) {
				class Message* d = m_head;
				m_head = m_head->m_next;
				m_head->m_prev = NULL;
				m_count--;
				delete d;
			}

			m_prev = m_tail;
			m_next = NULL;
			m_prev->m_next = this;
			m_tail = this;
			m_count++;
		}

		m_when = time(NULL);
	}

	String toString() {
      	String t = String( ctime(&m_when) );
		t.replace("\n", "\0" );
		switch( m_type ) {
		case NOISE:
			t = t + " Noise";
			break;
		case DISTURBANCE:
			t = t + " Disturbance";
			break;
		case STRIKE:
			t = t + " Strike, power 0x" + String( m_power, HEX ) + ", distance " + String( m_distance ) + "km";
			break;
		case DISTANCE_UPDATE:
			t = t + " Distance update " + String( m_distance ) + "km";
			break;
		default:
			t = t + " Unknown event" ;
			break;
		}
		return t;
	}
      
	int						m_distance;
	int						m_power;
	time_t					m_when;
	class Message*			m_prev;
	class Message*			m_next;
	int						m_type;
	static class Message* 	m_head;
	static class Message* 	m_tail;
	static int				m_count;
};

class Message* Message::m_head = 0;
class Message* Message::m_tail = 0;
int            Message::m_count = 0;

/* ******************************************************************************************************** *
 *  
 * ******************************************************************************************************** */

static void _httpRoot() {
  String body = String("<HTML><HEAD><meta http-equiv=\"refresh\" content=\"10\"> </HEAD><BODY><H1>Howdly Doooodly Dooo - I'm a lightning detector apparently\n\n</H1>");
  class Message* m = Message::m_head;
  while ( m ) {
    body = body + "<P>";
    body = body + m->toString();
    body = body + "</P>";
	m = m->m_next;
  }

  body += "</BODY></HTML>";
  server.send(200, "text/html", body);
}
  
static void _httpNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void setup() {
  
  Serial.begin(115200);  // start serial for output
  
  Serial.println("Howdy Dooodly Dooo");
   Wire.begin(SDA_PIN, SCL_PIN, I2C_MASTER); // join i2c bus (address optional for master)
  pinMode( IRQ_PIN, INPUT );

  configTime( TIME_ZONE, "pool.ntp.org" );

  Serial.println("Configure WiFi");

  WiFi.mode( WIFI_STA );
  WiFi.config( server_ip, gateway, subnet, dns );
  WiFi.begin( WIFI_SSID, WIFI_PASS );

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(WIFI_SSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  

  ls.presetDefault();
  ls.powerUp();
  ls.clearStatistics();
  ls.setRequiredNumStrikes( 1 );
  ls.setIndoors();
  ls.setNoiseFloor( 7 );
  ls.setWatchdogThreshold( 2 );   
  ls.setSpikeRejection( 2 );
  ls.disableDisturberDetection();
  (void)ls.calibrate();

  ls.dump();

  server.on("/", _httpRoot);
  server.onNotFound(_httpNotFound);
  server.begin();

  Serial.println("And off we go!");
  ls.listen();
}

void loop() {
 
  server.handleClient();
    if ( ls.poll() ) {
      uint8_t i;
      delay(2);

      i = ls.getInterrupt();
      if ( ls.wasInterruptNoise( i ) ) {
		Serial.println( (new Message( Message::NOISE ))->toString() );
      } else if ( ls.wasInterruptDisturbance( i ) ) {
		Serial.println( (new Message( Message::DISTURBANCE ))->toString() );
      } else if ( ls.wasInterruptStrike( i ) ) {
        int raw = ls.strikeEnergyRAW();
        int km = ls.lightningDistanceInKm();
		Serial.println( (new Message( Message::STRIKE, km, raw ))->toString() );
      } else if ( ls.wasInterruptDistanceUpdate( i ) ) {
        int km = ls.lightningDistanceInKm();
		Serial.println( (new Message( Message::DISTANCE_UPDATE, km ))->toString() );
      }
    }
  
}
