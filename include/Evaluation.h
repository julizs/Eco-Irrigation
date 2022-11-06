#ifndef Evaluation_h
#define Evaluation_h

#include <Irrigation.h>

/*
Evaluates overall Plant Happiness
Friend of all classes which that are part of Plant Happiness Eval Algorithm
*/
class Evaluation
{
    public:
    static Irrigation irr;
    bool evaluatePlants();
    
    private:
    ;
};

#endif // Evaluation_h