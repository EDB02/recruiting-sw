#include <stdio.h>
#include <iostream>
#include <time.h>
#include <unordered_map>
#include <string.h>

using namespace std;

extern "C"{
    #include "fake_receiver.h"
}

enum state{
    IDLE,
    RUN
} currentState;

enum message_type{
    START,
    STOP,
    GENERIC
};

struct stat{
    uint32_t number_of_messages;
    float mean_time;
    float last_time;
};

static const string logs_path = "./logs/", stats_path = "./stats/";
static FILE *log, *stats;
static bool opened = 0;
unordered_map<uint16_t, stat > stats_map;

int parse(const char *message, uint16_t &id, uint8_t* payload)
{
    char tmp[17];
    sscanf(message, "%hX#%s", &id, tmp);

    int size = strlen(tmp)/2;
    for(int i=0;i<size;i++)
    {
        sscanf(tmp, "%2hhX%s", &payload[i], tmp);
    }

    return size;
}

message_type get_type(uint16_t &id, uint8_t* payload, uint8_t size)
{
    if(size == 2)
    {
        if(id == 0x0A0)
        {
            if((payload[0]<<8) + payload[1] == 0x6601) return START;
            if((payload[0]<<8) + payload[1] == 0xFF01) return START;
            if((payload[0]<<8) + payload[1] == 0x66FF) return STOP;
        }
    }
    return GENERIC;
}


int main(void){
    char message[MAX_CAN_MESSAGE_SIZE];
    int bytes, index;
    uint16_t id;
    uint8_t payload[8], payload_s;
    long long int cur_time;
    float cur_time_ms; //cur_time in ms
    if(open_can("../candump.log"))
    {
        cout<<"errore apertura file\n";
        return -1;
    }
    while(1)
    {
        switch(currentState)
        {
        case IDLE:
            bytes = can_receive(message);
            payload_s = parse(message, id, payload);
            if(get_type(id, payload, payload_s) == START)
            {
                currentState = RUN;
            }
        break;
        case RUN:
            cur_time = time(0);
            cur_time_ms = clock()/(CLOCKS_PER_SEC/1000.0);

            if(!opened)
            {
                log = fopen((logs_path + to_string(cur_time) + ".log").c_str(), "w");
                stats = fopen((stats_path + to_string(cur_time) + ".csv").c_str(), "w");
                if(!log || !stats)
                {
                    cout<<"Impossibile creare i file. Forse mancano le cartelle stats e logs\n";
                    return -1;
                }
                opened = 1;
            }

            bytes = can_receive(message);

            payload_s = parse(message, id, payload);

            if(get_type(id, payload, payload_s) == START) //ignore start messages
            {
                cout<<"START"<<endl;
                continue;
            }

            if(get_type(id, payload, payload_s) == STOP)
            {
                for(auto i : stats_map)
                {
                    fprintf(stats, "%03X;%d;%.2f\n", i.first, i.second.number_of_messages, i.second.mean_time);
                }
                fclose(log);
                fclose(stats);
                opened = 0;
                stats_map.clear();
                currentState = IDLE;
                continue;
            }

            fprintf(log, "(%lld) %s\n", cur_time, message);

            if(!stats_map.count(id))
            {
                stats_map[id] = {1, 0, cur_time_ms};
            }
            else
            {
                stats_map[id].mean_time = ((stats_map[id].mean_time * stats_map[id].number_of_messages) + (cur_time_ms - stats_map[id].last_time)) / 
                                            (stats_map[id].number_of_messages+1.0);
                stats_map[id].last_time = cur_time_ms;
                stats_map[id].number_of_messages++;
            }

        break;
        }
    }

    close_can();

    return 0;
}