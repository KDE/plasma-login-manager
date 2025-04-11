/*
 * QDataStream implementation for safe socket operation
 * SPDX-FileCopyrightText: 2014 Martin Bříza <mbriza@redhat.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */

#include "SafeDataStream.h"

#include <QtCore/QDebug>
#include <QIODevice>

namespace PLASMALOGIN {
    SafeDataStream::SafeDataStream(QIODevice* device)
            : QDataStream(&m_data, QIODevice::ReadWrite)
            , m_device(device) { }

    void SafeDataStream::send() {
        qint64 length = m_data.length();
        qint64 writtenTotal = 0;
        if (!m_device->isOpen()) {
            qCritical() << " Auth: SafeDataStream: Could not write any data";
            return;
        }
        m_device->write((const char*) &length, sizeof(length));
        while (writtenTotal != length) {
            qint64 written = m_device->write(m_data.mid(writtenTotal));
            if (written < 0 || !m_device->isOpen()) {
                qCritical() << " Auth: SafeDataStream: Could not write all stored data";
                return;
            }
            writtenTotal += written;
            m_device->waitForBytesWritten(-1);
        }

        reset();
    }

    void SafeDataStream::receive() {
        qint64 length = -1;

        if (!m_device->isOpen()) {
            qCritical() << " Auth: SafeDataStream: Could not read from the device";
            return;
        }
        if (!m_device->bytesAvailable())
            m_device->waitForReadyRead(-1);
        m_device->read((char*) &length, sizeof(length));

        if (length < 0)
            return;
        reset();

        while (m_data.length() < length) {
            if (!m_device->isOpen()) {
                qCritical() << " Auth: SafeDataStream: Could not read from the device";
                return;
            }
            if (!m_device->bytesAvailable())
                m_device->waitForReadyRead(-1);
            m_data.append(m_device->read(length - m_data.length()));
        }
    }

    void SafeDataStream::reset() {
        m_data.clear();
        device()->reset();
        resetStatus();
    }
}
