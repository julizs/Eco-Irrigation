#include <ISubStateMachine.h>

struct StateWrapper
{
    State *state;
    bool done;
    uint8_t minTime, errorCode;
};