#ifndef influxHelper_h
#define influxHelper_h

#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

#define INFLUXDB_URL "https://juli.uber.space"
//#define INFLUXDB_TOKEN "njMUu2TT8HUi7rmhWj_tq75nCkIIhKuODAlRoR4w7GNoLVHGiFvL26VAZ-fQ4w2sQennJPCH9FsVbch3_r6zaA=="
#define INFLUXDB_TOKEN "Fam5Vt7jSGMYsC3bCrpNXxdK2WX-VfAdrH6BE5RrYi94Btd1OFZ6VEQg7HFdduhqyKLcbHDpa6wGE9t6o_UkPQ=="
#define INFLUXDB_ORG "private"
#define INFLUXDB_BUCKET "messdaten"
#define TZ_INFO "CET-1CEST,M3.5.0,M10.5.0/3"

class InfluxHelper
{
    public:
    InfluxHelper();
    void setupInflux();
    bool checkInfluxConnection();
    void writeDataPoint(Point &p);
    void doQuery();
};

extern InfluxDBClient client;
extern Point p0, p1;

#endif // influxHelper_h