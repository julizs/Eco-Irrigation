#ifndef Irrigation_h
#define Irrigation_h

#include <main.h>
#include <Services.h>

class Irrigation
{
    public:
        void decideIrrigation();
        void getSolenoidInfo(const char *irrigationSubject, int irrigationAmount);
        void getPumpInfo(const char* pumpName, int irrigationAmount);
        
};

#endif // Irrigation_h