#include <PowerMeter.h>

PowerMeter::PowerMeter(TwoWire &w) : wire(w)
{
  this->wire = wire;
  setupIna();
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

    PowerData data;
    
    data.busVoltage = ina219.getBusVoltage_V();
    data.shuntVoltage = abs(ina219.getShuntVoltage_mV() / 1000.0f);
    data.voltage = data.busVoltage + data.shuntVoltage;

    float current = abs(ina219.getCurrent_mA() / 1000.0f);
    if(isnan(current))
        data.current = 0.0f;

    data.power = ina219.getPower_mW();
    // data.power = data.busVoltage * data.current; // P = U * I

    ina219.powerSave(true);

    return data;
}

void PowerMeter::writePoint()
{
    PowerData data = measureIna();
    p0.addField("voltage", data.voltage);
    p0.addField("current", data.current);
    p0.addField("power", data.power);
    // p0.addField("busVoltage", inaData.busVoltage);
    // p0.addField("shuntVoltage", inaData.shuntVoltage);
    // delay(200);

    Serial.print("Power Readings INA 219: ");
    Serial.print(data.voltage);
    Serial.print(" ");
    Serial.print(data.current);
    Serial.print(" ");
    Serial.println(data.power);
}
