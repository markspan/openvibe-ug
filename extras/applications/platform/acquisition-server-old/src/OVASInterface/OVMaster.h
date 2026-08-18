#include <QtCore>
#include <QtQml>

#include <QJSValue>

#include "ImpedanceItem.h"

namespace OpenViBE {
namespace AcquisitionServer {

class OVMaster : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    
    Q_PROPERTY(QStringList drivers READ getDrivers NOTIFY driverChanged)
    Q_PROPERTY(QJSValue configParameters READ configParameters NOTIFY configParametersChanged)
    Q_PROPERTY(QJSValue driverParameters READ driverParameters NOTIFY driverParametersChanged)
    Q_PROPERTY(QJSValue deviceParameters READ deviceParameters NOTIFY deviceParametersChanged)
    Q_PROPERTY(int connectionPort READ connectionPort WRITE setConnectionPort NOTIFY connectionPortChanged)
    Q_PROPERTY(QList<int> sampleCountPerBlockList READ getSampleCountPerBlockList NOTIFY sampleCountPerBlockListChanged)
    Q_PROPERTY(QQmlListProperty<ImpedanceItem> impedances READ impedances NOTIFY impedancesChanged)
    Q_PROPERTY(double drift READ drift NOTIFY driftChanged)

public:
    OVMaster();
    ~OVMaster();

    QStringList getDrivers();
    QList<int> getSampleCountPerBlockList();
    QQmlListProperty<ImpedanceItem> impedances();
    QJSValue configParameters();
    QJSValue deviceParameters();
    QJSValue driverParameters();
    double drift();

    int connectionPort() const;
    void setConnectionPort(int port);

    Q_INVOKABLE void setCurrentDriver(const QString& driverName);
    Q_INVOKABLE void setSampleCountPerBlock(int sampleCount);
    Q_INVOKABLE int getInitSampleCountPerBlockIndex();
    Q_INVOKABLE bool connectDevice();
    Q_INVOKABLE bool disconnectDevice();
    Q_INVOKABLE bool startAcquisition();
    Q_INVOKABLE bool stopAcquisition();
    Q_INVOKABLE void configureDeviceDriver();
    Q_INVOKABLE QStringList getChannelNames();
    Q_INVOKABLE void setChannelNames(const QStringList& channelNames);

signals:
    void configParametersChanged(void);
    void deviceParametersChanged(void);
    void driverChanged();
    void driverParametersChanged(void);
    void connectionPortChanged(void);
    void sampleCountPerBlockListChanged(void);
    void driftChanged(void);
    void impedancesChanged(void);
    void impedanceRequested(void);

protected:
    struct OVMasterPrivate *d = nullptr;
};

}  // namespace AcquisitionServer
}  // namespace OpenViBE
