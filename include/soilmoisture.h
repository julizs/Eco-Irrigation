#ifndef soilmoisture_h
#define soilmoisture_h

class SoilMoisture
{
public:
    // Individueller "Floor" jedes Moisture Sensors 
    // (max. Feuchtigkeit = ca. 400-550 statt 0)
    int sensorErrorRate;
    int sampleRate;
    int analogPin;

    SoilMoisture(int sensorErrorRate);

    float SmoothedSoilMoisture();
    float MeasureSoilMoisture();


private:
    int GetSoilMoisture();
};

#endif //soilmoisture_h