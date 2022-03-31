#ifndef ISubStateMachine_h
#define ISubStateMachine_h

class ISubStateMachine
{
    public:
        virtual ~ISubStateMachine() {}
        virtual bool isDone() = 0;
};

#endif // ISubStateMachine