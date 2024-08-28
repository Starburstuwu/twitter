#ifndef FOCUSCONTROLLER_H
#define FOCUSCONTROLLER_H

#include <QObject>
#include <QQuickItem>
#include <QQmlComponent>
#include <QQmlApplicationEngine>
#include <QPoint>


class FocusController : public QObject
{
    Q_OBJECT
public:
    explicit FocusController(QQmlApplicationEngine* engine, QObject *parent = nullptr);
    ~FocusController();

    // Q_INVOKABLE QObject* nextKeyTabItem();
    QObject* previousKeyTabItem();
    QObject* nextKeyUpItem();
    QObject* nextKeyDownItem();
    QObject* nextKeyLeftItem();
    QObject* nextKeyRightItem();
    Q_INVOKABLE QQuickItem* currentFocusedItem() const;
    // qsizetype indexOfFocusedItem(const QObject& item) const;

signals:
    void nextTabItemChanged(QObject* item);
    void previousTabItemChanged(QObject* item);
    void nextKeyUpItemChanged(QObject* item);
    void nextKeyDownItemChanged(QObject* item);
    void nextKeyLeftItemChanged(QObject* item);
    void nextKeyRightItemChanged(QObject* item);
    void focusChainChanged();

public slots:
    void reload();
    void getFocusChain();
    void nextKeyTabItem();

private:
    QList<QObject*> getSubChain(QObject* item);

    QQmlApplicationEngine* m_engine; //< Pointer to engine to get root object
    QList<QObject*> m_focus_chain; //< List of current objects to be focused
    QQuickItem* m_focused_item; //< Pointer to the active focus item
    qsizetype m_focused_item_index; //< Active focus item's index in focus chain
};

#endif // FOCUSCONTROLLER_H
