#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <EEPROM.h>
#include <Creds.h>
#include <Protocol.h>
#include <ArduinoOTA.h>
#include <TimerAction.h>

// #define DEBUG

#ifdef DIGITAL_CONTROLLER
#include <DigitalLedController.h>
#else
#include <AnalogLedController.h>
#endif

// #define WRITE_PWD

#define MAX_CLIENTS 4
#define TIMEOUT 10000
#define MAX_ACTIONS 8

#define DAY_IN_MS 86400000

WiFiServer server(8080);
WiFiClient clients[MAX_CLIENTS];
int16_t timeouts[MAX_CLIENTS];
TimerAction actions[MAX_ACTIONS];
uint8_t packetType;

int64_t unixTimestampOffset = 0;

WiFiUDP udp;

#ifdef DIGITAL_CONTROLLER
DigitalLedController controller(120);
float correction[4] = {1, .7, .5, 1};
#else
AnalogLedController controller;
float correction[4] = {1, .6, .3, 1};
#endif

void setup()
{
#ifdef DEBUG
	Serial.begin(9600);
#endif

	controller.Setup();

#ifdef WRITE_PWD
	EEPROM.begin(64);
	const char *pass = _WIFI_PASSWORD;
	EEPROM.put(0, pass);
#else
	const char *pass = _WIFI_PASSWORD;

	EEPROM.begin(MAX_ACTIONS * sizeof(TimerAction));
	for (size_t i = 0; i < MAX_ACTIONS; i++)
	{
		EEPROM.get(i * sizeof(TimerAction), actions[i]);
	}

	// char pass[64];
	// EEPROM.get(0, pass);
#endif
	controller.ShowColor(.1, .1, .1, .1);

	WiFi.mode(WIFI_STA); // set mode to wifi station
	WiFi.setSleepMode(WIFI_NONE_SLEEP);
	WiFi.begin(_WIFI_SSID, pass); // connect to wifi router

	controller.ShowColor(.1, 0, 0, 0);

	while (WiFi.status() != WL_CONNECTED)
	{
		delay(100);
	}

	server.begin(8080);
	udp.begin(8081);
	ArduinoOTA.begin();

	controller.ShowColor(0, .1, 0, 0);
	delay(1000);

	controller.ShowColor(0, 0, 0, 0);
}

void respondToBroadcast()
{
	int available = udp.available();
	if (available)
	{
		if (udp.find("RGB STRIP BROADCAST"))
		{
			udp.beginPacketMulticast(IPAddress(255, 255, 255, 255), 8082, WiFi.localIP());
			udp.print("RGB STRIP RESPONCE");
			udp.endPacket();
		}
		for (int i = 0; i < available; i++)
		{
			udp.read();
		}
		udp.begin(8081);
	}
}

uint64_t timestamp()
{
	return millis() + unixTimestampOffset;
}

void handleActions()
{
	for (size_t i = 0; i < MAX_ACTIONS; i++)
	{
		if (actions[i].activated && actions[i].nextExecution < timestamp())
		{
			actions[i].nextExecution += DAY_IN_MS;
			controller.ShowColor(actions[i].r, actions[i].g, actions[i].b, actions[i].w);
		}
	}
}

void loop()
{
	uint64_t time_now = micros64();

	ArduinoOTA.handle();

	respondToBroadcast();

	handleActions();

	WiFiClient client = server.available();
	if (client)
	{
		for (size_t i = 0; i < MAX_CLIENTS; i++)
		{
			if (!clients[i] || !clients[i].connected())
			{
				client.setNoDelay(true);
				clients[i] = client;
				timeouts[i] = TIMEOUT;
				break;
			}
		}
	}

	for (size_t i = 0; i < MAX_CLIENTS; i++)
	{
		client = clients[i];
		if (!client || !client.connected())
		{
			continue;
		}
		if (timeouts[i] < 0)
		{
			client.stop();
			continue;
		}
		timeouts[i]--;
		if (client.available())
		{
			timeouts[i] = TIMEOUT;
			client.readBytes(&packetType, 1);

			switch (packetType)
			{
			case INFO_PACKET:
			{
				char data[1 + 1 + 1 + 4 * 4];
				data[0] = INFO_PACKET;
				data[1] = controller.GetLedCount();
				data[2] = controller.GetFlags();

				memcpy(&data[3], &correction, 16);

				client.write(data, sizeof(data));
				break;
			}
			case DATA_PACKET:
			{
				controller.ReadColors(client);
				break;
			}
			case TIMESTAMP_PACKET:
			{
				int64_t timestamp = 0;
				client.readBytes((char *)&timestamp, sizeof(int64_t));

				unixTimestampOffset = timestamp - millis();

				break;
			}
			case ADD_ACTION_PACKET:
			{
				TimerAction action;
				client.readBytes((char *)&action, sizeof(TimerAction));

				// yolo
				actions[action.id] = action;
				EEPROM.put(action.id * sizeof(TimerAction), action);
				EEPROM.commit();

#ifdef DEBUG
				Serial.printf("TimerAction: %llu %llu\n", timestamp(), action.nextExecution);
#endif
				break;
			}
			case REMOVE_ACTION_PACKET:
			{
				unsigned char actionId = 0;
				client.readBytes(&actionId, 1);

				actions[actionId].activated = false;

				break;
			}
			case KEEPALIVE_PACKET:
			{
				// Ignore keep alive packet yolo.
				break;
			}
			default:
			{
				client.stop();
				break;
			}
			}
		}
	}

	do
	{
		controller.ShowColors();
	} while (micros64() <= time_now + 1000);
}