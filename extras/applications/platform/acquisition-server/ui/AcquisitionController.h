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

/**
 * @brief How full the buffers between the amplifier and the clients got, shown where upstream shows the drift; see
 *        Acquisition::BufferStatus.
 *
 * UG fork. "Free" and "Seconds" are the worst since the previous update, twice a second; "Min" and "Max" the worst since
 * Play. The device's values hold only when deviceReported, the server's only when acquiring.
 */
struct BufferLevels
{
    Q_GADGET
    QML_VALUE_TYPE(bufferLevels)

    Q_PROPERTY(bool acquiring MEMBER acquiring)
    Q_PROPERTY(int serverBuffers MEMBER serverBuffers)
    Q_PROPERTY(int serverFree MEMBER serverFree)
    Q_PROPERTY(int serverMinFree MEMBER serverMinFree)
    Q_PROPERTY(bool deviceReported MEMBER deviceReported)
    Q_PROPERTY(double deviceSeconds MEMBER deviceSeconds)
    Q_PROPERTY(double deviceMaxSeconds MEMBER deviceMaxSeconds)
    Q_PROPERTY(double deviceCapacity MEMBER deviceCapacity)
    Q_PROPERTY(bool lossCounted MEMBER lossCounted)
    Q_PROPERTY(double lostSamples MEMBER lostSamples)

public:
    bool acquiring          = false;  ///< An acquisition runs
    int serverBuffers       = 0;      ///< Buffers in the server's pool
    int serverFree          = 0;      ///< Fewest of them free since the previous update
    int serverMinFree       = 0;      ///< Fewest of them free since Play
    bool deviceReported     = false;  ///< The driver tells how full the amplifier's buffer is
    double deviceSeconds    = 0.0;    ///< Most data waiting in the amplifier's buffer since the previous update
    double deviceMaxSeconds = 0.0;    ///< Most data waiting in it since Play
    double deviceCapacity   = 0.0;    ///< How much it holds, in seconds; 0 when unknown
    bool lossCounted        = false;  ///< The driver counts lost samples
    double lostSamples      = 0.0;    ///< Samples the amplifier lost since Play (a double, as QML numbers are)
};

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
    Q_PROPERTY(QQmlListProperty<ImpedanceItem> impedances READ impedances NOTIFY impedancesChanged)
    /// UG fork: see BufferLevels
    Q_PROPERTY(BufferLevels buffers READ buffers NOTIFY buffersChanged)
    Q_PROPERTY(QStringList channelNames READ getChannelNames NOTIFY channelNamesChanged)
    Q_PROPERTY(QStringList channelHints READ getChannelHints NOTIFY channelNamesChanged)  // UG fork
    // UG fork: what the driver says about its settings (e.g. what its montage did, why Play failed) and the
    // channel names it decides itself; upstream shows neither
    Q_PROPERTY(QString driverNote READ driverNote NOTIFY driverStateChanged)
    // UG fork: the OpenViBE version, for the window title. See UG.md, 2.47.
    Q_PROPERTY(QString version READ version CONSTANT)
    Q_PROPERTY(NoteLevel driverNoteLevel READ driverNoteLevel NOTIFY driverStateChanged)
    /// One flag per channel name, true for an entry the driver decides; empty when the user decides them all
    Q_PROPERTY(QVariantList channelLocks READ channelLocks NOTIFY driverStateChanged)

public:
    /// UG fork: how much a driver's note matters, as AbstractDriver::Note::Level, for QML
    enum class NoteLevel { Info, Warning, Error };
    Q_ENUM(NoteLevel)

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
    BufferLevels buffers() const;

    QObject* connectionPort() const;
    QObject* tcpTaggingPort() const;

    QObject* getSampleCountPerBlock() const;

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
    /// UG fork: the version the kernel was built as (${Application_Version}), e.g. "4.0.1-ug.6"; empty without a kernel
    QString version() const;
    NoteLevel driverNoteLevel() const;
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
    void buffersChanged(void);
    void impedancesChanged(void);
    void impedanceRequested(void);
    void channelNamesChanged();
    /// The driver's note or channel locks changed
    void driverStateChanged();

protected:
    /// Path of Charmeleon.exe when the current driver hands impedance checking to it and it is installed, else empty.
    /// UG fork: upstream has no Charmeleon.
    std::string charmeleonExecutable();

    struct AcquisitionControllerPrivate *d = nullptr;
};

}  // namespace AcquisitionServer
}  // namespace OpenViBE
