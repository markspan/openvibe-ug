#include "CTopographicMap2DView.hpp"

#include <QQmlContext>
#include <QQuickItem>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QColor>
#include <QVector>
#include <cmath>
#include <array>

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {


//static double Deg2Rad(const double x) { return x * 0.01745329251994329576923690768489; }

CTopographicMap2DView::CTopographicMap2DView(QQmlEngine* engine, CTopographicMapDatabase& mapDatabase, const EInterpolationType interpolation, double delay)
    : m_jsEngine(engine), m_mapDatabase(mapDatabase), m_currentInterpolation(interpolation)
{
    m_sampleCoordinatesMatrix.setDimensionCount(2);

    m_palette.clear();
    m_palette.append(QColor(255, 0, 0));
    m_palette.append(QColor(234, 1, 0));
    m_palette.append(QColor(205, 0, 101));
    m_palette.append(QColor(153, 0, 178));
    m_palette.append(QColor(115, 1, 177));
    m_palette.append(QColor(77, 0, 178));
    m_palette.append(QColor(0, 0, 152));
    m_palette.append(QColor(0, 97, 121));
    m_palette.append(QColor(0, 164, 100));
    m_palette.append(QColor(0, 225, 25));
    m_palette.append(QColor(150, 255, 0));
    m_palette.append(QColor(200, 255, 0));
    m_palette.append(QColor(255, 255, 0));

  
    m_potentialProvider = new ImagePotentialProvider();
    //ownership of imageProvider is passed to QML engine so no need to delete it!
    m_jsEngine->addImageProvider(m_providerID, m_potentialProvider);

  QQmlContext *context = new QQmlContext(m_jsEngine->rootContext());

  context->setContextProperty("dataSource", this);
#if defined TARGET_OS_Windows
  context->setContextProperty("TARGET_OS_Windows", QVariant(true));
#else
  context->setContextProperty("TARGET_OS_Windows", QVariant(false));
#endif

    QQmlComponent component(m_jsEngine, QUrl("qrc:/qml/CTopographicMap2DView.qml"));

    QObject *object = component.create(context);
    m_visualItem = qobject_cast<QQuickItem *>(object);
    if (m_visualItem) {
        context->setParent(m_visualItem);
    }
}

CTopographicMap2DView::~CTopographicMap2DView()
{
    if (m_jsEngine) {
        m_jsEngine->removeImageProvider(m_providerID);
        m_potentialProvider = nullptr;
    }
    if (m_visualItem) {
        m_visualItem->deleteLater();
        m_visualItem = nullptr;
    }
    m_jsEngine = nullptr;
}

void CTopographicMap2DView::Init()
{
    // Initialize the view
    m_currentProjection = EProjection::Radial;
    m_currentView = EView::Top;
    m_mapDatabase.SetInterpolationType(m_currentInterpolation);
    m_electrodesToggledOn = false;
    emit clearCanvas();
}

void CTopographicMap2DView::Redraw()
{
    emit launchQmlRedraw();
}


CMatrix* CTopographicMap2DView::GetSampleCoordinatesMatrix()
{
    return &m_sampleCoordinatesMatrix;
}

bool CTopographicMap2DView::SetSampleValuesMatrix(CMatrix* matrix)
{
    if (matrix == nullptr || matrix->getBufferElementCount() < m_sampleValues.size()) { return false; }

    double minPotential, maxPotential;
    m_mapDatabase.GetLastBufferInterpolatedMinMaxValue(minPotential, maxPotential);

    const size_t colorStartIdx = 0;
    const size_t colorEndIdx = m_palette.length() - 1;

    double invPotentialStep = 0;

    if (minPotential < maxPotential) { invPotentialStep = (colorEndIdx - colorStartIdx + 1) / (maxPotential - minPotential); }

    //determine color index of each sample
    for (size_t i = 0; i < m_sampleValues.size(); ++i) {
        const double value = *(matrix->getBuffer() + i);
        size_t colorIdx;

        if (value < minPotential) { colorIdx = 0; }
        else if (value > maxPotential) { colorIdx = colorEndIdx; }
        else {
            colorIdx = colorStartIdx + int((value - minPotential) * invPotentialStep);
            if (colorIdx > colorEndIdx) { colorIdx = colorEndIdx; }
        }
        m_sampleValues[i] = colorIdx;
    }

    //refresh potentials
    size_t w, h;
    size_t min_x=10000, min_y=100000, max_x=0, max_y=0;
    for (size_t i = 0; i < m_sampleValues.size(); ++i){
        min_x = std::min(min_x, m_sample2DCoordinates[i].first);
        min_y = std::min(min_y, m_sample2DCoordinates[i].second);
        max_x = std::max(max_x, m_sample2DCoordinates[i].first);
        max_y = std::max(max_y, m_sample2DCoordinates[i].second);
    } 

    for (size_t i = 0; i < m_sampleValues.size(); ++i){
        //cells of last row and last column may be smaller than other ones
        if (m_sample2DCoordinates[i].first + m_cellSize >= m_skullDiameter) { w = m_skullDiameter - m_sample2DCoordinates[i].first; }
        else { w = m_cellSize; }

        if (m_sample2DCoordinates[i].second + m_cellSize >= m_skullDiameter) { h = m_skullDiameter - m_sample2DCoordinates[i].second; }
        else { h = m_cellSize; }

        size_t colorIndex = m_sampleValues[i];
        if (colorIndex > colorEndIdx) { colorIndex = colorEndIdx; }

        for(uint j = 0; j < h; ++j){
            for(uint k = 0; k < w; ++k){
                m_potentialProvider->m_img.setPixelColor(
                    m_sample2DCoordinates[i].first + k,
                    m_sample2DCoordinates[i].second + j,
                    m_palette[colorIndex]);
            }
        }
    }

    emit refreshPotentials();
    return true;
}

QList<QColor> CTopographicMap2DView::palette()
{
    return m_palette;
}

QString CTopographicMap2DView::currentView()
{
    if (m_currentView == EView::Top) { return "Top"; }
    else if (m_currentView == EView::Left) { return "Left"; }
    else if (m_currentView == EView::Right) { return "Right"; }
    else if (m_currentView == EView::Back) { return "Back"; }
    else { return "NumView"; }
}

void CTopographicMap2DView::setCurrentView(const QString& view)
{
    if (view == "Top" && m_currentView != EView::Top ) {
        m_currentView = EView::Top;
        emit currentViewChanged();
        emit clearCanvas();
    }
    else if (view == "Left" && m_currentView != EView::Left) {
        m_currentView = EView::Left;
        emit currentViewChanged();
        emit clearCanvas();
    }
    else if (view == "Right" && m_currentView != EView::Right) {
        m_currentView = EView::Right;
        emit currentViewChanged();
        emit clearCanvas();
    }
    else if (view == "Back" && m_currentView != EView::Back) {
        m_currentView = EView::Back;
        emit currentViewChanged();
        emit clearCanvas();
    }
}


void CTopographicMap2DView::SetProjectionCB(int projection)
{
    m_currentProjection = static_cast<EProjection>(projection);
    emit clearCanvas();
}

QString CTopographicMap2DView::projection()
{
    if (m_currentProjection == EProjection::Axial) { return "Axial"; }
    else if (m_currentProjection == EProjection::Radial) { return "Radial"; }
    else { return "NumProjection"; }
}

void CTopographicMap2DView::setProjection(const QString& projection)
{
    if (projection == "Axial" && m_currentProjection != EProjection::Axial) {
        m_currentProjection = EProjection::Axial;
        emit projectionChanged();
        emit clearCanvas();
    } else if (projection == "Radial" && m_currentProjection != EProjection::Radial) {
        m_currentProjection = EProjection::Radial;
        emit projectionChanged();
        emit clearCanvas();
    } else if (m_currentProjection != EProjection::NumProjection) { 
        m_currentProjection = EProjection::NumProjection;
        emit projectionChanged();
        emit clearCanvas();
    }
}

// 1 spline , 2 laplacian
void CTopographicMap2DView::SetInterpolationCB(int interpolation)
{
    m_currentInterpolation = static_cast<EInterpolationType>(interpolation);
    m_mapDatabase.SetInterpolationType(m_currentInterpolation);
    emit clearCanvas();
}

void CTopographicMap2DView::resizeData()
{
    m_potentialProvider->m_img = QImage(m_skullDiameter, m_skullDiameter, QImage::Format_RGB32);

    //determine size of colored cells
    const size_t cellMinSize            = 6;
    const size_t cellMaxSize            = 6;
    const double cellOverSkullSizeRatio = 0.02;
    m_cellSize                          = size_t(double(m_skullDiameter) * cellOverSkullSizeRatio);

    if (m_cellSize < cellMinSize) { m_cellSize = cellMinSize; }
    else if (m_cellSize > cellMaxSize) { m_cellSize = cellMaxSize; }
    if (m_cellSize == 0) { return; }

    //number of samples in a row or column
    m_gridSize = size_t(ceil(double(m_skullDiameter) / double(m_cellSize)));

    //determine number of samples lying within skull
    const size_t nSamples = computeSamplesNormalizedCoordinates(false);

    //resize sample grids accordingly
    m_sample2DCoordinates.resize(nSamples);
    m_sampleCoordinatesMatrix.resize(nSamples, 3);
    m_sampleValues.resize(nSamples);

    //compute samples normalized coordinates
    computeSamplesNormalizedCoordinates(true);
}

int CTopographicMap2DView::nbChannels() const
{
    return m_mapDatabase.GetChannelCount();
}

QString CTopographicMap2DView::channelLabel(const size_t index) const
{
    CString temp;
    m_mapDatabase.GetChannelLabel(index, temp);
    return QString::fromStdString(temp.toASCIIString());
}

QPoint CTopographicMap2DView::channel2DPosition(const size_t index, const int skullX, const int skullY) const
{
    const double skullRadius = double(m_skullDiameter) / 2.0;
    const int skullCenterX = skullX + int(skullRadius);
    const int skullCenterY = skullY + int(skullRadius);
    QPoint res(10000, 10000);

    double* originaPosition;
    m_mapDatabase.GetChannelPosition(index, originaPosition);

    //const std::array<double, 3> position = { originaPosition[0], originaPosition[1], originaPosition[2] };
    const std::array<double, 3> position = { -originaPosition[1], originaPosition[0], originaPosition[2] };

    if (m_currentView == EView::Top) {
        if (m_currentProjection == EProjection::Axial) {
            res.rx() = int(double(skullCenterX) + position[0] * skullRadius);
            res.ry() = int(double(skullCenterY) - position[1] * skullRadius);
        } else {
            const double theta = getThetaFromCartesianCoordinates(position);
            const double phi = getPhiFromCartesianCoordinates(position);
            return compute2DCoordinates(theta, phi, skullCenterX, skullCenterY);
        }
    } else if (m_currentView == EView::Back) {
        if (position[1] > sin(1.0 / 90.0 * M_PI / 2.0)) { return res; }

        if (m_currentProjection == EProjection::Axial) {
            res.rx() = int(double(skullCenterX) + position[0] * skullRadius);
            res.ry() = int(double(skullCenterY) - position[2] * skullRadius);
        } else {
            const std::array<double, 3> backPosition = { position[0], position[2], -position[1] };
            const double theta = getThetaFromCartesianCoordinates(backPosition);
            const double phi = getPhiFromCartesianCoordinates(backPosition);
            return compute2DCoordinates(theta, phi, skullCenterX, skullCenterY);
        }
    } else if (m_currentView == EView::Left) {
        //if(electrodePosition[0] > 0) //electrode not visible
        if (position[0] > cos(89.0 / 90.0 * M_PI / 2.0)) { return res; }

        if (m_currentProjection == EProjection::Axial) {
            res.rx() = int(double(skullCenterX) - position[1] * skullRadius);
            res.ry() = int(double(skullCenterY) - position[2] * skullRadius);
        } else {
            const std::array<double, 3> backPosition = { -position[1], position[2], -position[0] };
            const double theta = getThetaFromCartesianCoordinates(backPosition);
            const double phi = getPhiFromCartesianCoordinates(backPosition);
            return compute2DCoordinates(theta, phi, skullCenterX, skullCenterY);
        }
    } else if (m_currentView == EView::Right) {
        //if(electrodePosition[0] < 0) //electrode not visible
        if (position[0] < -cos(89.0 / 90.0 * M_PI / 2.0)) { return res; }

        if (m_currentProjection == EProjection::Axial) {
            res.rx() = int(double(skullCenterX) + position[1] * skullRadius);
            res.ry() = int(double(skullCenterY) - position[2] * skullRadius);
        } else {
            const std::array<double, 3> backPosition = { position[1], position[2], position[0] };
            const double theta = getThetaFromCartesianCoordinates(backPosition);
            const double phi = getPhiFromCartesianCoordinates(backPosition);
            return compute2DCoordinates(theta, phi, skullCenterX, skullCenterY);
        }
    }

    return res;
}

size_t CTopographicMap2DView::computeSamplesNormalizedCoordinates(bool all)
{
    size_t curSample = 0;

    //sampling over skull area
    const double skullRadius  = double(m_skullDiameter) / 2.0;
    const double skullCenterX = double(m_skullX) + skullRadius;
    const double skullCenterY = double(m_skullY) + skullRadius;
    double* buffer = m_sampleCoordinatesMatrix.getBuffer();

    //for each row
    double curY = double(m_skullY);
    for (size_t i = 0; i < m_gridSize; ++i) {
        //for each column
        double curX = double(m_skullX);
        for (size_t j = 0; j < m_gridSize; ++j) {
            //find corner closest to skull center
            const double closestX = std::fabs(curX - skullCenterX) < std::fabs(curX + double(m_cellSize) - skullCenterX) ? curX : (curX + double(m_cellSize));
            const double closestY = std::fabs(curY - skullCenterY) < std::fabs(curY + double(m_cellSize) - skullCenterY) ? curY : (curY + double(m_cellSize));

            //make sure electrode is in the non clipped area of the display
            //TODO : perform this test once per view only!
            //ensure closest corner lies within "skull sphere"
            if ((closestX - skullCenterX) * (closestX - skullCenterX) + (closestY - skullCenterY) * (closestY - skullCenterY) <= (skullRadius * skullRadius)) {
                if (all) {
                    m_sample2DCoordinates[curSample].first = j * m_cellSize;
                    m_sample2DCoordinates[curSample].second = i * m_cellSize;

                        //compute normalized coordinates to be fed to spherical spline algorithm
                        //----------------------------------------------------------------------
                        const size_t baseIndex = 3 * curSample;

                    const double x = (closestX - skullCenterX) / skullRadius;
                    const double y = -(closestY - skullCenterY) / skullRadius;

                    if (m_currentProjection == EProjection::Axial) {
                        if (m_currentView == EView::Top) {
                            *(buffer + baseIndex) = x;
                            *(buffer + baseIndex + 1) = y;
                            const double squareXYSum = x * x + y * y;
                            *(buffer + baseIndex + 2) = (squareXYSum >= 1) ? 0 : sqrt(1 - squareXYSum);
                        } else if (m_currentView == EView::Back) {
                            *(buffer + baseIndex) = x;
                            *(buffer + baseIndex + 2) = y;
                            const double squareXYSum = x * x + y * y;
                            *(buffer + baseIndex + 1) = (squareXYSum >= 1) ? 0 : sqrt(1 - squareXYSum);
                        } else if (m_currentView == EView::Left) {
                            *(buffer + baseIndex + 1) = -x;
                            *(buffer + baseIndex + 2) = y;
                            const double squareXYSum = x * x + y * y;
                            *(buffer + baseIndex) = (squareXYSum >= 1) ? 0 : sqrt(1 - squareXYSum);
                        } else if (m_currentView == EView::Right) {
                            *(buffer + baseIndex + 1) = x;
                            *(buffer + baseIndex + 2) = y;
                            const double squareXYSum = x * x + y * y;
                            *(buffer + baseIndex) = (squareXYSum >= 1) ? 0 : sqrt(1 - squareXYSum);
                        }
                    } else {
                        const double theta = double(M_PI / 2 * sqrt(x * x + y * y));
                        const double scalingFactor = (theta <= 1e-3) ? 0 : (sin(theta) / theta);
                        const std::array<double, 3> sampleLocalCoordinates = {
                            scalingFactor * x * (M_PI / 2.0), scalingFactor * y * (M_PI / 2.0), cos(theta)
                        };

                        if (m_currentView == EView::Top) {
                            *(buffer + baseIndex) = sampleLocalCoordinates[0];
                            *(buffer + baseIndex + 1) = sampleLocalCoordinates[1];
                            *(buffer + baseIndex + 2) = sampleLocalCoordinates[2];
                        } else if (m_currentView == EView::Back) {
                            *(buffer + baseIndex) = sampleLocalCoordinates[0];
                            *(buffer + baseIndex + 1) = -sampleLocalCoordinates[2];
                            *(buffer + baseIndex + 2) = sampleLocalCoordinates[1];
                        } else if (m_currentView == EView::Left) {
                            *(buffer + baseIndex) = -sampleLocalCoordinates[2];
                            *(buffer + baseIndex + 1) = -sampleLocalCoordinates[0];
                            *(buffer + baseIndex + 2) = sampleLocalCoordinates[1];
                        } else if (m_currentView == EView::Right) {
                            *(buffer + baseIndex) = sampleLocalCoordinates[2];
                            *(buffer + baseIndex + 1) = sampleLocalCoordinates[0];
                            *(buffer + baseIndex + 2) = sampleLocalCoordinates[1];
                        }
                    }
                }

                curSample++;
            }
            curX += double(m_cellSize);
        }
        curY += double(m_cellSize);
    }

    return curSample;
}


double CTopographicMap2DView::getThetaFromCartesianCoordinates(const std::array<double, 3>& cartesian) const { return acos(cartesian[2]); }

double CTopographicMap2DView::getPhiFromCartesianCoordinates(const std::array<double, 3>& cartesian) const
{
    double phi;
    if (cartesian[0] > 0.001) {
        phi = atan(cartesian[1] / cartesian[0]);
        if (phi < 0) { phi += 2 * M_PI; }
    } else if (cartesian[0] < -0.001) { phi = atan(cartesian[1] / cartesian[0]) + M_PI; }
    else { phi = cartesian[1] > 0 ? (M_PI / 2) : (3 * M_PI / 2); }

    return phi;
}

QPoint CTopographicMap2DView::compute2DCoordinates(double theta, double phi, int skullCenterX, int skullCenterY) const
{
    const double length = theta / (M_PI / 2.0) * double(m_skullDiameter) / 2.0;
    const double x1 = cos(phi );
    const double y1 = sin(phi);
    int x = int(double(skullCenterX) + length * x1);
    int y = int(double(skullCenterY) - length * y1);
    return QPoint(x,y);
}

}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
