#ifndef SINGLETON_H
#define SINGLETON_H

#include <KUserFeedback/provider.h>

class Singleton
{
public:
    Singleton(QObject * app,QUrl & url){
        static KUserFeedback::Provider lifeSpanProvider(app);
        lifeSpanProvider.setFeedbackServer(url);// To be changed
        lifeSpanProvider.setEnabled(true);
        lifeSpanProvider.DetailedUsageStatistics;
    }

};

#endif // SINGLETON_H
