#pragma once

#include "../../defines.hpp"

#include <openvibe/ov_all.h>

#include <QQmlEngine>
#include <QQuickImageProvider>

#include "CTopographicMapDatabase.hpp"

#include <vector>

class QQmlEngine;
class QQuickItem;

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

class ImagePotentialProvider : public QQuickImageProvider
{
public:
    ImagePotentialProvider() : QQuickImageProvider(QQuickImageProvider::Image) {}

    QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override
    {
        Q_UNUSED(id);
        Q_UNUSED(requestedSize);
        Q_UNUSED(size);
        return m_img;
    }
    
    QImage m_img;
};


class CTopographicMap2DView final : public QObject, public CTopographicMapDrawable
{
    Q_OBJECT

    Q_PROPERTY(QString currentView READ currentView WRITE setCurrentView NOTIFY currentViewChanged)
    Q_PROPERTY(QString projection READ projection WRITE setProjection NOTIFY projectionChanged)
    Q_PROPERTY(uint skullDiameter MEMBER m_skullDiameter NOTIFY skullDiameterChanged) //TOCHECK updated from qml!!
    Q_PROPERTY(uint skullX MEMBER m_skullX NOTIFY skullPositionChanged)
    Q_PROPERTY(uint skullY MEMBER m_skullY NOTIFY skullPositionChanged)
    Q_PROPERTY(QList<QColor> palette READ palette CONSTANT)


public:
    enum class EProjection { Axial, Radial, NumProjection };
    enum class EView { Top, Left, Right, Back };

    /**
     * \brief Constructor
     * \param mapDatabase Datastore
     * \param interpolation Interpolation mode
     * \param delay Delay to apply to displayed data
     */
    CTopographicMap2DView(QQmlEngine* engine, CTopographicMapDatabase& mapDatabase, EInterpolationType interpolation, double delay);
    ~CTopographicMap2DView() override;

    //not really qml like code, that's a first version.
    //can be refactored toprovide channels as a model
    Q_INVOKABLE QPoint channel2DPosition(size_t index, const int skullX, const int skullY) const;
    Q_INVOKABLE int nbChannels() const;
    Q_INVOKABLE QString channelLabel(size_t index) const;

    Q_INVOKABLE void resizeData(); //update potentials size and co

    void Init() override;
    void Redraw() override;

    /**
     * \brief Get matrix of sample points coordinates (places where to interpolate values)
     * \return Pointer to matrix of sample points coordinates
     */
    CMatrix* GetSampleCoordinatesMatrix() override;

    /**
     * \brief Set matrix of sample points values (values interpolated at places specified in sample coordinates matrix)
     * \param [in] matrix Pointer to matrix of sample points values
     * \return True if values were successfully set, false otherwise
     */
    bool SetSampleValuesMatrix(CMatrix* matrix) override;


    /**
     * \brief Get ID of current view
     * \return ID of current view
     */
    EView GetCurrentView() const { return m_currentView; }

    QString currentView();
    void setCurrentView(const QString& view);

    QString projection();
    void setProjection(const QString& projection);

    QList<QColor> palette();

    QQuickItem* visualItem() { return m_visualItem; }

signals:
    void currentViewChanged();
    void launchQmlRedraw();
    void projectionChanged();
    void refreshPotentials();
    void skullDiameterChanged();
    void skullPositionChanged();
    void clearCanvas(); // when there is a resize, or orientation change ..

private slots:
    void SetProjectionCB(int projection);
    void SetInterpolationCB(int interpolation);
    void SetDelayCB(double delay) const { m_mapDatabase.SetDelay(delay); }

private:
    /**
     * \brief Compute normalized coordinates of 2D samples
     * \remarks This method should first be called with bComputeCoordinates = false, allowing caller
     * to resize data structures appropriately, and then it may be called with bComputeCoordinates = true
     * \param all If false, this method only computes the number of visible samples
     * \return Number of visible samples (samples lying within the actual skull area)
     */
    size_t computeSamplesNormalizedCoordinates(bool all);
    double getThetaFromCartesianCoordinates(const std::array<double, 3>& cartesian) const;
    double getPhiFromCartesianCoordinates(const std::array<double, 3>& cartesian) const;
    QPoint compute2DCoordinates(double theta, double phi, int skullCenterX, int skullCenterY) const;

    QQmlEngine* m_jsEngine = nullptr;
    QQuickItem* m_visualItem = nullptr;
    //! The database that contains the information to use to draw the signals
    CTopographicMapDatabase& m_mapDatabase;
    double m_maxDelay = 2.0;

    EProjection m_currentProjection = EProjection::Radial;
    EView m_currentView = EView::Top;
    EInterpolationType m_currentInterpolation = EInterpolationType::Laplacian;
    uint m_skullDiameter = 0;
    uint m_skullX = 0, m_skullY = 0;
    size_t m_rowStride = 0;

    //TODO removefrom here
    bool m_electrodesToggledOn = true;
    bool m_needResize = true;
    size_t m_gridSize = 0, m_cellSize = 0;

    CMatrix m_sampleCoordinatesMatrix;

    std::vector<size_t> m_sampleValues;
    std::vector<std::pair<size_t, size_t>> m_sample2DCoordinates; //in skull coords

    ImagePotentialProvider *m_potentialProvider = nullptr;
    QString m_providerID = "skullPotentials";
    QList<QColor> m_palette;
};

}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
