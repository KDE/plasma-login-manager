/*
 *  SPDX-FileCopyrightText: 2026 Oliver Beard <olib141@outlook.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <QQuickWindow>

class GreeterEventFilter : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QQuickWindow *window READ window WRITE setWindow NOTIFY windowChanged)

public:
    explicit GreeterEventFilter(QObject *parent = nullptr);

    QQuickWindow *window() const;
    void setWindow(QQuickWindow *window);

Q_SIGNALS:
    void windowChanged();

    void keyPressed();
    void escapeKeyPressed();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    QPointer<QQuickWindow> m_window = nullptr;
};
