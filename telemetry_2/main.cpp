#include <stdio.h>
#include <iostream>
using namespace std;

enum state{
    IDLE,
    RUN
} currentState;

extern "C"{
    #include "fake_receiver.h"
}

uint16_t get_id(char *message, int ind)
{
    uint16_t ret = 0;
    for(int i=0;i<ind;i++)
    {
        if(message[i] >= 'A' && message[i] <= 'F') ret |= (message[i]-'A'+10)<<(4*(ind-i-1));
        else ret |= (message[i]-'0')<<(4*(ind-i-1));
    }
    return ret;
}
uint64_t get_payload(char *message, int ind, int size)
{
    uint64_t ret = 0;
    for(int i=ind+1;i<size;i++)
    {
        if(message[i] >= 'A' && message[i] <= 'F') ret |= (message[i]-'A'+10)<<(4*(ind-i-1));
        else ret |= (message[i]-'0')<<(4*(ind-i-1));
    }
    return ret;
}
int split(char *message, int size)
{
    for(int i=0;i<size;i++)
    {
        if(message[i] == '#') return i;
    }
    return -1;
}

int main(void){
    char message[20];
    int bytes, index;
    uint16_t id;
    uint64_t payload;
    switch(currentState)
    {
        case IDLE:
            bytes = can_receive(message);
            index = split(message, bytes);
            id = get_id(message, index);
            payload = get_payload(message, index, bytes);
            if(id == 0x0A0 && (payload == 0x6601 || payload == 0xFF01))
            {
                currentState = RUN;
            }
            break;
        case RUN:
            cout<<"im running\n";
            break;
    }

    return 0;
}