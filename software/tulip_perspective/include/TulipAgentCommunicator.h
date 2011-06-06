/*
 * This file was generated by qdbusxml2cpp version 0.7
 * Command line was: qdbusxml2cpp -v -c TulipAgentCommunicator -p include/TulipAgentCommunicator.h:src/TulipAgentCommunicator.cpp ../tulip/resources/org.labri.TulipAgentService.xml
 *
 * qdbusxml2cpp is Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
 *
 * This is an auto-generated file.
 * Do not edit! All changes made to it will be lost.
 */

#ifndef TULIPAGENTCOMMUNICATOR_H_1307077630
#define TULIPAGENTCOMMUNICATOR_H_1307077630

#include <QtCore/QObject>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtDBus/QtDBus>

/*
 * Proxy class for interface org.labri.Tulip
 */
class TulipAgentCommunicator: public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char *staticInterfaceName()
    { return "org.labri.Tulip"; }

public:
    TulipAgentCommunicator(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent = 0);

    ~TulipAgentCommunicator();

    Q_PROPERTY(qlonglong pid READ pid)
    inline qlonglong pid() const
    { return qvariant_cast< qlonglong >(property("pid")); }

public Q_SLOTS: // METHODS
    inline QDBusPendingReply<> ShowAboutPage()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QLatin1String("ShowAboutPage"), argumentList);
    }

    inline QDBusPendingReply<> ShowPluginsCenter()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QLatin1String("ShowPluginsCenter"), argumentList);
    }

    inline QDBusPendingReply<> ShowWelcomeScreen()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QLatin1String("ShowWelcomeScreen"), argumentList);
    }

Q_SIGNALS: // SIGNALS
};

namespace org {
  namespace labri {
    typedef ::TulipAgentCommunicator Tulip;
  }
}
#endif
