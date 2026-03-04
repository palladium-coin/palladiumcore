// Copyright (c) 2011-2020 The Palladium Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PALLADIUM_QT_PALLADIUM_H
#define PALLADIUM_QT_PALLADIUM_H

#if defined(HAVE_CONFIG_H)
#include <config/palladium-config.h>
#endif

#include <interfaces/node.h>

#include <QApplication>
#include <memory>

class PalladiumGUI;
class ClientModel;
class InitExecutor;
class NetworkStyle;
class OptionsModel;
class PaymentServer;
class PlatformStyle;
class WalletController;
class WalletModel;

namespace interfaces {
class Handler;
class Node;
} // namespace interfaces

/** Main Palladium application object */
class PalladiumApplication : public QApplication
{
    Q_OBJECT
public:
    explicit PalladiumApplication(interfaces::Node& node);
    ~PalladiumApplication();

#ifdef ENABLE_WALLET
    /// Create payment server
    void createPaymentServer();
#endif
    /// parameter interaction/setup based on rules
    void parameterSetup();
    /// Create options model
    void createOptionsModel(bool resetSettings);
    /// Initialize prune setting
    void InitializePruneSetting(bool prune);
    /// Create main window
    void createWindow(const NetworkStyle* networkStyle);
    /// Create splash screen
    void createSplashScreen(const NetworkStyle* networkStyle);
    /// Basic initialization, before starting initialization/shutdown thread. Return true on success.
    bool baseInitialize();

    /// Request core initialization
    void requestInitialize();
    /// Request core shutdown
    void requestShutdown();

    /// Get process return value
    int getReturnValue() const { return returnValue; }

    /// Get window identifier of QMainWindow (PalladiumGUI)
    WId getMainWinId() const;

    /// Setup platform style
    void setupPlatformStyle();

public Q_SLOTS:
    void initializeResult(bool success, interfaces::BlockAndHeaderTipInfo tip_info);
    void shutdownResult();
    /// Handle runaway exceptions. Shows a message box with the problem and quits the program.
    void handleRunawayException(const QString& message);

Q_SIGNALS:
    void requestedInitialize();
    void requestedShutdown();
    void splashFinished();
    void windowShown(PalladiumGUI* window);

private:
    interfaces::Node& m_node;
    std::unique_ptr<InitExecutor> m_executor;
    OptionsModel* optionsModel;
    ClientModel* clientModel;
    PalladiumGUI* window;
    QTimer* pollShutdownTimer;
#ifdef ENABLE_WALLET
    PaymentServer* paymentServer{nullptr};
    WalletController* m_wallet_controller{nullptr};
#endif
    int returnValue;
    const PlatformStyle* platformStyle;
    std::unique_ptr<QWidget> shutdownWindow;

    void startThread();
};

int GuiMain(int argc, char* argv[]);

#endif // PALLADIUM_QT_PALLADIUM_H
