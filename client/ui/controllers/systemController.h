#ifndef SYSTEMCONTROLLER_H
#define SYSTEMCONTROLLER_H

#include <QObject>

#include "settings.h"

class SystemController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool appHasFocus READ appHasFocus WRITE setAppHasFocus NOTIFY appHasFocusChanged)
public:
    explicit SystemController(const std::shared_ptr<Settings> &setting, QObject *parent = nullptr);

    void setAppHasFocus(bool isActive);
    bool appHasFocus() const;

    static void saveFile(QString fileName, const QString &data);

public slots:
    QString getFileName(const QString &acceptLabel, const QString &nameFilter, const QString &selectedFile = "",
                        const bool isSaveMode = false, const QString &defaultSuffix = "");

    void setQmlRoot(QObject *qmlRoot);

signals:
    void fileDialogClosed(const bool isAccepted);
    void appHasFocusChanged();

private:
    std::shared_ptr<Settings> m_settings;

    QObject *m_qmlRoot;
    bool m_appHasFocus{false};
};

#endif // SYSTEMCONTROLLER_H
