#include "MumbleProtobuf.h"

void MumbleBufferWriter::writeVarint(quint64 val) {
    while (val >= 0x80) {
        m_buffer.append(static_cast<char>((val & 0x7F) | 0x80));
        val >>= 7;
    }
    m_buffer.append(static_cast<char>(val & 0x7F));
}

void MumbleBufferWriter::writeTag(int fieldNum, int wireType) {
    writeVarint(static_cast<quint64>((fieldNum << 3) | wireType));
}

void MumbleBufferWriter::writeVarintField(int fieldNum, quint64 val) {
    writeTag(fieldNum, 0);
    writeVarint(val);
}

void MumbleBufferWriter::writeStringField(int fieldNum, const QString &val) {
    if (val.isEmpty()) return;
    QByteArray bytes = val.toUtf8();
    writeTag(fieldNum, 2);
    writeVarint(static_cast<quint64>(bytes.size()));
    m_buffer.append(bytes);
}

MumbleBufferReader::MumbleBufferReader(const QByteArray &data) : m_buffer(data), m_offset(0) {}

quint64 MumbleBufferReader::readVarint() {
    quint64 result = 0;
    int shift = 0;
    while (m_offset < m_buffer.size()) {
        quint8 b = static_cast<quint8>(m_buffer[m_offset++]);
        result |= static_cast<quint64>(b & 0x7F) << shift;
        if ((b & 0x80) == 0) break;
        shift += 7;
    }
    return result;
}

QString MumbleBufferReader::readString(int len) {
    if (m_offset + len > m_buffer.size()) {
        len = qMax(0, m_buffer.size() - m_offset);
    }
    QString str = QString::fromUtf8(m_buffer.constData() + m_offset, len);
    m_offset += len;
    return str;
}

void MumbleBufferReader::skip(int len) {
    m_offset = qMin(m_buffer.size(), m_offset + len);
}
