#ifndef Action_h
#define Action_h

#include <ISubStateMachine.h>

class Action
{
    public:
        float duration;
        ISubStateMachine *stateMachine;
};

#endif // Action_h