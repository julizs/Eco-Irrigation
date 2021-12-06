#ifndef influxHelper_h
#define influxHelper_h

#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

#define INFLUXDB_URL "https://juli.uber.space"
#define INFLUXDB_TOKEN "njMUu2TT8HUi7rmhWj_tq75nCkIIhKuODAlRoR4w7GNoLVHGiFvL26VAZ-fQ4w2sQennJPCH9FsVbch3_r6zaA=="
#define INFLUXDB_ORG "private"
#define INFLUXDB_BUCKET "messdaten"
#define TZ_INFO "CET-1CEST,M3.5.0,M10.5.0/3"

class InfluxHelper
{
    public:
    InfluxHelper();
    void setupInflux();
    bool checkInfluxConnection();
    void writeDataPoint(int value);
    void doQuery();
};

extern InfluxDBClient client;
extern Point sensor;

#endif // influxHelper_h