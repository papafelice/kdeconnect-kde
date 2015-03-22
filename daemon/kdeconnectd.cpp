/**
 * Copyright 2014 Yuri Samoilenko <kinnalru@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <unistd.h>

#include <QSocketNotifier>
#include <QApplication>

#include <KDBusService>

#include "core/daemon.h"
#include "kdeconnect-version.h"

static int sigtermfd[2];
const static char deadbeef = 1;
struct sigaction action;

void sighandler(int signum)
{
    if( signum == SIGTERM || signum == SIGINT)
    {
        ssize_t unused = ::write(sigtermfd[0], &deadbeef, sizeof(deadbeef));
        Q_UNUSED(unused);
    }
}

void initializeTermHandlers(QCoreApplication* app, Daemon* daemon)
{
    ::socketpair(AF_UNIX, SOCK_STREAM, 0, sigtermfd);
    QSocketNotifier* snTerm = new QSocketNotifier(sigtermfd[1], QSocketNotifier::Read, app);
    QObject::connect(snTerm, SIGNAL(activated(int)), daemon, SLOT(deleteLater()));

    action.sa_handler = sighandler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    sigaction(SIGTERM, &action, NULL);
    sigaction(SIGINT, &action, NULL);
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("kdeconnectd");
    app.setApplicationVersion(QLatin1String(KDECONNECT_VERSION_STRING));
    app.setOrganizationDomain("kde.org");
    app.setQuitOnLastWindowClosed(false);

    KDBusService dbusService(KDBusService::Unique);

    Daemon* daemon = new Daemon(0);
    QObject::connect(daemon, SIGNAL(destroyed(QObject*)), &app, SLOT(quit()));
    initializeTermHandlers(&app, daemon);

    return app.exec();
}