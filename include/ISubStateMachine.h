#ifndef ISubStateMachine_h
#define ISubStateMachine_h

// https://stackoverflow.com/questions/9756893/how-to-implement-interfaces-in-c
class ISubStateMachine
{
    public:
        virtual ~ISubStateMachine() {};
        virtual void loop() = 0;
        virtual bool isDone() = 0;
        virtual void resetMachine() = 0;
};

#endif // ISubStateMachine