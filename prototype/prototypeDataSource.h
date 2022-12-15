#ifndef PROTOTYPEDATASOURCE_H
#define PROTOTYPEDATASOURCE_H
#include <KUserFeedback/abstractdatasource.h>
#include <KUserFeedback/provider.h>
class prototypeDataSource : KUserFeedback::AbstractDataSource {
    public:
    prototypeDataSource():
        AbstractDataSource (QStringLiteral("mdiDataSource"), KUserFeedback::Provider::TelemetryMode::DetailedUsageStatistics) {}

//    QVariant data() override
//        {
//            return new QVariant();
//        }


};

#endif // PROTOTYPEDATASOURCE_H
