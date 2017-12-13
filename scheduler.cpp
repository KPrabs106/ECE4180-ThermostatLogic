#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "MQTTAsync.h"
#include <time.h>
#include <chrono>
#include <thread>
#include <string>

#define ADDRESS     "mqtt://m14.cloudmqtt.com:14522"
#define CLIENTID    "ExampleClientPub"
#define TOPIC       "MQTT Examples"
#define PAYLOAD     "Hello World!"
#define QOS         1
#define TIMEOUT     10000L

volatile int schedule_time[4] = {-1, -1, -1, -1};
volatile int temp[4] = {-1, -1, -1, -1};
volatile int idealTemp = -1;
volatile int scheduleOn = -1;
volatile bool changedData = false;
volatile MQTTAsync_token deliveredtoken;

int disc_finished = 0;
int subscribed = 0;
int finished = 0;


bool receivedData(){
    for(int i = 0; i < 4; i++){
        if(schedule_time[i] == -1){
            return false;
        }
        if(temp[i] == -1){
            return false;
        }
    }

    if(idealTemp == -1){
        return false;
    }

    if(scheduleOn == -1){
        return false;
    }

    return true;
}

void connlost(void *context, char *cause)
{
    MQTTAsync client = (MQTTAsync)context;
    MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
    int rc;
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
    printf("Reconnecting\n");
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
    {
            printf("Failed to start connect, return code %d\n", rc);
            finished = 1;
    }
}

int getTimeInMins(char* t){
    std::string time(t);
    int pos = time.find(":"); 
    return stoi(time.substr(0, pos)) * 60 + stoi(time.substr(pos+1));
}

int msgarrvd(void *context, char *topicName, int topicLen, MQTTAsync_message *message)
{
    int i;
    char* payloadptr;
    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);

    char buf[10];
    sprintf(buf, "%.*s", message->payloadlen, (char*)(message->payload) );

    std::string topic(topicName);

    if(topic == "ideal"){
        idealTemp = atoi(buf);
    }
    else if(topic == "scheduleOn"){
        scheduleOn = atoi(buf);
    }
    else if(topic == "schedule/1/temp"){
        temp[0] = atoi(buf);
    }
    else if(topic == "schedule/2/temp"){
        temp[1] = atoi(buf);
    }
    else if(topic == "schedule/3/temp"){
        temp[2] = atoi(buf);
    }
    else if(topic == "schedule/4/temp"){
        temp[3] = atoi(buf);
    }
    else if(topic == "schedule/1/time"){
        getTimeInMins(buf);
    }
    else if(topic == "schedule/2/time"){
        getTimeInMins(buf);
    }
    else if(topic == "schedule/3/time"){
        getTimeInMins(buf);
    }
    else if(topic == "schedule/4/time"){
        getTimeInMins(buf);
    }

    payloadptr = message->payload;
    for(i=0; i<message->payloadlen; i++)
    {
        putchar(*payloadptr++);
    }
    putchar('\n');

    MQTTAsync_freeMessage(&message);
    MQTTAsync_free(topicName);
    return 1;
}

void onDisconnect(void* context, MQTTAsync_successData* response)
{
    printf("Successful disconnection\n");
    finished = 1;
}

void onSubscribe(void* context, MQTTAsync_successData* response)
{
    printf("Subscribe succeeded\n");
}
void onSubscribeFailure(void* context, MQTTAsync_failureData* response)
{
    printf("Subscribe failed, rc %d\n", response ? response->code : 0);
    finished = 1;
}

void onConnectFailure(void* context, MQTTAsync_failureData* response)
{
    printf("Connect failed, rc %d\n", response ? response->code : 0);
    finished = 1;
}


void onSend(void* context, MQTTAsync_successData* response)
{
    printf("Sent!\n");
}

void sendTemp(MQTTAsync client, int temp){
    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    MQTTAsync_message pubmsg = MQTTAsync_message_initializer;

    int rc;
    opts.context = client;
    opts.onSuccess = onSend;

    char tempToSend[10];
    sprintf(tempToSend, "%d", temp);

    printf("sending %s\n", tempToSend);

    pubmsg.payload = tempToSend;
    pubmsg.payloadlen = strlen(tempToSend);
    pubmsg.qos = 1;
    pubmsg.retained = 1; 

    if ((rc = MQTTAsync_sendMessage(client, "finalTargetTemp", &pubmsg, &opts)) != MQTTASYNC_SUCCESS)
    {
        printf("Failed to start sendMessage, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }
}

void onConnect(void* context, MQTTAsync_successData* response)
{
    MQTTAsync client = (MQTTAsync)context;
    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    
    int rc;
    printf("Successful connection\n");
    
    opts.onSuccess = onSubscribe;
    opts.onFailure = onSubscribeFailure;
    opts.context = client;
    deliveredtoken = 0;
    printf("before sub\n");
    if ((rc = MQTTAsync_subscribe(client, "ideal", 2, &opts)) != MQTTASYNC_SUCCESS)
    {
            printf("Failed to start subscribe, return code %d\n", rc);
            exit(EXIT_FAILURE);
    }
    if ((rc = MQTTAsync_subscribe(client, "scheduleOn", 2, &opts)) != MQTTASYNC_SUCCESS)
    {
            printf("Failed to start subscribe, return code %d\n", rc);
            exit(EXIT_FAILURE);
    }
    if ((rc = MQTTAsync_subscribe(client, "schedule/1/temp", 2, &opts)) != MQTTASYNC_SUCCESS)
    {
            printf("Failed to start subscribe, return code %d\n", rc);
            exit(EXIT_FAILURE);
    }
    if ((rc = MQTTAsync_subscribe(client, "schedule/1/time", 2, &opts)) != MQTTASYNC_SUCCESS)
    {
            printf("Failed to start subscribe, return code %d\n", rc);
            exit(EXIT_FAILURE);
    }
    if ((rc = MQTTAsync_subscribe(client, "schedule/2/temp", 2, &opts)) != MQTTASYNC_SUCCESS)
    {
            printf("Failed to start subscribe, return code %d\n", rc);
            exit(EXIT_FAILURE);
    }
    if ((rc = MQTTAsync_subscribe(client, "schedule/2/time", 2, &opts)) != MQTTASYNC_SUCCESS)
    {
            printf("Failed to start subscribe, return code %d\n", rc);
            exit(EXIT_FAILURE);
    }
    if ((rc = MQTTAsync_subscribe(client, "schedule/3/temp", 2, &opts)) != MQTTASYNC_SUCCESS)
    {
            printf("Failed to start subscribe, return code %d\n", rc);
            exit(EXIT_FAILURE);
    }
    if ((rc = MQTTAsync_subscribe(client, "schedule/3/time", 2, &opts)) != MQTTASYNC_SUCCESS)
    {
            printf("Failed to start subscribe, return code %d\n", rc);
            exit(EXIT_FAILURE);
    }
    if ((rc = MQTTAsync_subscribe(client, "schedule/4/temp", 2, &opts)) != MQTTASYNC_SUCCESS)
    {
            printf("Failed to start subscribe, return code %d\n", rc);
            exit(EXIT_FAILURE);
    }
    if ((rc = MQTTAsync_subscribe(client, "schedule/4/time", 2, &opts)) != MQTTASYNC_SUCCESS)
    {
            printf("Failed to start subscribe, return code %d\n", rc);
            exit(EXIT_FAILURE);
    }

    while(!receivedData()){
        //std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    printf("received all \n");
    while(true){
        if(scheduleOn == 0){
            sendTemp(client, idealTemp);
        }
        else{
            time_t theTime = time(NULL);
            struct tm *aTime = localtime(&theTime);
            
            int timeInMins = aTime->tm_hour * 60 + aTime->tm_min;

            if(timeInMins < schedule_time[0]){
                sendTemp(client, temp[3]);
            }
            else if(timeInMins < schedule_time[1]){
                sendTemp(client, temp[0]);
            }
            else if(timeInMins < schedule_time[2]){
                sendTemp(client, temp[1]);
            }
            else if(timeInMins < schedule_time[3]){
                sendTemp(client, temp[2]);
            }
            else{
                sendTemp(client, temp[3]);
            }
        }
        printf("Waiting for a minute...\n");
        std::this_thread::sleep_for(std::chrono::minutes(1));
    }
}

int main(int argc, char* argv[])
{
    MQTTAsync client;
    MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
    MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
    MQTTAsync_token token;
    int rc;
    
    MQTTAsync_create(&client, "tcp://m14.cloudmqtt.com:14522", "Scheduler", MQTTCLIENT_PERSISTENCE_NONE, NULL);
    MQTTAsync_setCallbacks(client, NULL, connlost, msgarrvd, NULL);
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    conn_opts.struct_version = 0;
    conn_opts.onSuccess = onConnect;
    conn_opts.onFailure = onConnectFailure;

    conn_opts.username = "hyuurysh";
    conn_opts.password = "PSPd3wvV9Zi2";
    conn_opts.context = client;

    MQTTAsync_SSLOptions ssl_options = MQTTAsync_SSLOptions_initializer;
    ssl_options.enableServerCertAuth = 0;

    conn_opts.ssl = NULL;

    if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
    {
            printf("Failed to start connect, return code %d\n", rc);
            exit(EXIT_FAILURE);
    }
    
    while(!finished){

    }

    MQTTAsync_destroy(&client);
    return rc;

}   