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
    Q_PROPERTY(QString currentDriver READ currentDriver NOTIFY driverChanged)
    Q_PROPERTY(QJSValue configParameters READ configParameters NOTIFY configParametersChanged)
    Q_PROPERTY(QJSValue driverParameters READ driverParameters NOTIFY driverParametersChanged)
    Q_PROPERTY(QJSValue deviceParameters READ deviceParameters NOTIFY deviceParametersChanged)
    Q_PROPERTY(QObject* connectionPort READ connectionPort CONSTANT) // Pointer is constant although value may change
    Q_PROPERTY(QObject* tcpTaggingPort READ tcpTaggingPort CONSTANT) // Pointer is constant although value may change
    Q_PROPERTY(QObject* sampleCountPerBlock READ getSampleCountPerBlock CONSTANT)
    // UG fork: the drift correction settings
    Q_PROPERTY(QObject* driftCorrectionPolicy READ driftCorrectionPolicy CONSTANT)
    Q_PROPERTY(QObject* driftTolerance READ driftTolerance CONSTANT)
    Q_PROPERTY(QQmlListProperty<ImpedanceItem> impedances READ impedances NOTIFY impedancesChanged)
    Q_PROPERTY(double driftLimit READ driftLimit CONSTANT)
    Q_PROPERTY(double instantDrift READ instantDrift NOTIFY driftChanged)
    Q_PROPERTY(double globalDrift READ globalDrift NOTIFY driftChanged)
    Q_PROPERTY(QStringList channelNames READ getChannelNames NOTIFY channelNamesChanged)
    Q_PROPERTY(QStringList channelHints READ getChannelHints NOTIFY channelNamesChanged)  // UG fork
    // UG fork: what the driver says about its settings (e.g. what its montage did, why Play failed) and the
    // channel names it decides itself; upstream shows neither
    Q_PROPERTY(QString driverNote READ driverNote NOTIFY driverStateChanged)
    /// "info", "warning" or "error"
    Q_PROPERTY(QString driverNoteLevel READ driverNoteLevel NOTIFY driverStateChanged)
    /// One flag per channel name, true for an entry the driver decides; empty when the user decides them all
    Q_PROPERTY(QVariantList channelLocks READ channelLocks NOTIFY driverStateChanged)

public:
    AcquisitionController();
    ~AcquisitionController();

    bool isAcquiring();
    bool isCheckingImpedance();
    bool supportsImpedanceCheck();
    QStringList getDrivers();
    QString currentDriver() const;
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
    QObject* driftCorrectionPolicy() const;
    QObject* driftTolerance() const;

    Q_INVOKABLE void setCurrentDriver(const QString& driverName);

    /// Saves the driver, its settings, the chunk size, the ports and the writers, so that they are
    /// restored on the next run. Connected to the application's aboutToQuit signal.
    void saveSession();
    Q_INVOKABLE bool connectDevice();
    Q_INVOKABLE bool disconnectDevice();
    Q_INVOKABLE bool startAcquisition();
    Q_INVOKABLE bool stopAcquisition();
    Q_INVOKABLE bool toggleImpedanceCheck();
    Q_INVOKABLE void configureDeviceDriver();
    Q_INVOKABLE QStringList getChannelNames();
    Q_INVOKABLE QStringList getChannelHints();
    /**
     * @brief Hands the montage the Charmeleon this server started saved to the driver, once it has closed.
     *
     * UG fork: called when that Charmeleon ends, and before an acquisition starts; does nothing when there is
     * no such Charmeleon, or it saved nothing.
     */
    void takeOverCharmeleonMontage();
    QString driverNote() const;
    QString driverNoteLevel() const;
    QVariantList channelLocks() const;
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
    /// The driver's note or channel locks changed
    void driverStateChanged();

protected:
    /// Path of Charmeleon.exe when the current driver hands impedance checking to it and it is installed, else empty.
    std::string charmeleonExecutable();

    struct AcquisitionControllerPrivate *d = nullptr;
};

}  // namespace AcquisitionServer
}  // namespace OpenViBE
