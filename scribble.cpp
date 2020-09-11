#include "scribble.hpp"

#include <cmath>
#include <QPainter>

QVector<QPoint> Scribble::contourPoints() const
{
    if (!m_cacheIsValid) {
        m_cachedContourPoints.clear();

        if (!m_image.isNull()) {
            for (int y = 1; y < m_image.height() - 1; ++y) {
                const quint8 *pixel = m_image.scanLine(y);
                for (int x = 0; x < m_image.width() - 1; ++x, ++pixel) {
                    if (*pixel == 0) {
                        continue;
                    }

                    if (*(pixel - m_image.bytesPerLine() - 1) == 0 ||
                        *(pixel - m_image.bytesPerLine() - 0) == 0 ||
                        *(pixel - m_image.bytesPerLine() + 1) == 0 ||
                        *(pixel - 1) == 0 ||
                        *(pixel + 1) == 0 ||
                        *(pixel + m_image.bytesPerLine() - 1) == 0 ||
                        *(pixel + m_image.bytesPerLine() - 0) == 0 ||
                        *(pixel + m_image.bytesPerLine() + 1) == 0) {
                        m_cachedContourPoints.push_back(QPoint(x + m_imageRect.x(), y + m_imageRect.y()));
                    }
                } 
            }
        }

        m_cacheIsValid = true;
    }
    return m_cachedContourPoints;
}

bool Scribble::containsPoint(const QPoint & point) const
{
    if (m_image.isNull() || !m_imageRect.contains(point)) {
        return false;
    }

    const int x = point.x() - m_imageRect.x();
    const int y = point.y() - m_imageRect.y();
    
    return *(m_image.constBits() + y * m_image.bytesPerLine() + x) == 255;
}

void Scribble::moveTo(const QPoint & point, int radius)
{
    m_position = point;
    m_radius = radius;
}

void Scribble::lineTo(const QPoint & point, int radius)
{                       
    resizeImageToContain(point, radius);

    constexpr double spacing = 1.0;
    const double dx = point.x() - m_position.x();
    const double dy = point.y() - m_position.y();
    const double dradius = radius - m_radius;
    const double dist = std::sqrt(dx * dx + dy * dy);
    const double inc = spacing / dist;
    const double incx = dx * inc;
    const double incy = dy * inc;
    const double incradius = dradius * inc;

    QPainter painter(&m_image);
    double x = m_position.x();
    double y = m_position.y();
    double r = m_radius;
    double t = 0.0;

    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::white);
    
    while (t < 1.0) {
        int ri = static_cast<int>(std::round(r));
        painter.drawEllipse(
            static_cast<int>(std::round(x)) - m_imageRect.x() - ri,
            static_cast<int>(std::round(y)) - m_imageRect.y() - ri,
            ri * 2 + 1,
            ri * 2 + 1
        );

        t += inc;
        x += incx;
        y += incy;
        r += incradius;
    }

    painter.drawEllipse(
        point.x() - m_imageRect.x() - radius,
        point.y() - m_imageRect.y() - radius,
        radius * 2 + 1,
        radius * 2 + 1
    );

    moveTo(point, radius);

    m_cacheIsValid = false;
}

const QRect& Scribble::rect() const
{
    return m_imageRect;
}

const QImage& Scribble::image() const
{
    return m_image;
}

void Scribble::resizeImageToContain(const QPoint & point, int radius)
{
    QRect startPointRect(m_position.x() - m_radius,
                         m_position.y() - m_radius,
                         m_radius * 2 + 1,
                         m_radius * 2 + 1);
    QRect endPointRect(point.x() - radius,
                       point.y() - radius,
                       radius * 2 + 1,
                       radius * 2 + 1);
    QRect rect = startPointRect.united(endPointRect).adjusted(-1, -1, 1, 1).united(m_imageRect);

    QImage img(rect.width(), rect.height(), QImage::Format_Grayscale8);
    img.fill(0);
    QPainter painter(&img);
    painter.drawImage(m_imageRect.topLeft() - rect.topLeft(), m_image);

    m_image = img;
    m_imageRect = rect;
}

BKColorizerScribble::BKColorizerScribble(const Scribble &scribble,
                                         BKColorizerScribble::LabelIdType labelId)
    : m_scribble(scribble)
    , m_labelId(labelId)
{}

QVector<QPoint> BKColorizerScribble::contourPoints() const
{
    return m_scribble.contourPoints();
}

bool BKColorizerScribble::containsPoint(const QPoint & point) const
{
    return m_scribble.containsPoint(point);
}

const QRect& BKColorizerScribble::rect() const
{
    return m_scribble.rect();
}

BKColorizerScribble::LabelIdType BKColorizerScribble::labelId() const
{
    return m_labelId;
}
