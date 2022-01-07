#include "plant.h"


//Plant::Plant(Fotoresistor &f): lightSensor(f){}

Plant::Plant(AmbientLight &l, SoilMoisture &s): lightSensor(l)
{
    soilMoistureSensors.push_back(s);
}

bool Plant::measureSensors()
{
    bool measurementsSuccessful;
    

    // Each Plant has 0-3 associated Soil Moisture Sensors
    if(soilMoistureSensors.size() > 0)
    {
        int i = 0;

        for(auto& sMS: soilMoistureSensors) {
        soilMoistureValues[i] = sMS.measureSoilMoistureSmoothed();
        i++;
        }
    }

    // Each Plant has 1 associated TSL2591
    lightSensor.setupTSL2591();
    TSL2591data data = lightSensor.measureLight();
    irLight = data.infraRed;
    visibleLight = data.visibleLight;
    
    return measurementsSuccessful;
}

bool Plant::setName(String newName)
{
    name = newName;
}

bool Plant::writeToInflux()
{
    p1.addTag("Plant", name);
    //p1.addTag("PlantGroup", plantGroup);

    p1.clearFields();

    if(soilMoistureSensors.size() > 0)
    {
        int i = 0;

        for(auto& sMS: soilMoistureSensors) {
        //influxHelper.writeDataPoint(p1, "Soil Moisture Sensor " + i, soilMoistureValues[i]);
        p1.addField("Soil Moisture Sensor " + i, soilMoistureValues[i]);
        i++;
        }
    } 

    //influxHelper.writeDataPoint(p1, "Infrared Light", irLight);
    //influxHelper.writeDataPoint(p1, "Visible Light", visibleLight);
    p1.addField("Infrared Light", irLight);
    p1.addField("Visible Light", visibleLight);

    Serial.println(p1.toLineProtocol());

    influxHelper.writeDataPoint(p1);

    return true;
}