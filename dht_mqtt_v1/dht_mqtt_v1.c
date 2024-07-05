#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <time.h>
#include <string.h>
#include <MQTTClient.h>
//------MQTT----------
#define CLIENTID    "ExampleClientPub"
#define QOS         1
#define TIMEOUT     10000L
double DELAY;
//--------------------

#define MAXTIMINGS	85
#define DHTPIN		7
int dht11_dat[5] = { 0, 0, 0, 0, 0 };
 
void read_dht11_dat()
{
	uint8_t laststate	= HIGH;
	uint8_t counter		= 0;
	uint8_t j		= 0, i;
	float	f; 
 
	dht11_dat[0] = dht11_dat[1] = dht11_dat[2] = dht11_dat[3] = dht11_dat[4] = 0;
 
	pinMode( DHTPIN, OUTPUT );
	digitalWrite( DHTPIN, LOW );
	delay( 18 );
	digitalWrite( DHTPIN, HIGH );
	delayMicroseconds( 40 );
	pinMode( DHTPIN, INPUT );
 
	for ( i = 0; i < MAXTIMINGS; i++ )
	{
		counter = 0;
		while ( digitalRead( DHTPIN ) == laststate )
		{
			counter++;
			delayMicroseconds( 2 );
			if ( counter == 255 )
			{
				break;
			}
		}
		laststate = digitalRead( DHTPIN );
 
		if ( counter == 255 )
			break;
 
		if ( (i >=4) && (i % 2 == 0) )
		{
			dht11_dat[j / 8] <<= 1;
			if ( counter > 16 )
				dht11_dat[j / 8] |= 1;
			j++;
		}
	}
 	if ( (j >= 39) &&
	     (dht11_dat[4] == ( (dht11_dat[0] + dht11_dat[1] + dht11_dat[2] + dht11_dat[3]) & 0xFF) ) )
	{
		f = dht11_dat[2] * 9. / 5. + 32; 
		printf( "Humidity = %d.%d %% Temperature = %d.%d C (%.1f F)\n",
			dht11_dat[0], dht11_dat[1], dht11_dat[2], dht11_dat[3], f );
	}else  {
		printf( "Data not good, skip\n" );
	}
}
 
int main( int argc, char **argv)
{
if (argc != 6)
	{
	printf("***Information to help you run the program correctly***\n");
	
printf("______________  _________ ______  ___      __________ \n"); 
printf("___  __ \\__  / / /__  __/ ___   |/  /_____ __  /__  /_\n");
printf("__  / / /_  /_/ /__  /    __  /|_/ /_  __ `/  __/  __/\n");
printf("_  /_/ /_  __  / _  /     _  /  / / / /_/ // /_ / /_  \n");
printf("/_____/ /_/ /_/  /_/______/_/  /_/  \\__, / \\__/ \\__/  \n");
printf("                   _/_____/           /_/             \n");
printf("========================================================\n");
printf("For the program to work correctly, you must specify 5 parameters separated by a space:\n");
printf("1. Address and port\n");
printf("2. Username\n");
printf("3. Password\n");
printf("4. Topic for Temperature\n");
printf("5. Topic for Humidity\n");
printf("========================================================\n");
printf("*****************************************************************************************\n");
printf("* example: ./dht_mqtt_v1 192.168.100.120:1883 IoT student1 /node-red/temp /node-red/hum *\n");
printf("*****************************************************************************************\n");
	return 2;
	}
	printf("Enter interval for sending packets in seconds: ");
	scanf("%lf",&DELAY);
	printf("\n");
	printf( "Raspberry Pi wiringPi DHT11 Temperature and Humidity\n" );
	printf("******START TO WORK****** \n");
	
	
//-------------MQTT init----------------------
	MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    int rc;

    MQTTClient_create(&client, argv[1], CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    conn_opts.username = argv[2];//"IoT";
    conn_opts.password = argv[3];//"student1";
    

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc);
        exit(-1);
    }
 
	if ( wiringPiSetup() == -1 )
	{
		printf("Failed to initlib WiringPI\n");
		exit( 1 );
	}

	
	while ( 1)
	{
		clock_t begin = clock();
		
		read_dht11_dat();
		//--------topic temperature---------
		char tem[25];
        sprintf(tem,"%d.%d",dht11_dat[2],dht11_dat[3]);
        
        pubmsg.payload = tem;
        pubmsg.payloadlen = strlen(tem);
        pubmsg.qos = QOS;
        pubmsg.retained = 0;
        MQTTClient_publishMessage(client, argv[4], &pubmsg, &token);
        MQTTClient_waitForCompletion(client, token, TIMEOUT);
        printf("Temperature with delivery token %d delivered\n", token);
        //--------topic humidity---------
		char hum[25];
        sprintf(hum,"%d.%d",dht11_dat[0],dht11_dat[1]);
        
        pubmsg.payload = hum;
        pubmsg.payloadlen = strlen(hum);
        pubmsg.qos = QOS;
        pubmsg.retained = 0;
        MQTTClient_publishMessage(client, argv[5], &pubmsg, &token);
        MQTTClient_waitForCompletion(client, token, TIMEOUT);
        
        printf("Humidity with delivery token %d delivered\n", token);
        
        while ((double)(clock() - begin)/CLOCKS_PER_SEC<DELAY)
        {}
	}
	MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
	return(0);
}
