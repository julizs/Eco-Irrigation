#ifndef Irrigation_h
#define Irrigation_h

#include <main.h>
#include <Services.h>

class Irrigation
{
    public:
        static void decideIrrigation();
        static void getSolenoidInfo(const char *irrigationSubject, int irrigationAmount);
        static void getPumpInfo(const char* pumpName, int irrigationAmount);
        
};

#endif // Irrigation_h