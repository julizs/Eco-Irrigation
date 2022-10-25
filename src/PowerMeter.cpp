#include <PowerMeter.h>

PowerMeter::PowerMeter(TwoWire &w) : wire(w)
{
  this->wire = wire;
  setupIna();

  // Default Data, incase Sensor doesnt init, prevent random Data in InfluxDB
  measurement.voltage = 0.0f;
  measurement.current = 0.0f;
  measurement.power = 0.0f;
}

/*
*/
bool PowerMeter::setupIna()
{
    if (!ina219.begin(&wire)) // &I2Ctwo
    {
        Serial.println(F("Failed to boot Ina219"));
        return false;
    }

    ina219.powerSave(true);

    return true;
}

bool PowerMeter::inaReady()
{
    ina219.setCalibration_32V_2A();
    bool ready = ina219.success();
    if(ready)
        Serial.println("INA 219 is ready.");

    return ready;
}

/*
Use isnan, constrain or max on current to prevent Infinity, otherwise InfluxDB Buffer Write fails
data.current = max(current, 24.0f);
data.current = constrain(current, 0.0f, 4.0f);
*/
PowerData PowerMeter::measureIna()
{
    ina219.powerSave(false);
    
    measurement.busVoltage = ina219.getBusVoltage_V();
    measurement.shuntVoltage = abs(ina219.getShuntVoltage_mV() / 1000.0f);
    measurement.voltage = measurement.busVoltage + measurement.shuntVoltage;

    float current = abs(ina219.getCurrent_mA() / 1000.0f);
    if(isnan(current))
       measurement.current = 0.0f;

    // data.power = ina219.getPower_mW(); buggy
    measurement.power = measurement.busVoltage * measurement.current; // P = U * I

    ina219.powerSave(true);
    
    // writePoint();

    return measurement;
}

void PowerMeter::writePoint()
{
    // PowerData data = measureIna();

    // Point p4("Power Usage");
    p4.clearTags();
    p4.clearFields();

    // Tags?
    p4.addField("voltage", measurement.voltage);
    p4.addField("current", measurement.current);
    p4.addField("power", measurement.power);

    InfluxHelper::writeDataPoint(p4);

    char message[64];
    snprintf(message, 64, "Powermeter: Voltage: %.2f, Current: %.2f, Power: %.2f", measurement.voltage, measurement.current, measurement.power);
    Serial.println(message);
}
