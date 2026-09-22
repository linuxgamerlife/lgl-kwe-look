#include "xcursor.h"

#include <QFile>
#include <cstdlib>

namespace XCursor {

namespace {

constexpr quint32 kImageType = 0xfffd0002u;
constexpr quint32 kMaxTocEntries = 4096;
constexpr quint32 kMaxDimension = 2048;

class Reader
{
public:
    explicit Reader(const QByteArray &d) : m_d(d) {}

    bool u32(qsizetype pos, quint32 *out) const
    {
        if (pos < 0 || pos > m_d.size() - 4)
            return false;
        const auto *p = reinterpret_cast<const uchar *>(m_d.constData()) + pos;
        *out = quint32(p[0]) | (quint32(p[1]) << 8) | (quint32(p[2]) << 16) | (quint32(p[3]) << 24);
        return true;
    }

private:
    const QByteArray &m_d;
};

}  // namespace

QList<Frame> decode(const QByteArray &data)
{
    QList<Frame> frames;
    if (data.size() < 16 || !data.startsWith("Xcur"))
        return frames;

    const Reader r(data);
    quint32 ntoc = 0;
    if (!r.u32(12, &ntoc) || ntoc > kMaxTocEntries)
        return frames;

    for (quint32 i = 0; i < ntoc; ++i) {
        const qsizetype entry = 16 + qsizetype(i) * 12;
        quint32 type = 0, subtype = 0, position = 0;
        if (!r.u32(entry, &type) || !r.u32(entry + 4, &subtype) || !r.u32(entry + 8, &position))
            return frames;
        if (type != kImageType)
            continue;

        // chunk header: header size, type, subtype, version; then the image fields
        quint32 w = 0, h = 0, xhot = 0, yhot = 0, delay = 0;
        const qsizetype c = qsizetype(position);
        if (!r.u32(c + 16, &w) || !r.u32(c + 20, &h) || !r.u32(c + 24, &xhot)
            || !r.u32(c + 28, &yhot) || !r.u32(c + 32, &delay))
            continue;
        if (w == 0 || h == 0 || w > kMaxDimension || h > kMaxDimension)
            continue;

        const qsizetype pixelsAt = c + 36;
        const qsizetype pixelBytes = qsizetype(w) * qsizetype(h) * 4;
        if (pixelsAt > data.size() || pixelBytes > data.size() - pixelsAt)
            continue;

        Frame f;
        f.nominalSize = int(subtype);
        f.xhot = int(xhot);
        f.yhot = int(yhot);
        f.delayMs = int(delay);
        f.image = QImage(int(w), int(h), QImage::Format_ARGB32_Premultiplied);
        for (quint32 y = 0; y < h; ++y) {
            auto *line = reinterpret_cast<quint32 *>(f.image.scanLine(int(y)));
            for (quint32 x = 0; x < w; ++x) {
                quint32 px = 0;
                r.u32(pixelsAt + qsizetype(y * w + x) * 4, &px);
                line[x] = px;
            }
        }
        frames.append(f);
    }
    return frames;
}

QList<Frame> decodeFile(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return {};
    return decode(f.read(32 * 1024 * 1024));   // real cursors are a few hundred KiB
}

Frame bestFrame(const QList<Frame> &frames, int size)
{
    Frame best;
    int bestDist = 1 << 30;
    for (const Frame &f : frames) {
        const int dist = std::abs(f.nominalSize - size);
        // Strictly closer wins, so the first frame of an animation is kept.
        if (dist < bestDist) {
            bestDist = dist;
            best = f;
        }
    }
    return best;
}

}  // namespace XCursor
