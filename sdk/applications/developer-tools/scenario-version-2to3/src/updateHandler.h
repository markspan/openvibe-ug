/**
 * SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/ 

#pragma once

#include <QObject>
#include <QUrl>

class UpdateHandler : public QObject {
    Q_OBJECT
public:
    explicit UpdateHandler(QObject *parent = nullptr): QObject(parent) {};

    Q_INVOKABLE void processFile(const QUrl &fileUrl, bool overwrite);
};