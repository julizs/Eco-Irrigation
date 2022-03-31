#ifndef ISubStateMachine_h
#define ISubStateMachine_h

class ISubStateMachine
{
    public:
        virtual ~ISubStateMachine() {}
        virtual void loop() = 0;
        virtual bool isDone() = 0;
        virtual void resetMachine() = 0;
};

#endif // ISubStateMachine