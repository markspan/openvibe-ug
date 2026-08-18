// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

pragma Singleton

import QtQuick
import DesignSystem as DS

QtObject {
    id: root


    /*
     * Return the color associated to a stream type
     * Stream names are hardcode for now and are matching the ones in
     * sdk\kernel\src\kernel\ovkCKernelContext.cpp: m_typeManager->registerStreamType() calls.
     */
    function getStreamColor(streamType) {
        switch(streamType) {
            case "EBML stream":
                return DS.Theme.ebmlStream
            case "Experiment information":
                return DS.Theme.experimentInfoStream
            case "Stimulations":
                return DS.Theme.stimulationStream
            case "Streamed matrix":
                return DS.Theme.streamedMatrixStream
            case "Channel localisation":
                return DS.Theme.channelLocalisationStream
            case "Channel units":
                return DS.Theme.channelUnitsStream
            case "Feature vector":
                return DS.Theme.featureVectorStream
            case "Signal":
                return DS.Theme.signalStream
            case "Spectrum":
                return DS.Theme.spectrumStream
            case "Time-frequency":
                return DS.Theme.timeFrequencyStream
            case "Covariance Matrix":
                return DS.Theme.covarianceMatrixStream
            default:
                return "#000000"
        }
    }
}