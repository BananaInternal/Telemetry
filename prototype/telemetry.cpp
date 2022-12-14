#include "telemetry.h"
#include "QtNetwork/qnetworkaccessmanager.h"
#include <QCoreApplication>


KUserFeedback::Provider *Telemetry::m_provider = nullptr;
Telemetry *Telemetry::m_instance = nullptr;

constexpr char c_provederUrl[] = "http://localhost:8000/";

Telemetry::Telemetry()
{
    m_provider = new KUserFeedback::Provider(QCoreApplication::instance());
    m_provider->setFeedbackServer(QUrl(c_provederUrl));
    m_provider->setEnabled(true);
    m_provider->setTelemetryMode(KUserFeedback::Provider::TelemetryMode::DetailedUsageStatistics);
}

Telemetry::~Telemetry()
{
    release();
}

Telemetry *Telemetry::instance()
{
    if (m_instance == nullptr)
        m_instance = new Telemetry();
    return m_instance;
}

KUserFeedback::Provider *Telemetry::provider()
{
    if (m_instance == nullptr)
        m_instance = new Telemetry();
    return m_provider;
}

void Telemetry::release()
{
    if (m_instance != nullptr) {
        delete m_instance;
        m_instance = nullptr;
    }
}

void Telemetry::putRequest() {
    static QNetworkAccessManager manager(QCoreApplication::instance());
    QNetworkReply* reply = manager.put(QNetworkRequest(QUrl(c_provederUrl)),c_provederUrl);
}
