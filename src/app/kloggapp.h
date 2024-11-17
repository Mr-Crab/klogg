/*
 * Copyright (C) 2016 -- 2019 Anton Filimonov and other contributors
 *
 * This file is part of klogg.
 *
 * klogg is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * klogg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with klogg.  If not, see <http://www.gnu.org/licenses/>.
 */

 /*
 * Copyright (C) 2024 Xiaohuang Zhu
 *
 * This file is part of qlogg.
 *
 * qlogg is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * qlogg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with qlogg.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef KLOGG_KLOGGAPP_H
#define KLOGG_KLOGGAPP_H

#include <iterator>
#include <numeric>
#include <qapplication.h>
#include <stack>
#include <QApplication>
#include <kdsingleapplication.h>
#include "crashhandler.h"
#include "mainwindow.h"
#include "messagereceiver.h"
#include "versionchecker.h"

class KloggApp : public QApplication {
    Q_OBJECT

public:
    KloggApp(int& argc, char* argv[]);
    void sendFilesToPrimaryInstance(std::vector<QString> filenames);
    void initCrashHandler();
    MainWindow* reloadSession();
    void clearInactiveSessions();
    MainWindow* newWindow();
    void loadFileNonInteractive(const QString& file);
    void startBackgroundTasks();
    bool isSecondary() const {
        return !singleApplication_.isPrimaryInstance();
    }

    qint64 primaryPid() const {
        return singleApplication_.primaryPid();
    }
#ifdef Q_OS_MAC
    bool event(QEvent* event) override;
#endif

private:
    MainWindow* newWindow(WindowSession&& session);
    void onWindowActivated(MainWindow& window);
    void onWindowClosed(MainWindow& window);
    void exitApplication();
    void newVersionNotification(const QString& new_version, const QString& url, const QStringList& changes);
    size_t nextWindowIndex() const;

  private:
    KDSingleApplication singleApplication_;
    std::unique_ptr<CrashHandler> crashHandler_;
    MessageReceiver messageReceiver_;
    std::shared_ptr<Session> session_;
    std::list<std::pair<WindowSession, MainWindow*>> mainWindows_;
    std::stack<QPointer<MainWindow>> activeWindows_;

    VersionChecker versionChecker_;
};

#endif // KLOGG_KLOGGAPP_H
