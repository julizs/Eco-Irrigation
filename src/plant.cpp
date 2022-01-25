#include "plant.h"


//Plant::Plant(Fotoresistor &f): lightSensor(f){}

Plant::Plant(AmbientLight &l, SoilMoisture &s): lightSensor(l)
{
    soilMoistureSensors.push_back(s);
}

bool Plant::measureSensors()
{
    bool measurementsSuccessful;
    soilMoistureValues.clear();
    
    // Each Plant has 0-3 associated Soil Moisture Sensors
    if(soilMoistureSensors.size() > 0)
    {
        for(auto& sMS: soilMoistureSensors) {
        //soilMoistureValues[i] = sMS.measureSoilMoistureSmoothed();
        int moisture = sMS.measureSoilMoistureSmoothed();
        soilMoistureValues.push_back(moisture);
        }
    }

    // Each Plant has 1 associated TSL2591
    lightSensor.setupTSL2591();
    TSL2591data data = lightSensor.measureLight();
    irLight = data.infraRed;
    visibleLight = data.visibleLight;

    // Do in doMeasurements() in main
    // Too many Connections (1 per Plant) to InfluxDB, SSL failed
    writeToInflux();
    
    return measurementsSuccessful;
}

bool Plant::setName(char newName[])
{
    strncpy(name, newName, 50);
}

bool Plant::writeToInflux()
{
    // Point p(name); One measurement/table per Plant?

    p1.clearTags(); // Same data Point gets reused for each Plant, so clear Tags
    p1.addTag("Plant", name);
    //p1.addTag("PlantGroup", plantGroup);
    p1.clearFields();

    int i = 0;

    for(const auto& value: soilMoistureValues) {
        char key[25];
        sprintf(key, "Soil Moisture Sensor %d", i);
        p1.addField(key, value);
        i++;
    }
    
    p1.addField("Infrared Light", irLight);
    p1.addField("Visible Light", visibleLight);

    //Serial.println(p1.toLineProtocol()); 
    influxHelper.writeDataPoint(p1);

    return true;
}