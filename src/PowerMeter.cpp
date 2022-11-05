#include <PowerMeter.h>

PowerMeter::PowerMeter(TwoWire &w) : wire(w)
{
  this->wire = wire;

  data.voltage = 0.0f;
  data.current = 0.0f;
  data.power = 0.0f;

  setup();
}

/*
*/
bool PowerMeter::setup()
{
    if (!ina219.begin(&wire)) // &I2Ctwo
    {
        Serial.println(F("Failed to boot Ina219"));
        return false;
    }

    ina219.powerSave(true);

    return true;
}

bool PowerMeter::isReady()
{
    ina219.setCalibration_32V_2A();
    bool ready = ina219.success();
    if(ready)
        Serial.println("INA 219 is ready.");

    return ready;
}

/*
Use isnan or constrain to prevent Infinity, otherwise InfluxDB Buffer Write fails
data.current = constrain(current, 0.0f, 4.0f);
*/
PowerData PowerMeter::measure()
{
    ina219.powerSave(false);
    
    data.busVoltage = ina219.getBusVoltage_V();
    // if(!ina219.success()) {return;} // Exit if any Measurement fails
    data.shuntVoltage = abs(ina219.getShuntVoltage_mV() / 1000.0f);
    data.voltage = data.busVoltage + data.shuntVoltage;
    data.current = abs(ina219.getCurrent_mA() / 1000.0f);

    // data.power = ina219.getPower_mW(); buggy

    // P = U * I
    data.power = data.busVoltage * data.current; 

    ina219.powerSave(true);

    return data;
}

/*
How to identify invalid Measurement?
0 is valid
*/
bool PowerMeter::measurementValid(const PowerData &measurement)
{
    if(isnan(measurement.voltage) || isnan(measurement.current))
    {
        Serial.println("Ina219 measurement Invalid.");
        return false;
    }

    return true;
}

bool PowerMeter::measureAndSubmit()
{
    PowerData measurement = measure();

    if(!measurementValid(measurement))
    {
        return false;
    }

    if(!writePoint(measurement))
        return false; 
    return true;
}

bool PowerMeter::writePoint(const PowerData &measurement)
{
    // Point p4("Power Usage");
    p4.clearTags();
    p4.clearFields();

    // Tags?
    p4.addField("voltage", measurement.voltage);
    p4.addField("current", measurement.current);
    p4.addField("power", measurement.power);

    if(!InfluxHelper::writeDataPoint(p4));
        return false;
    return true;
}

void PowerMeter::printMeasurement(const PowerData &measurement)
{
    char message[64];
    snprintf(message, 64, "PowerMeter: Voltage: %.2f, Current: %.2f, Power: %.2f", 
    measurement.voltage, measurement.current, measurement.power);

    Serial.println(message);
}
