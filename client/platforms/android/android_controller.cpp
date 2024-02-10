#include <QJniEnvironment>
#include <QJsonDocument>
#include <QQmlFile>
#include <QEventLoop>

#include "android_controller.h"
#include "android_utils.h"
#include "ui/controllers/importController.h"

namespace
{
    AndroidController *s_instance = nullptr;

    constexpr auto QT_ANDROID_CONTROLLER_CLASS = "org/amnezia/vpn/qt/QtAndroidController";
    constexpr auto ANDROID_LOG_CLASS = "org/amnezia/vpn/util/Log";
    constexpr auto TAG = "AmneziaQt";
} // namespace

AndroidController::AndroidController() : QObject()
{
    connect(this, &AndroidController::status, this,
            [this](AndroidController::ConnectionState state) {
                qDebug() << "Android event: status =" << textConnectionState(state);
                if (isWaitingStatus) {
                    qDebug() << "Initialization by service status";
                    isWaitingStatus = false;
                    emit initConnectionState(convertState(state));
                }
            },
            Qt::QueuedConnection);

    connect(
        this, &AndroidController::serviceDisconnected, this,
        [this]() {
            qDebug() << "Android event: service disconnected";
            isWaitingStatus = true;
            emit connectionStateChanged(Vpn::ConnectionState::Disconnected);
        },
        Qt::QueuedConnection);

    connect(
        this, &AndroidController::serviceError, this,
        [this]() {
            qDebug() << "Android event: service error";
            // todo: add error message
            emit connectionStateChanged(Vpn::ConnectionState::Error);
        },
        Qt::QueuedConnection);

    connect(
        this, &AndroidController::vpnPermissionRejected, this,
        [this]() {
            qWarning() << "Android event: VPN permission rejected";
            emit connectionStateChanged(Vpn::ConnectionState::Disconnected);
        },
        Qt::QueuedConnection);

    connect(
        this, &AndroidController::vpnConnected, this,
        [this]() {
            qDebug() << "Android event: VPN connected";
            emit connectionStateChanged(Vpn::ConnectionState::Connected);
        },
        Qt::QueuedConnection);

    connect(
        this, &AndroidController::vpnDisconnected, this,
        [this]() {
            qDebug() << "Android event: VPN disconnected";
            emit connectionStateChanged(Vpn::ConnectionState::Disconnected);
        },
        Qt::QueuedConnection);

    connect(
        this, &AndroidController::vpnReconnecting, this,
        [this]() {
            qDebug() << "Android event: VPN reconnecting";
            emit connectionStateChanged(Vpn::ConnectionState::Reconnecting);
        },
        Qt::QueuedConnection);

    connect(
        this, &AndroidController::configImported, this,
        [this](const QString& config) {
            qDebug() << "Android event: config import";
            emit importConfigFromOutside(config);
        },
        Qt::QueuedConnection);
}

AndroidController *AndroidController::instance()
{
    if (!s_instance) {
        s_instance = new AndroidController();
    }

    return s_instance;
}

bool AndroidController::initialize()
{
    qDebug() << "Initialize AndroidController";

    const JNINativeMethod methods[] = {
        {"onStatus", "(I)V", reinterpret_cast<void *>(onStatus)},
        {"onServiceDisconnected", "()V", reinterpret_cast<void *>(onServiceDisconnected)},
        {"onServiceError", "()V", reinterpret_cast<void *>(onServiceError)},
        {"onVpnPermissionRejected", "()V", reinterpret_cast<void *>(onVpnPermissionRejected)},
        {"onVpnConnected", "()V", reinterpret_cast<void *>(onVpnConnected)},
        {"onVpnDisconnected", "()V", reinterpret_cast<void *>(onVpnDisconnected)},
        {"onVpnReconnecting", "()V", reinterpret_cast<void *>(onVpnReconnecting)},
        {"onStatisticsUpdate", "(JJ)V", reinterpret_cast<void *>(onStatisticsUpdate)},
        {"onFileOpened", "(Ljava/lang/String;)V", reinterpret_cast<void *>(onFileOpened)},
        {"onConfigImported", "(Ljava/lang/String;)V", reinterpret_cast<void *>(onConfigImported)},
        {"decodeQrCode", "(Ljava/lang/String;)Z", reinterpret_cast<bool *>(decodeQrCode)}
    };

    QJniEnvironment env;
    bool registered = env.registerNativeMethods(QT_ANDROID_CONTROLLER_CLASS, methods,
                                                sizeof(methods) / sizeof(JNINativeMethod));
    if (!registered) {
        qCritical() << "Failed native method registration";
        return false;
    }
    qtAndroidControllerInitialized();
    return true;
}

// static
template <typename Ret, typename ...Args>
auto AndroidController::callActivityMethod(const char *methodName, const char *signature, Args &&...args)
{
    qDebug() << "Call activity method:" << methodName;
    QJniObject activity = AndroidUtils::getActivity();
    Q_ASSERT(activity.isValid());
    return activity.callMethod<Ret>(methodName, signature, std::forward<Args>(args)...);
}

// static
template <typename ...Args>
void AndroidController::callActivityMethod(const char *methodName, const char *signature, Args &&...args)
{
    callActivityMethod<void>(methodName, signature, std::forward<Args>(args)...);
}

ErrorCode AndroidController::start(const QJsonObject &vpnConfig)
{
    isWaitingStatus = false;
    auto config = QJsonDocument(vpnConfig).toJson();
    callActivityMethod("start", "(Ljava/lang/String;)V",
                       QJniObject::fromString(config).object<jstring>());

    return NoError;
}

void AndroidController::stop()
{
    callActivityMethod("stop", "()V");
}

void AndroidController::saveFile(const QString &fileName, const QString &data)
{
    callActivityMethod("saveFile", "(Ljava/lang/String;Ljava/lang/String;)V",
                       QJniObject::fromString(fileName).object<jstring>(),
                       QJniObject::fromString(data).object<jstring>());
}

QString AndroidController::openFile(const QString &filter)
{
    QEventLoop wait;
    QString fileName;
    connect(this, &AndroidController::fileOpened, this,
            [&fileName, &wait](const QString &uri) {
                qDebug() << "Android event: file opened; uri:" << uri;
                fileName = QQmlFile::urlToLocalFileOrQrc(uri);
                qDebug() << "Android opened filename:" << fileName;
                wait.quit();
            },
            static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::SingleShotConnection));
    callActivityMethod("openFile", "(Ljava/lang/String;)V",
                       QJniObject::fromString(filter).object<jstring>());
    wait.exec();
    return fileName;
}

void AndroidController::setNotificationText(const QString &title, const QString &message, int timerSec)
{
    callActivityMethod("setNotificationText", "(Ljava/lang/String;Ljava/lang/String;I)V",
                       QJniObject::fromString(title).object<jstring>(),
                       QJniObject::fromString(message).object<jstring>(),
                       (jint) timerSec);
}

bool AndroidController::isCameraPresent()
{
    return callActivityMethod<jboolean>("isCameraPresent", "()Z");
}

void AndroidController::startQrReaderActivity()
{
    callActivityMethod("startQrCodeReader", "()V");
}

void AndroidController::setSaveLogs(bool enabled)
{
    callActivityMethod("setSaveLogs", "(Z)V", enabled);
}

void AndroidController::exportLogsFile(const QString &fileName)
{
    callActivityMethod("exportLogsFile", "(Ljava/lang/String;)V",
                       QJniObject::fromString(fileName).object<jstring>());
}

void AndroidController::clearLogs()
{
    callActivityMethod("clearLogs", "()V");
}

// Moving log processing to the Android side
jclass AndroidController::log;
jmethodID AndroidController::logDebug;
jmethodID AndroidController::logInfo;
jmethodID AndroidController::logWarning;
jmethodID AndroidController::logError;
jmethodID AndroidController::logFatal;

// static
bool AndroidController::initLogging()
{
    QJniEnvironment env;

    log = env.findClass(ANDROID_LOG_CLASS);
    if (log == nullptr) {
        qCritical() << "Android log class" << ANDROID_LOG_CLASS << "not found";
        return false;
    }

    auto logMethodSignature = "(Ljava/lang/String;Ljava/lang/String;)V";

    logDebug = env.findStaticMethod(log, "d", logMethodSignature);
    if (logDebug == nullptr) {
        qCritical() << "Android debug log method not found";
        return false;
    }

    logInfo = env.findStaticMethod(log, "i", logMethodSignature);
    if (logInfo == nullptr) {
        qCritical() << "Android info log method not found";
        return false;
    }

    logWarning = env.findStaticMethod(log, "w", logMethodSignature);
    if (logWarning == nullptr) {
        qCritical() << "Android warning log method not found";
        return false;
    }

    logError = env.findStaticMethod(log, "e", logMethodSignature);
    if (logError == nullptr) {
        qCritical() << "Android error log method not found";
        return false;
    }

    logFatal = env.findStaticMethod(log, "f", logMethodSignature);
    if (logFatal == nullptr) {
        qCritical() << "Android fatal log method not found";
        return false;
    }

    qInstallMessageHandler(messageHandler);
    return true;
}

// static
void AndroidController::messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    jmethodID logMethod = logDebug;
    switch (type) {
        case QtDebugMsg:
            logMethod = logDebug;
            break;
        case QtInfoMsg:
            logMethod = logInfo;
            break;
        case QtWarningMsg:
            logMethod = logWarning;
            break;
        case QtCriticalMsg:
            logMethod = logError;
            break;
        case QtFatalMsg:
            logMethod = logFatal;
            break;
    }
    QString formattedMessage = qFormatLogMessage(type, context, message);
    QJniObject::callStaticMethod<void>(log, logMethod,
                                       QJniObject::fromString(TAG).object<jstring>(),
                                       QJniObject::fromString(formattedMessage).object<jstring>());
}

void AndroidController::qtAndroidControllerInitialized()
{
    callActivityMethod("qtAndroidControllerInitialized", "()V");
}

// static
Vpn::ConnectionState AndroidController::convertState(AndroidController::ConnectionState state)
{
    switch (state) {
        case AndroidController::ConnectionState::CONNECTED: return Vpn::ConnectionState::Connected;
        case AndroidController::ConnectionState::CONNECTING: return Vpn::ConnectionState::Connecting;
        case AndroidController::ConnectionState::DISCONNECTED: return Vpn::ConnectionState::Disconnected;
        case AndroidController::ConnectionState::DISCONNECTING: return Vpn::ConnectionState::Disconnecting;
        case AndroidController::ConnectionState::RECONNECTING: return Vpn::ConnectionState::Reconnecting;
        case AndroidController::ConnectionState::UNKNOWN: return Vpn::ConnectionState::Unknown;
    }
}

// static
QString AndroidController::textConnectionState(AndroidController::ConnectionState state)
{
    switch (state) {
        case AndroidController::ConnectionState::CONNECTED: return "CONNECTED";
        case AndroidController::ConnectionState::CONNECTING: return "CONNECTING";
        case AndroidController::ConnectionState::DISCONNECTED: return "DISCONNECTED";
        case AndroidController::ConnectionState::DISCONNECTING: return "DISCONNECTING";
        case AndroidController::ConnectionState::RECONNECTING: return "RECONNECTING";
        case AndroidController::ConnectionState::UNKNOWN: return "UNKNOWN";
    }
}

// JNI functions called by Android
// static
void AndroidController::onStatus(JNIEnv *env, jobject thiz, jint stateCode)
{
    Q_UNUSED(env);
    Q_UNUSED(thiz);

    auto state = ConnectionState(stateCode);

    emit AndroidController::instance()->status(state);
}

// static
void AndroidController::onServiceDisconnected(JNIEnv *env, jobject thiz)
{
    Q_UNUSED(env);
    Q_UNUSED(thiz);

    emit AndroidController::instance()->serviceDisconnected();
}

// static
void AndroidController::onServiceError(JNIEnv *env, jobject thiz)
{
    Q_UNUSED(env);
    Q_UNUSED(thiz);

    emit AndroidController::instance()->serviceError();
}

// static
void AndroidController::onVpnPermissionRejected(JNIEnv *env, jobject thiz)
{
    Q_UNUSED(env);
    Q_UNUSED(thiz);

    emit AndroidController::instance()->vpnPermissionRejected();
}

// static
void AndroidController::onVpnConnected(JNIEnv *env, jobject thiz)
{
    Q_UNUSED(env);
    Q_UNUSED(thiz);

    emit AndroidController::instance()->vpnConnected();
}

// static
void AndroidController::onVpnDisconnected(JNIEnv *env, jobject thiz)
{
    Q_UNUSED(env);
    Q_UNUSED(thiz);

    emit AndroidController::instance()->vpnDisconnected();
}

// static
void AndroidController::onVpnReconnecting(JNIEnv *env, jobject thiz)
{
    Q_UNUSED(env);
    Q_UNUSED(thiz);

    emit AndroidController::instance()->vpnReconnecting();
}

// static
void AndroidController::onStatisticsUpdate(JNIEnv *env, jobject thiz, jlong rxBytes, jlong txBytes)
{
    Q_UNUSED(env);
    Q_UNUSED(thiz);

    emit AndroidController::instance()->statisticsUpdated((quint64) rxBytes, (quint64) txBytes);
}

// static
void AndroidController::onFileOpened(JNIEnv *env, jobject thiz, jstring uri)
{
    Q_UNUSED(thiz);

    emit AndroidController::instance()->fileOpened(AndroidUtils::convertJString(env, uri));
}

// static
void AndroidController::onConfigImported(JNIEnv *env, jobject thiz, jstring data)
{
    Q_UNUSED(thiz);

    emit AndroidController::instance()->configImported(AndroidUtils::convertJString(env, data));
}

// static
bool AndroidController::decodeQrCode(JNIEnv *env, jobject thiz, jstring data)
{
    Q_UNUSED(thiz);

    return ImportController::decodeQrCode(AndroidUtils::convertJString(env, data));
}
