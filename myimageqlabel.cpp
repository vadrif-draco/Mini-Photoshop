
#include "myimageqlabel.h"

double MyImageQLabel::getScaleFactor() {
    return scaleFactor;
}

void MyImageQLabel::setScaleFactor(double s) {
    scaleFactor = s;
}

void MyImageQLabel::scaleImagePixmap() {
    setFixedSize(loadedImage.size() * scaleFactor); // Scale the QLabel
    setPixmap(QPixmap::fromImage(loadedImage).scaled(size())); // And its image pixmap
    drawSelectionArea(); // Then finally re-draw the selection area with this new scale factor considered
}

void MyImageQLabel::setSelectionMode(Selection s) {
    selectionMode = s;
    switch (selectionMode) {
        case Selection::None: printf("Selection Mode: None\n"); break;
        case Selection::Rectangular: printf("Selection Mode: Rectangular\n"); break;
        case Selection::Elliptical: printf("Selection Mode: Elliptical\n"); break;
        case Selection::Lasso: printf("Selection Mode: Lasso\n"); break;
        default: printf("Selection Mode: ??????\n"); break;
    }
    clearSelectionArea(); // Changing selection mode clears the current selection
    fflush(stdout);
};

void MyImageQLabel::clearSelectionArea() {
    noSelection = true; // Raise the no-selection flag
    setPixmap(QPixmap::fromImage(loadedImage).scaled(size())); // Set pixmap to image scaled to current widget size
}

QPixmap MyImageQLabel::getSelectionPixmap() {
    QPixmap originalImagePixmap = QPixmap::fromImage(loadedImage);
    if (noSelection) return originalImagePixmap;
    // Notice: Pixmap::copy needs a normalized QRect, unlike QPainter::drawRect which can handle a non-normalized QRect
    // And here's something funnier! QRect::normalized does NOT fix it :) That's why I had to fix it manually later :))
    return originalImagePixmap.copy(QRect(selectionTopLeft, selectionBottRight));
}

void MyImageQLabel::replaceSelectionPixmap(QPixmap pixmap) {
    if (noSelection) {
        loadedImage = pixmap.toImage(); // Simply replace with input pixmap directly
        scaleImagePixmap(); // Then scale it according to current scale factor
        return;
    }
    QPixmap originalImagePixmap(QPixmap::fromImage(loadedImage)); // Get a pixmap of the original image
    QPainter painter(&originalImagePixmap); // Create a painter to paint the provided pixmap on top of that
    // Now we tell the painter where exactly to paint
    QPainterPath lassoPath;
    switch (selectionMode) {
        case Selection::Elliptical:
            painter.setClipRegion(QRegion(QRect(selectionTopLeft, selectionBottRight), QRegion::RegionType::Ellipse));
            break;
        case Selection::Lasso:
            lassoPath.addPolygon(QPolygon(lassoPoints));
            painter.setClipPath(lassoPath);
            break;
        case Selection::None:
        case Selection::Rectangular:
        default:
            break;
    }
    // Then it shall paint starting from the selection top left corner
    painter.drawPixmap(QPoint(selectionTopLeft), pixmap);
    painter.end();
    // Set the pixmap to this painted one now...
    setPixmap(originalImagePixmap);
    // And finally, replace the image with this painted pixmap's image, un-scaled
    loadedImage = originalImagePixmap.toImage().scaled(loadedImage.size());
    // Then we re-draw the selection area to show on the GUI that the selection area is retained
    drawSelectionArea();
}

void MyImageQLabel::mousePressEvent(QMouseEvent* event) {
    if (selectionMode != Selection::None) {
        QPoint point(getPixelCoordinatesFromOriginalImageAt(event->pos()));
        selecting = true;
        noSelection = false;
        lassoPoints.clear();
        if (selectionMode == Selection::Lasso) lassoPoints.append(point);
        else selectionStart = point;
    }
}

void MyImageQLabel::mouseMoveEvent(QMouseEvent* event) {
    QPoint point(getPixelCoordinatesFromOriginalImageAt(event->pos()));
    displayPixelInfoAtPoint(point);
    // if (scaleFactor > 16) highlightPixelAtPoint(point);
    if (selecting) updateSelectionArea(point);
};

void MyImageQLabel::mouseReleaseEvent(QMouseEvent* event) {
    if (event) selecting = false;
}

// Reverses the scaling performed on the provided point and clips it within image boundaries
QPoint MyImageQLabel::getPixelCoordinatesFromOriginalImageAt(QPoint point) {
    int x = floor(float(point.x()) / scaleFactor);
    int y = floor(float(point.y()) / scaleFactor);
    int maxWidth = round(float(width()) / scaleFactor) - 1;
    int maxHeight = round(float(height()) / scaleFactor) - 1;
    if (x < 0) x = 0; else if (x > maxWidth) x = maxWidth;
    if (y < 0) y = 0; else if (y > maxHeight) y = maxHeight;
    return QPoint(x, y);
}

void MyImageQLabel::displayPixelInfoAtPoint(QPoint point) {
    QColor colors = loadedImage.pixel(point);
    emit pixelTextInfo(
        QString("(%1, %2): %3 %4 %5 ")
        .arg(point.x()).arg(point.y()).arg(colors.red()).arg(colors.green()).arg(colors.blue())
    );
}

void MyImageQLabel::highlightPixelAtPoint(QPoint point) {
    // If we multiply by the scaleFactor, we'll get the topleft corner of the pixel in the scaled pixmap
    point.setX(point.x() * scaleFactor);
    point.setY(point.y() * scaleFactor);
    // Draw a rectangle from this topleft corner with edge length = scaleFactor - 1
    // The - 1 is to conpensate for the width of the border
    QPixmap imagePixmap = pixmap();
    QPainter painter(&imagePixmap);
    painter.setPen(QColor(0, 0, 0, 63));
    painter.drawRect(QRect(point.x(), point.y(), round(scaleFactor) - 1, round(scaleFactor) - 1));
    setPixmap(imagePixmap);
}

void MyImageQLabel::updateSelectionArea(QPoint point) {
    if (noSelection || selectionEnd == point) return;
    selectionEnd = point;
    int minX = INT_MAX, minY = INT_MAX, maxX = 0, maxY = 0;
    switch (selectionMode) {
        case Selection::Rectangular:
        case Selection::Elliptical:
            selectionTopLeft = QPoint(
                fmin(selectionStart.x(), selectionEnd.x()),
                fmin(selectionStart.y(), selectionEnd.y())
            );
            selectionBottRight = QPoint(
                fmax(selectionStart.x(), selectionEnd.x()),
                fmax(selectionStart.y(), selectionEnd.y())
            );
            break;
        case Selection::Lasso:
            lassoPoints.append(selectionEnd);
            for (QPoint point : lassoPoints) {
                if (point.x() < minX) minX = point.x();
                if (point.y() < minY) minY = point.y();
                if (point.x() > maxX) maxX = point.x();
                if (point.y() > maxY) maxY = point.y();
            }
            selectionTopLeft = QPoint(minX, minY);
            selectionBottRight = QPoint(maxX, maxY);
            break;
        case Selection::None:
        default:
            break;
    }
    drawSelectionArea();
}

void MyImageQLabel::drawSelectionArea() {
    if (noSelection) return;
    QBrush fillBrush;
    fillBrush.setColor(QColor(0, 0, 255, 31));
    fillBrush.setStyle(Qt::BrushStyle::Dense1Pattern);
    QPixmap scaledImagePixmap = QPixmap::fromImage(loadedImage).scaled(size());
    QPainter painter(&scaledImagePixmap);
    painter.setBrush(fillBrush);
    painter.setPen(QPen(Qt::PenStyle::NoPen));
    QList<QPoint> scaledLassoPoints;
    switch (selectionMode) {
        case Selection::Rectangular:
            painter.drawRect(QRect(selectionTopLeft * scaleFactor, (selectionBottRight + QPoint(1, 1)) * scaleFactor));
            break;
        case Selection::Elliptical:
            painter.drawEllipse(QRect(selectionTopLeft * scaleFactor, (selectionBottRight + QPoint(1, 1)) * scaleFactor));
            break;
        case Selection::Lasso:
            for (QPoint point : lassoPoints) scaledLassoPoints.append(point * scaleFactor);
            painter.drawPolygon(&scaledLassoPoints[0], scaledLassoPoints.size());
            break;
        case Selection::None:
        default:
            break;
    }
    setPixmap(scaledImagePixmap);
}
