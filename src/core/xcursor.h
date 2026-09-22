#pragma once
#include <QByteArray>
#include <QImage>
#include <QList>
#include <QString>

// Native Xcursor decoder. Replaces nwg-look's dependency on the xcur2png tool.
//
// Xcursor file: "Xcur" header, a table of contents, then chunks. Image chunks
// (type 0xfffd0002) hold premultiplied ARGB pixels, little-endian.
namespace XCursor {

struct Frame {
    int nominalSize = 0;   // the size the theme labels this image with
    int xhot = 0;
    int yhot = 0;
    int delayMs = 0;       // 0 for still cursors
    QImage image;          // Format_ARGB32_Premultiplied
};

// Bounds-checked: malformed or truncated data yields an empty list, never a crash.
QList<Frame> decode(const QByteArray &data);
QList<Frame> decodeFile(const QString &path);

// The first frame of the nominal size closest to `size`. For animated cursors
// this is the first frame of the animation.
Frame bestFrame(const QList<Frame> &frames, int size);

}  // namespace XCursor
