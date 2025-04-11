/*
 * QDataStream implementation for safe socket operation
 * SPDX-FileCopyrightText: 2014 Martin Bříza <mbriza@redhat.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */

#ifndef SAFEDATASTREAM_H
#define SAFEDATASTREAM_H

#include <QtCore/QDataStream>
#include <QByteArray>

namespace PLASMALOGIN {
    class SafeDataStream : public QDataStream {
    public:
        SafeDataStream(QIODevice* device);
        void send();
        void receive();
        void reset();

    private:
        QByteArray m_data { };
        QIODevice *m_device { nullptr };
    };
}

#endif // SAFEDATASTREAM_H
