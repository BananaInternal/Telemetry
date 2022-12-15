#ifndef SINGLETON_H
#define SINGLETON_H

#include <KUserFeedback/provider.h>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkAccessManager>

class Telemetry : public QObject
{
private :
    static KUserFeedback::Provider *m_provider;
    static Telemetry *m_instance;
    Telemetry();
public:
    ~Telemetry() override;
    static KUserFeedback::Provider *provider();
    static Telemetry *instance();
    static void release();
    static void putRequest();
};

#endif // SINGLETON_H
