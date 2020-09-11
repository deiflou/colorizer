#ifndef SCRIBBLE_HPP
#define SCRIBBLE_HPP

#include <BKColorizer.hpp>

#include <QImage>
#include <QVector>
#include <QPoint>
#include <QRect>

// Quick and dirty scribble implementation
class Scribble
{
public:
    QVector<QPoint> contourPoints() const;
    bool containsPoint(const QPoint & point) const;
    const QRect& rect() const;

    void moveTo(const QPoint & point, int radius = 0);
    void lineTo(const QPoint & point, int radius);

    const QImage& image() const;

private:
    QImage m_image;
    QPoint m_position;
    int m_radius;
    QRect m_imageRect;
    mutable QVector<QPoint> m_cachedContourPoints;
    mutable bool m_cacheIsValid{false};

    void resizeImageToContain(const QPoint & point, int radius);
};

// Proxy scribble class to use with the colorizer
class BKColorizerScribble
{
public:
    using LabelIdType = typename Colorizer::BKColorizer<BKColorizerScribble>::LabelIdType;

    BKColorizerScribble(const BKColorizerScribble &) = default;
    BKColorizerScribble(BKColorizerScribble &&) = default;
    BKColorizerScribble& operator=(const BKColorizerScribble &) = default;
    BKColorizerScribble& operator=(BKColorizerScribble &&) = default;
    BKColorizerScribble(const Scribble &scribble,
                        LabelIdType labelId = Colorizer::BKColorizer<BKColorizerScribble>::LabelId_Undefined);

    QVector<QPoint> contourPoints() const;
    bool containsPoint(const QPoint & point) const;
    const QRect& rect() const;
    LabelIdType labelId() const;

private:
    Scribble m_scribble;
    LabelIdType m_labelId;
};

#endif
