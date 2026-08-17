#ifndef MUMBLEPROTOBUF_H
#define MUMBLEPROTOBUF_H

#include <QByteArray>
#include <QString>
#include <QVector>

class MumbleBufferWriter {
public:
    void writeVarint(quint64 val);
    void writeTag(int fieldNum, int wireType);
    void writeVarintField(int fieldNum, quint64 val);
    void writeStringField(int fieldNum, const QString &val);
    QByteArray toByteArray() const { return m_buffer; }

private:
    QByteArray m_buffer;
};

class MumbleBufferReader {
public:
    MumbleBufferReader(const QByteArray &data);
    bool hasMore() const { return m_offset < m_buffer.size(); }
    quint64 readVarint();
    QString readString(int len);
    void skip(int len);

private:
    QByteArray m_buffer;
    int m_offset = 0;
};

#endif // MUMBLEPROTOBUF_H
