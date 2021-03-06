/* ============================================================
* QupZilla - WebKit based browser
* Copyright (C) 2010-2012  David Rosca <nowrep@gmail.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* ============================================================ */
#include "adblockicon.h"
#include "adblockrule.h"
#include "adblockmanager.h"
#include "mainapplication.h"
#include "qupzilla.h"
#include "webpage.h"
#include "tabbedwebview.h"
#include "tabwidget.h"
#include "desktopnotificationsfactory.h"

#include <QMenu>
#include <QTimer>

AdBlockIcon::AdBlockIcon(QupZilla* mainClass, QWidget* parent)
    : ClickableLabel(parent)
    , p_QupZilla(mainClass)
    , m_menuAction(0)
    , m_flashTimer(0)
    , m_timerTicks(0)
    , m_enabled(false)
{
    setMaximumHeight(16);
    setCursor(Qt::PointingHandCursor);
    setToolTip(tr("AdBlock lets you block unwanted content on web pages"));

    connect(this, SIGNAL(clicked(QPoint)), this, SLOT(showMenu(QPoint)));
}

void AdBlockIcon::popupBlocked(const QString &ruleString, const QUrl &url)
{
    int index = ruleString.lastIndexOf(" (");

    const QString &subscriptionName = ruleString.left(index);
    const QString &filter = ruleString.mid(index + 2, ruleString.size() - index - 3);
    AdBlockSubscription* subscription = AdBlockManager::instance()->subscriptionByName(subscriptionName);
    if (filter.isEmpty() || !subscription) {
        return;
    }

    AdBlockRule rule(filter, subscription);

    QPair<AdBlockRule, QUrl> pair;
    pair.first = rule;
    pair.second = url;
    m_blockedPopups.append(pair);

    mApp->desktopNotifications()->showNotification(QPixmap(":html/adblock_big.png"), tr("Blocked popup window"), tr("AdBlock blocked unwanted popup window."));

    if (!m_flashTimer) {
        m_flashTimer = new QTimer(this);
    }

    if (m_flashTimer->isActive()) {
        stopAnimation();
    }

    m_flashTimer->setInterval(500);
    m_flashTimer->start();

    connect(m_flashTimer, SIGNAL(timeout()), this, SLOT(animateIcon()));
}

QAction* AdBlockIcon::menuAction()
{
    if (!m_menuAction) {
        m_menuAction = new QAction(tr("AdBlock"), this);
        m_menuAction->setMenu(new QMenu);
        connect(m_menuAction->menu(), SIGNAL(aboutToShow()), this, SLOT(createMenu()));
    }

    m_menuAction->setIcon(QIcon(m_enabled ? ":icons/other/adblock.png" : ":icons/other/adblock-disabled.png"));

    return m_menuAction;
}

void AdBlockIcon::createMenu(QMenu* menu)
{
    if (!menu) {
        menu = qobject_cast<QMenu*>(sender());
        if (!menu) {
            return;
        }
    }

    menu->clear();

    AdBlockManager* manager = AdBlockManager::instance();

    menu->addAction(tr("Show AdBlock &Settings"), manager, SLOT(showDialog()));
    menu->addSeparator();
    if (!m_blockedPopups.isEmpty()) {
        menu->addAction(tr("Blocked Popup Windows"))->setEnabled(false);
        for (int i = 0; i < m_blockedPopups.count(); i++) {
            const QPair<AdBlockRule, QUrl> &pair = m_blockedPopups.at(i);

            QString address = pair.second.toString().right(55);
            QString actionText = tr("%1 with (%2)").arg(address, pair.first.filter()).replace('&', "&&");

            QAction* action = menu->addAction(actionText, manager, SLOT(showRule()));
            action->setData(qVariantFromValue((void*)&pair.first));
        }
    }

    menu->addSeparator();
    QList<WebPage::AdBlockedEntry> entries = p_QupZilla->weView()->page()->adBlockedEntries();
    if (entries.isEmpty()) {
        menu->addAction(tr("No content blocked"))->setEnabled(false);
    }
    else {
        menu->addAction(tr("Blocked URL (AdBlock Rule) - click to edit rule"))->setEnabled(false);
        foreach(const WebPage::AdBlockedEntry & entry, entries) {
            QString address = entry.url.toString().right(55);
            QString actionText = tr("%1 with (%2)").arg(address, entry.rule->filter()).replace('&', "&&");

            QAction* action = menu->addAction(actionText, manager, SLOT(showRule()));
            action->setData(qVariantFromValue((void*)entry.rule));
        }
    }
}

void AdBlockIcon::showMenu(const QPoint &pos)
{
    QMenu menu;
    createMenu(&menu);

    menu.exec(pos);
}

void AdBlockIcon::animateIcon()
{
    ++m_timerTicks;
    if (m_timerTicks > 10) {
        stopAnimation();
        return;
    }

    if (pixmap()->isNull()) {
        setPixmap(QPixmap(":icons/other/adblock.png"));
    }
    else {
        setPixmap(QPixmap());
    }
}

void AdBlockIcon::stopAnimation()
{
    m_timerTicks = 0;
    m_flashTimer->stop();
    disconnect(m_flashTimer, SIGNAL(timeout()), this, SLOT(animateIcon()));

    setEnabled(m_enabled);
}

void AdBlockIcon::setEnabled(bool enabled)
{
    if (enabled) {
        setPixmap(QPixmap(":icons/other/adblock.png"));
    }
    else {
        setPixmap(QPixmap(":icons/other/adblock-disabled.png"));
    }

    m_enabled = enabled;
}

AdBlockIcon::~AdBlockIcon()
{
}
