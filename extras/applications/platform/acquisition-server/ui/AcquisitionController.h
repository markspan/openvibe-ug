/**
 * SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
 */
#pragma once

#include <QtCore>
#include <QtQml>

#include <QJSValue>

#include "ImpedanceItem.h"

namespace OpenViBE {
namespace AcquisitionServer {

class AcquisitionController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    
    Q_PROPERTY(bool isAcquiring READ isAcquiring NOTIFY isAcquiringChanged)
    Q_PROPERTY(bool isCheckingImpedance READ isCheckingImpedance NOTIFY isCheckingImpedanceChanged)
    Q_PROPERTY(bool supportsImpedanceCheck READ supportsImpedanceCheck NOTIFY supportsImpedanceCheckChanged)
    Q_PROPERTY(QStringList drivers READ getDrivers NOTIFY driverChanged)
    Q_PROPERTY(QJSValue configParameters READ configParameters NOTIFY configParametersChanged)
    Q_PROPERTY(QJSValue driverParameters READ driverParameters NOTIFY driverParametersChanged)
    Q_PROPERTY(QJSValue deviceParameters READ deviceParameters NOTIFY deviceParametersChanged)
    Q_PROPERTY(QObject* connectionPort READ connectionPort CONSTANT) // Pointer is constant although value may change
    Q_PROPERTY(QObject* tcpTaggingPort READ tcpTaggingPort CONSTANT) // Pointer is constant although value may change
    Q_PROPERTY(QObject* sampleCountPerBlock READ getSampleCountPerBlock CONSTANT)
    Q_PROPERTY(QQmlListProperty<ImpedanceItem> impedances READ impedances NOTIFY impedancesChanged)
    Q_PROPERTY(double driftLimit READ driftLimit CONSTANT)
    Q_PROPERTY(double instantDrift READ instantDrift NOTIFY driftChanged)
    Q_PROPERTY(double globalDrift READ globalDrift NOTIFY driftChanged)
    Q_PROPERTY(QStringList channelNames READ getChannelNames NOTIFY channelNamesChanged)

public:
    AcquisitionController();
    ~AcquisitionController();

    bool isAcquiring();
    bool isCheckingImpedance();
    bool supportsImpedanceCheck();
    QStringList getDrivers();
    QQmlListProperty<ImpedanceItem> impedances();
    QJSValue configParameters();
    QJSValue deviceParameters();
    QJSValue driverParameters();
    double instantDrift();
    double globalDrift();
    double driftLimit();

    QObject* connectionPort() const;
    QObject* tcpTaggingPort() const;

    QObject* getSampleCountPerBlock() const;

    Q_INVOKABLE void setCurrentDriver(const QString& driverName);
    Q_INVOKABLE bool connectDevice();
    Q_INVOKABLE bool disconnectDevice();
    Q_INVOKABLE bool startAcquisition();
    Q_INVOKABLE bool stopAcquisition();
    Q_INVOKABLE bool toggleImpedanceCheck();
    Q_INVOKABLE void configureDeviceDriver();
    Q_INVOKABLE QStringList getChannelNames();
    Q_INVOKABLE void setChannelNames(const QStringList& channelNames);
    Q_INVOKABLE void setChannelName(int index, const QString& name);

signals:
    void isAcquiringChanged(void);
    void isCheckingImpedanceChanged(void);
    void supportsImpedanceCheckChanged(void);
    void configParametersChanged(void);
    void deviceParametersChanged(void);
    void driverChanged();
    void driverParametersChanged(void);
    void driftChanged(void);
    void impedancesChanged(void);
    void impedanceRequested(void);
    void channelNamesChanged();

protected:
    struct AcquisitionControllerPrivate *d = nullptr;
};

}  // namespace AcquisitionServer
}  // namespace OpenViBE
