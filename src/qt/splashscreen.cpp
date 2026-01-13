// Copyright (c) 2011-2019 The Palladium Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/palladium-config.h>
#endif

#include <qt/splashscreen.h>

#include <clientversion.h>
#include <interfaces/handler.h>
#include <interfaces/node.h>
#include <interfaces/wallet.h>
#include <qt/guiutil.h>
#include <qt/networkstyle.h>
#include <ui_interface.h>
#include <util/system.h>
#include <util/translation.h>

#include <QApplication>
#include <QCloseEvent>
#include <QPainter>
#include <QRadialGradient>
#include <QScreen>


SplashScreen::SplashScreen(interfaces::Node& node, Qt::WindowFlags f, const NetworkStyle *networkStyle) :
    QWidget(nullptr, f), curAlignment(0), m_node(node)
{
    // set reference point, paddings
    int paddingRight            = 20;
    int paddingTop              = 50;
    int titleVersionVSpace      = 17;
    int titleCopyrightVSpace    = 40;

    float fontFactor            = 1.3;
    float devicePixelRatio      = 1.0;
    devicePixelRatio = static_cast<QGuiApplication*>(QCoreApplication::instance())->devicePixelRatio();

    // define text to place
    QString titleText       = PACKAGE_NAME;
    QString versionText     = QString("Version %1").arg(QString::fromStdString(FormatFullVersion()));
    QString copyrightText   = QString::fromUtf8(CopyrightHolders(strprintf("\xc2\xA9 %u-%u ", 2024, COPYRIGHT_YEAR)).c_str());
    QString titleAddText    = networkStyle->getTitleAddText();

    QString font            = QApplication::font().toString();

    // create a bitmap according to device pixelratio
    QSize splashSize(480*devicePixelRatio,320*devicePixelRatio);
    pixmap = QPixmap(splashSize);

    // change to HiDPI if it makes sense
    pixmap.setDevicePixelRatio(devicePixelRatio);

    QPainter pixPaint(&pixmap);
    pixPaint.setPen(QColor(100,100,100));

    // draw a slightly radial gradient
    QRadialGradient gradient(QPoint(0,0), splashSize.width()/devicePixelRatio);
    gradient.setColorAt(0, Qt::white);
    gradient.setColorAt(1, QColor(247,247,247));
    QRect rGradient(QPoint(0,0), splashSize);
    pixPaint.fillRect(rGradient, gradient);

    // draw the palladium icon, expected size of PNG: 1024x1024
    QRect rectIcon(QPoint(-130,-122), QSize(430,430));

    const QSize requiredSize(1024,1024);
    QPixmap icon(networkStyle->getAppIcon().pixmap(requiredSize));

    pixPaint.drawPixmap(rectIcon, icon);

    // check font size and drawing with
    pixPaint.setFont(QFont(font, 33*fontFactor));
    QFontMetrics fm = pixPaint.fontMetrics();
    int titleTextWidth = GUIUtil::TextWidth(fm, titleText);
    if (titleTextWidth > 176) {
        fontFactor = fontFactor * 176 / titleTextWidth;
    }

    pixPaint.setFont(QFont(font, 33*fontFactor));
    fm = pixPaint.fontMetrics();
    titleTextWidth  = GUIUtil::TextWidth(fm, titleText);

    // Draw title with shadow effect
    int titleX = pixmap.width()/devicePixelRatio-titleTextWidth-paddingRight;
    int titleY = paddingTop;
    // Shadow (subtle offset with high opacity)
    pixPaint.setPen(QColor(255, 255, 255, 230));
    pixPaint.drawText(titleX+1, titleY+1, titleText);
    // Main text in very dark color for maximum contrast
    pixPaint.setPen(QColor(0, 0, 0));
    pixPaint.drawText(titleX, titleY, titleText);

    pixPaint.setFont(QFont(font, 15*fontFactor));

    // if the version string is too long, reduce size
    fm = pixPaint.fontMetrics();
    int versionTextWidth  = GUIUtil::TextWidth(fm, versionText);
    if(versionTextWidth > titleTextWidth+paddingRight-10) {
        pixPaint.setFont(QFont(font, 10*fontFactor));
        titleVersionVSpace -= 5;
    }

    // Draw version with shadow effect
    int versionX = pixmap.width()/devicePixelRatio-titleTextWidth-paddingRight+2;
    int versionY = paddingTop+titleVersionVSpace;
    // Shadow (subtle offset with high opacity)
    pixPaint.setPen(QColor(255, 255, 255, 230));
    pixPaint.drawText(versionX+1, versionY+1, versionText);
    // Main text in very dark color for maximum contrast
    pixPaint.setPen(QColor(0, 0, 0));
    pixPaint.drawText(versionX, versionY, versionText);

    // draw copyright stuff
    {
        pixPaint.setFont(QFont(font, 13*fontFactor));
        const int x = pixmap.width()/devicePixelRatio-titleTextWidth-paddingRight;
        const int y = paddingTop+titleCopyrightVSpace;
        QRect copyrightRect(x, y, pixmap.width() - x - paddingRight, pixmap.height() - y);
        // Shadow for copyright (subtle offset with high opacity)
        pixPaint.setPen(QColor(255, 255, 255, 230));
        copyrightRect.translate(1, 1);
        pixPaint.drawText(copyrightRect, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, copyrightText);
        // Main text for copyright in very dark color for maximum contrast
        copyrightRect.translate(-1, -1);
        pixPaint.setPen(QColor(0, 0, 0));
        pixPaint.drawText(copyrightRect, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, copyrightText);
    }

    // draw additional text if special network
    if(!titleAddText.isEmpty()) {
        QFont boldFont = QFont(font, 10*fontFactor);
        boldFont.setWeight(QFont::Bold);
        pixPaint.setFont(boldFont);
        fm = pixPaint.fontMetrics();
        int titleAddTextWidth  = GUIUtil::TextWidth(fm, titleAddText);
        pixPaint.drawText(pixmap.width()/devicePixelRatio-titleAddTextWidth-10,15,titleAddText);
    }

    pixPaint.end();

    // Set window title
    setWindowTitle(titleText + " " + titleAddText);

    // Resize window and move to center of desktop, disallow resizing
    QRect r(QPoint(), QSize(pixmap.size().width()/devicePixelRatio,pixmap.size().height()/devicePixelRatio));
    resize(r.size());
    setFixedSize(r.size());
    move(QGuiApplication::primaryScreen()->geometry().center() - r.center());

    subscribeToCoreSignals();
    installEventFilter(this);
}

SplashScreen::~SplashScreen()
{
    unsubscribeFromCoreSignals();
}

bool SplashScreen::eventFilter(QObject * obj, QEvent * ev) {
    if (ev->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(ev);
        if (keyEvent->key() == Qt::Key_Q) {
            m_node.startShutdown();
        }
    }
    return QObject::eventFilter(obj, ev);
}

void SplashScreen::finish()
{
    /* If the window is minimized, hide() will be ignored. */
    /* Make sure we de-minimize the splashscreen window before hiding */
    if (isMinimized())
        showNormal();
    hide();
    deleteLater(); // No more need for this
}

static void InitMessage(SplashScreen *splash, const std::string &message)
{
    bool invoked = QMetaObject::invokeMethod(splash, "showMessage",
        Qt::QueuedConnection,
        Q_ARG(QString, QString::fromStdString(message)),
        Q_ARG(int, Qt::AlignBottom|Qt::AlignHCenter),
        Q_ARG(QColor, QColor(55,55,55)));
    assert(invoked);
}

static void ShowProgress(SplashScreen *splash, const std::string &title, int nProgress, bool resume_possible)
{
    InitMessage(splash, title + std::string("\n") +
            (resume_possible ? _("(press q to shutdown and continue later)").translated
                                : _("press q to shutdown").translated) +
            strprintf("\n%d", nProgress) + "%");
}
#ifdef ENABLE_WALLET
void SplashScreen::ConnectWallet(std::unique_ptr<interfaces::Wallet> wallet)
{
    m_connected_wallet_handlers.emplace_back(wallet->handleShowProgress(std::bind(ShowProgress, this, std::placeholders::_1, std::placeholders::_2, false)));
    m_connected_wallets.emplace_back(std::move(wallet));
}
#endif

void SplashScreen::subscribeToCoreSignals()
{
    // Connect signals to client
    m_handler_init_message = m_node.handleInitMessage(std::bind(InitMessage, this, std::placeholders::_1));
    m_handler_show_progress = m_node.handleShowProgress(std::bind(ShowProgress, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
#ifdef ENABLE_WALLET
    m_handler_load_wallet = m_node.handleLoadWallet([this](std::unique_ptr<interfaces::Wallet> wallet) { ConnectWallet(std::move(wallet)); });
#endif
}

void SplashScreen::unsubscribeFromCoreSignals()
{
    // Disconnect signals from client
    m_handler_init_message->disconnect();
    m_handler_show_progress->disconnect();
    for (const auto& handler : m_connected_wallet_handlers) {
        handler->disconnect();
    }
    m_connected_wallet_handlers.clear();
    m_connected_wallets.clear();
}

void SplashScreen::showMessage(const QString &message, int alignment, const QColor &color)
{
    curMessage = message;
    curAlignment = alignment;
    curColor = color;
    update();
}

void SplashScreen::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.drawPixmap(0, 0, pixmap);
    QRect r = rect().adjusted(5, 5, -5, -5);
    painter.setPen(curColor);
    painter.drawText(r, curAlignment, curMessage);
}

void SplashScreen::closeEvent(QCloseEvent *event)
{
    m_node.startShutdown(); // allows an "emergency" shutdown during startup
    event->ignore();
}
