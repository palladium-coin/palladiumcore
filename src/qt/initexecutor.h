// Copyright (c) 2014-2020 The Palladium Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PALLADIUM_QT_INITEXECUTOR_H
#define PALLADIUM_QT_INITEXECUTOR_H

#include <interfaces/node.h>

#include <exception>

#include <QObject>
#include <QThread>

QT_BEGIN_NAMESPACE
class QString;
QT_END_NAMESPACE

/** Class encapsulating Palladium Core startup and shutdown.
 * Allows running startup and shutdown in a different thread from the UI thread.
 */
class InitExecutor : public QObject
{
    Q_OBJECT
public:
    explicit InitExecutor(interfaces::Node& node);
    ~InitExecutor();

public Q_SLOTS:
    void initialize();
    void shutdown();

Q_SIGNALS:
    void initializeResult(bool success, interfaces::BlockAndHeaderTipInfo tip_info);
    void shutdownResult();
    void runawayException(const QString& message);
    // Internal queued requests executed in m_thread via m_context affinity.
    void requestedInitialize();
    void requestedShutdown();

private:
    /// Pass fatal exception message to UI thread
    void handleRunawayException(const std::exception* e);
    void runInitialization();
    void runShutdown();

    interfaces::Node& m_node;
    QObject m_context;
    QThread m_thread;
};

#endif // PALLADIUM_QT_INITEXECUTOR_H
