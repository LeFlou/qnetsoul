/*
	Copyright 2009 Dally Richard
	This file is part of QNetSoul.
	QNetSoul is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	QNetSoul is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QNetSoul.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <iostream>
#include <QByteArray>
#include "Url.h"
#include "QNetsoul.h"
#include "ContactWidget.h"
#include "ContactsReader.h"
#include "ContactsWriter.h"
#include "ConnectionPoint.h"

const char*	ONLINE = "Online";
const char*	OFFLINE = "Offline";

QNetsoul::QNetsoul(QWidget* parent)
	:	QMainWindow(parent),
		_network(new Network(this)),
		_options(new Options(this)),
		_addContact(new AddContact(this)),
		_standardItemModel(new QStandardItemModel(this)),
		_trayIcon(NULL)
{
	setupUi(this);
	setupTrayIcon();
	setupStatusButton();
	this->contactsListView->setModel(this->_standardItemModel);
	connectQNetsoulItems();
	connectActionsSignals();
	connectNetworkSignals();
	readSettings();
	loadContacts("contacts.xml");
	this->_portraitResolver.addRequest(getContactLogins());
}

QNetsoul::~QNetsoul(void)
{
}

void	QNetsoul::closeEvent(QCloseEvent* event)
{
	if (NULL == this->_trayIcon)
	{
		event->accept();
		return;
	}
	static bool	firstTime = true;
	
	if (this->_trayIcon->isVisible())
	{
		this->_oldPos = this->pos();
		hide();
		if (true == firstTime)
		{
			firstTime = false;
			this->_trayIcon->showMessage("QNetSoul", tr("QNetSoul is still running."),
										 QSystemTrayIcon::Information, 5000);
		}
		event->ignore();
	}
}

void	QNetsoul::connectToServer(void)
{
	if (!this->_options->loginLineEdit->text().isEmpty())
	{
		if (!this->_options->passwordLineEdit->text().isEmpty())
		{
			bool	ok;
			quint16	port = this->_options->portLineEdit->text().toUShort(&ok);
			if (true == ok)
			{
				this->statusbar->showMessage(tr("Connecting..."), 3000);
				this->_network->connect(this->_options->serverLineEdit->text(), port);
				return;
			}
			else
			{
				QMessageBox::warning(this, "QNetSoul", tr("Port is invalid."));
				openOptionsDialog(this->_options->portLineEdit);
			}
		}
		else
		{
			QMessageBox::warning(this, "QNetSoul", tr("Your password is missing."));
			openOptionsDialog(this->_options->passwordLineEdit);
		}
	}
	else
	{
		QMessageBox::warning(this, "QNetSoul", tr("Your login is missing."));
		openOptionsDialog(this->_options->loginLineEdit);
	}
}

void	QNetsoul::disconnect(void)
{
	resetAllContacts();
	this->_network->disconnect();

}

void	QNetsoul::updateMenuBar(const QAbstractSocket::SocketState& state)
{
	if (QAbstractSocket::ConnectedState == state)
	{
		actionConnect->setEnabled(false);
		actionDisconnect->setEnabled(true);
	}
	else if (QAbstractSocket::UnconnectedState == state)
	{
		actionConnect->setEnabled(true);
		actionDisconnect->setEnabled(false);
	}
}

void	QNetsoul::updateStatusBar(const QAbstractSocket::SocketState& state)
{
	if (QAbstractSocket::ConnectedState == state)
	{
		this->_statusPushButton->setText(tr(ONLINE));
		this->_statusPushButton->setToolTip(tr("Disconnect"));
	}
	else if (QAbstractSocket::UnconnectedState == state)
	{
		this->_statusPushButton->setText(tr(OFFLINE));
		this->_statusPushButton->setToolTip(tr("Get NetSouled !"));
		this->statusbar->showMessage(tr("Disconnected"), 2000);
	}
}

void	QNetsoul::updateStatusComboBox(const QAbstractSocket::SocketState& state)
{
	if (QAbstractSocket::ConnectedState == state)
	{
		this->statusComboBox->setEnabled(true);
	}
	else if (QAbstractSocket::UnconnectedState == state)
	{
		this->statusComboBox->setEnabled(false);
		this->statusComboBox->setCurrentIndex(0);
	}
}

void	QNetsoul::displayError(const QAbstractSocket::SocketError& error)
{
	switch (error)
	{
		case QAbstractSocket::HostNotFoundError:
			QMessageBox::warning(this, "QNetsoul", tr("The host was not found."));
			break;

		case QAbstractSocket::ConnectionRefusedError:
			QMessageBox::warning(this, "QNetsoul", tr("The connection was refused by the peer."));
			break;
		case QAbstractSocket::NetworkError:
			QMessageBox::warning(this, "QNetsoul", tr("Perhaps, the port is wrong...\n"
			"Please check that you ethernet cable is plugged in."));
			break;
		default: QMessageBox::warning(this, "QNetsoul", tr("Error !"));
			std::cerr << "Error number: " << error << std::endl;
	}
}

void	QNetsoul::toggleConnection(void)
{
	if (tr(OFFLINE) == this->_statusPushButton->text())
	{
		connectToServer();
	}
	else
	{
		disconnect();
	}
}

void	QNetsoul::saveStateBeforeQuiting(void)
{
	saveContacts("contacts.xml");
	writeSettings();
	qApp->quit();
}

void	QNetsoul::showConversation(const QModelIndex& index)
{
	ContactWidget*	cw = static_cast<ContactWidget*>(contactsListView->indexWidget(index));
	if (NULL != cw)
	{
		showConversation(cw->aliasLabel->text());
	}
}

void	QNetsoul::openAddContactDialog(void)
{
	if (this->_addContact->isVisible() == false)
	{
		this->_addContact->loginLineEdit->clear();
		this->_addContact->loginLineEdit->setFocus();
		this->_addContact->show();
	}
	else
	{
		this->_addContact->activateWindow();
	}
}

void	QNetsoul::openOptionsDialog(QLineEdit* newLineFocus)
{
	if (this->_options->isVisible() == false)
	{
		this->_options->update();
		if (NULL != newLineFocus)
		{
			newLineFocus->setFocus();
		}
		else
		{
			this->_options->serverLineEdit->setFocus();
		}
		this->_options->show();
	}
	else
	{
		this->_options->activateWindow();
	}
}

void	QNetsoul::loadContacts(void)
{
	const QString fileName =
             QFileDialog::getOpenFileName(this, tr("Open Contacts File"),
                                          QDir::currentPath(),
                                          tr("XML Files (*.xml)"));
	if (false == fileName.isEmpty())
		loadContacts(fileName);
}

void	QNetsoul::saveContactsAs(void)
{
	QString fileName =
			QFileDialog::getSaveFileName(this, tr("Save Contacts File"),
										 QDir::currentPath(),
										 tr("XML Files (*.xml)"));
	if (!fileName.isEmpty())
		saveContacts(fileName);
}

void	QNetsoul::toggleSortContacts(void)
{
	this->contactsListView->model()->sort(0);
}

void	QNetsoul::handleClicksOnTrayIcon(QSystemTrayIcon::ActivationReason reason)
{
	if (QSystemTrayIcon::Trigger == reason)
	{
		if (this->isVisible())
		{
			this->_oldPos = this->pos();
			this->hide();
		}
		else
		{
			this->show();
		}
	}
}

void	QNetsoul::addContact(void)
{
	addContact(this->_addContact->loginLineEdit->text());
}

void	QNetsoul::addContact(const QList<Contact> list)
{
	const int	size = list.size();

	for (int i = 0; i < size; ++i)
	{
		addContact(list[i].login, list[i].alias);
	}
}

void	QNetsoul::addContact(const QString& login, const QString& alias)
{
	if (!login.isEmpty() && false == doesThisContactAlreadyExist(login))
	{
		this->_portraitResolver.addRequest(login, false);
		ContactWidget*	widget = new ContactWidget(this, login, alias);
		QStandardItem*	newItem = new QStandardItem();
		newItem->setSizeHint(widget->size());
		this->_standardItemModel->insertRow(this->_standardItemModel->rowCount(), newItem);
		this->contactsListView->setIndexWidget(newItem->index(), widget);
		this->_addContact->loginLineEdit->clear();
		if (QAbstractSocket::ConnectedState == this->_network->state())
		{
			refreshContact(login);
			watchLogContact(login);
		}
	}
}

// Ca a l'air vraiment trop simple :)=
void	QNetsoul::removeSelectedContact(void)
{
	const	int row = this->contactsListView->currentIndex().row();
	this->contactsListView->model()->removeRow(row);
}

void	QNetsoul::sendStatus(const int& status) const
{
	switch (status)
	{
		case 0: this->_network->sendMessage("state actif\n"); break;
		case 1: this->_network->sendMessage("state lock\n"); break;
		case 2: this->_network->sendMessage("state away\n"); break;
		case 3: this->_network->sendMessage("state server\n"); break;
		default:;
	}
}

// A REVOIR si ça ne marche pas...
void	QNetsoul::changeStatus(const QString& login, const QString& id, const QString& state)
{
	ContactWidget*	contactWidget = getContact(login);

	if (NULL != contactWidget)
	{
		if (NULL != this->_trayIcon)
			this->_trayIcon->showMessage(login, "is now " + state, QSystemTrayIcon::NoIcon);
		if (0 == contactWidget->getConnectionsSize())
		{
			refreshContact(login);
		}
		else
		{
			contactWidget->updateConnectionPoint(id, state);

			Chat*	chat = getChat(login);
			if (NULL != chat)
			{
				chat->statusLabel->setPixmap(contactWidget->getStatus());
			}
		}
	}
}

// id(4), login(5), ip(6), group(13), state(14), location(12), comment(15)
void	QNetsoul::updateContact(const QStringList& parts)
{
	ContactWidget*	contactWidget = getContact(parts.at(5));

	if (NULL != contactWidget)
	{
		ConnectionPoint	connection = {parts.at(4), parts.at(6),
									  parts.at(14).section(':', 0, 0),
									  url_decode(parts.at(15).toStdString().c_str()),
									  url_decode(parts.at(12).toStdString().c_str())};

		contactWidget->addConnectionPoint(connection);
		if (false == contactWidget->hasGroup())
			contactWidget->setGroup(parts.at(13));
	}
}

void	QNetsoul::showConversation(const QString& login, const QString& message)
{
	Chat*	window = getChat(login);

	if (NULL == window)
	{
		ContactWidget*	contactWidget = getContact(login);

		window = createWindowChat(login);
		window->inputTextEdit->setFocus();
		if (contactWidget)
			window->statusLabel->setPixmap(contactWidget->getStatus());
		window->setVisible(true);
	}
	else
	{
		if (false == window->isVisible())
		{
			window->setVisible(true);
			window->outputTextEdit->clear();
			window->inputTextEdit->clear();
			window->inputTextEdit->setFocus();
		}
		else
		{
			window->showNormal();
		}
		//window->activateWindow(); // Option ?
	}
	if (window && false == message.isEmpty())
	{
		window->insertMessage(login, QString(url_decode(message.toStdString().c_str())), QColor(204, 0, 0));
	}
}

void	QNetsoul::processHandShaking(int step, QStringList args)
{
	static QByteArray	sum;
	std::cerr << "Step: " << step << std::endl;

	switch (step)
	{
		case 0:
		{
			const QString	password = this->_options->passwordLineEdit->text();
			if (!password.isEmpty() && args.size() > 3)
			{
				sum.clear();
				this->_timeStamp = args.at(5);
				sum.append(QString("%1-%2/%3%4")
				.arg(args.at(2)).arg(args.at(3)).arg(args.at(4)).arg(password));
				sum = QCryptographicHash::hash(sum, QCryptographicHash::Md5);
				this->_network->sendMessage("auth_ag ext_user none none\n");
			}
			break;
		}
		case 1:
		{
			const QString hex = sum.toHex();
			QString location(this->_options->locationLineEdit->text());
			QString comment(this->_options->commentLineEdit->text());
			if (location.isEmpty() || location.contains("%L"))
				this->_network->resolveLocation(location);
			if (comment.isEmpty())
				comment = "{QNetSoul} \\o/";
			QByteArray	message;
			message.append("ext_user_log ");
			message.append(this->_options->loginLineEdit->text() + ' ');
			message.append(hex);
			message.append(' ');
			message.append(url_encode(location.toStdString().c_str()));
			message.append(' ');
			message.append(url_encode(comment.toStdString().c_str()));
			message.append("\n");
			std::cout << message.data() << std::endl;
			this->_network->sendMessage(message);
			break;
		}
		case 2:
		{
			QByteArray	state;
			state.append("state actif\n");
			//state.append(QString::number(QString(this->_timeStamp).toInt() + 50));
			//state.append("\n");
			this->_network->sendMessage(state);
			this->_network->sendMessage("ping\n");
			watchLogContacts();
			refreshContacts();
			this->statusbar->showMessage(tr("You are now netsouled."), 2000);
			break;
		}
		default:;
	}
}

void	QNetsoul::transmitMsg(const QString& login, const QString& msg)
{
	Chat*	chat = getChat(login);

	if (NULL != chat)
	{
		chat->insertMessage(this->_options->loginLineEdit->text(), msg, QColor(32, 74, 135));
	}

	QByteArray	result;
	result.append("user_cmd msg_user " + login + " msg ");
	result.append(url_encode(msg.toStdString().c_str()));
	result.append('\n');
	this->_network->sendMessage(result);
}

// A FAIRE: il faut trouver la bonne commande... ~~
void	QNetsoul::transmitTypingStatus(const QString& /*login*/, bool /*status*/)
{
	/*
	if (!status)
		this->_network->sendMessage("user_cmd UserTyping\n");
	std::cerr << std::boolalpha << status << std::endl;
	*/
}

void	QNetsoul::notifyTypingStatus(const QString& login, bool typing)
{
	QMap<QString, Chat*>::iterator    it;
	it = this->_windowsChat.find(login);
	if (it != this->_windowsChat.end())
	{
		it.value()->notifyTypingStatus(login, typing);
	}
}

void	QNetsoul::setPortrait(const QString& login)
{
	const QString	filename = PortraitResolver::buildFilename(login, false);
	QDir			portraits("./portraits");

	if (false == portraits.exists(filename))
		return;

	Chat*	chat = getChat(login);

	if (NULL != chat)
	{
		chat->portraitLabel->setPixmap(QPixmap("./portraits/" + filename));
	}
	ContactWidget*	cw = getContact(login);
	if (NULL != cw)
		cw->portraitLabel->setPixmap(QPixmap("./portraits/" + filename));
}

void	QNetsoul::aboutQNetSoul(void)
{
	QMessageBox::about(this, "QNetSoul", this->whatsThis());
}

Chat*	QNetsoul::getChat(const QString& login) const
{
	QMap<QString, Chat*>::const_iterator	cit;

	cit = this->_windowsChat.find(login);
	if (this->_windowsChat.end() == cit)
		return (NULL);
	return (cit.value());
}
	
bool	QNetsoul::doesThisContactAlreadyExist(const QString& contact) const
{
	if (getContact(contact))
		return (true);
	return (false);
}

ContactWidget*	QNetsoul::getContact(const QString& login) const
{
	const int	rows = this->_standardItemModel->rowCount();

	for (int i = 0; i < rows; ++i)
	{
		QStandardItem*	standardItem = this->_standardItemModel->item(i);
		if (standardItem != NULL)
		{
			QWidget*		widget = this->contactsListView->indexWidget(standardItem->index());
			ContactWidget*	contactWidget = dynamic_cast<ContactWidget*>(widget);
			if (contactWidget != NULL)
			{
				if (login == contactWidget->aliasLabel->text())
				{
					return (contactWidget);
				}
			}
		}
	}
	return (NULL);
}

QStringList		QNetsoul::getContactLogins(void) const
{
	QStringList	list;
	const int	rows = this->_standardItemModel->rowCount();

	for (int i = 0; i < rows; ++i)
	{
		QStandardItem*	standardItem = this->_standardItemModel->item(i);
		if (standardItem != NULL)
		{
			QWidget*		widget = this->contactsListView->indexWidget(standardItem->index());
			ContactWidget*	contactWidget = dynamic_cast<ContactWidget*>(widget);
			if (contactWidget != NULL)
			{
				list.push_back(contactWidget->getLogin());
			}
		}
	}
	return (list);
}

QList<ContactWidget*>		QNetsoul::getContactWidgets(void) const
{
	QList<ContactWidget*>	list;
	const int				rows = this->_standardItemModel->rowCount();

	for (int i = 0; i < rows; ++i)
	{
		QStandardItem*	standardItem = this->_standardItemModel->item(i);
		if (standardItem != NULL)
		{
			QWidget*		widget = this->contactsListView->indexWidget(standardItem->index());
			ContactWidget*	contactWidget = dynamic_cast<ContactWidget*>(widget);
			if (contactWidget != NULL)
			{
				list.push_back(contactWidget);
			}
		}
	}
	return (list);
}

void	QNetsoul::watchLogContacts(void)
{
	const QStringList	list = getContactLogins();
	const int			size = list.size();

	if (size > 0)
	{
		QByteArray	netMsg("user_cmd watch_log_user {");
		for (int i = 0; i < size; ++i)
		{
			netMsg.append(list[i]);
			if (i + 1 < size)
				netMsg.append(',');
		}
		netMsg.append("}\n");
		this->_network->sendMessage(netMsg);
	}
}

void	QNetsoul::watchLogContact(const QString& contact)
{
	QByteArray	netMsg("user_cmd watch_log_user ");
	netMsg.append(contact);
	netMsg.append('\n');
	this->_network->sendMessage(netMsg);
}

void	QNetsoul::refreshContact(const QString& contact) const
{
	QByteArray	message("user_cmd who ");
	message.append(contact + "\n");
	this->_network->sendMessage(message);
}

void	QNetsoul::refreshContacts(void) const
{
	const QStringList	contacts = getContactLogins();

	const int	size = contacts.size();
	for (int i = 0; i < size; ++i)
	{
		refreshContact(contacts.at(i));
	}
}

void	QNetsoul::resetAllContacts(void) const
{
	const QList<ContactWidget*>	contacts = getContactWidgets();

	 const int	size = contacts.size();
	 for (int i = 0; i < size; ++i)
	 {
		 contacts[i]->reset();
	 }
}

void	QNetsoul::setupTrayIcon(void)
{
	if (QSystemTrayIcon::isSystemTrayAvailable())
	{
		this->_trayIcon = new QSystemTrayIcon(this);
		QMenu*		trayIconMenu = new QMenu(this);
		QAction*	trayIconActionQuit = new QAction(QIcon(":/images/quit.png"), tr("Quit"), this);
		trayIconMenu->addAction(trayIconActionQuit);
		this->_trayIcon->setContextMenu(trayIconMenu);
		connect(trayIconActionQuit, SIGNAL(triggered()), SLOT(saveStateBeforeQuiting()));
		connect(this->_trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
				SLOT(handleClicksOnTrayIcon(QSystemTrayIcon::ActivationReason)));
		this->_trayIcon->setIcon(QIcon(":/images/qnetsoul.png"));
		this->_trayIcon->show();
	}
}

void	QNetsoul::setupStatusButton(void)
{
	this->_statusPushButton = new QPushButton(tr("Offline"));
	this->_statusPushButton->setFlat(true);
	this->_statusPushButton->setToolTip(tr("Get NetSouled !"));
	this->statusbar->addPermanentWidget(_statusPushButton);
	connect(this->_statusPushButton, SIGNAL(clicked()), SLOT(toggleConnection()));
}

void	QNetsoul::readSettings(void)
{
	QSettings	settings("Epitech", "QNetsoul");

	settings.beginGroup("MainWindow");
	resize(settings.value("size", QSize(240, 545)).toSize());
	move(settings.value("pos", QPoint(501, 232)).toPoint());
	settings.endGroup();
}

void	QNetsoul::writeSettings(void)
{
	QSettings	settings("Epitech", "QNetsoul");

	settings.beginGroup("MainWindow");
	settings.setValue("size", size());
	if (this->isVisible())
		settings.setValue("pos", pos());
	else
		settings.setValue("pos", this->_oldPos);
	settings.endGroup();
	this->_options->writeOptionSettings();
}

void	QNetsoul::loadContacts(const QString& fileName)
{
	QFile file(fileName);
	if (!file.open(QFile::ReadOnly | QFile::Text))
	{
		this->statusBar()->showMessage(tr("Unable to load Contacts"), 2000);
		return;
	}

	QList<Contact>	list;
	ContactsReader reader;

	list = reader.read(&file);
	addContact(list);
	this->statusBar()->showMessage(tr("Contacts loaded"), 2000);
}

void	QNetsoul::saveContacts(const QString& fileName)
{
	QFile file(fileName);
	if (!file.open(QFile::WriteOnly | QFile::Text)) {
		QMessageBox::warning(this, tr("QXmlStream Contacts"),
							 tr("Cannot write file %1:\n%2.")
							 .arg(fileName)
							 .arg(file.errorString()));
		return;
	}

	ContactsWriter writer(getContactWidgets());
	if (writer.writeFile(&file))
		this->statusBar()->showMessage(tr("Contacts saved"), 2000);
}

void	QNetsoul::connectQNetsoulItems(void)
{
	connect(this->_addContact->addPushButton, SIGNAL(clicked()), SLOT(addContact()));
	connect(contactsListView, SIGNAL(activated(const QModelIndex&)),
			SLOT(showConversation(const QModelIndex&)));
	connect(&this->_portraitResolver, SIGNAL(downloadedPortrait(const QString&)),
			SLOT(setPortrait(const QString&)));
}

void	QNetsoul::connectActionsSignals(void)
{
	// QNetsoul
	connect(actionConnect, SIGNAL(triggered()), SLOT(connectToServer()));
	connect(actionDisconnect, SIGNAL(triggered()), SLOT(disconnect()));
	connect(actionQuit, SIGNAL(triggered()), SLOT(saveStateBeforeQuiting()));
	// Account
	connect(actionAdd_a_contact, SIGNAL(triggered()), SLOT(openAddContactDialog()));
	connect(actionRemove_selected_contact, SIGNAL(triggered()), SLOT(removeSelectedContact()));
	connect(action_Load_contacts, SIGNAL(triggered()), SLOT(loadContacts()));
	connect(actionSave_contacts_as, SIGNAL(triggered()), SLOT(saveContactsAs()));
	connect(actionToggle_sort_contacts, SIGNAL(triggered()), SLOT(toggleSortContacts()));
	// Options
	connect(actionPreferences, SIGNAL(triggered()), SLOT(openOptionsDialog()));
	// Help
	connect(actionAbout_QNetSoul, SIGNAL(triggered()), SLOT(aboutQNetSoul()));
	connect(actionAbout_Qt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
	// Status
	connect(statusComboBox, SIGNAL(currentIndexChanged(int)), SLOT(sendStatus(const int&)));
}

void	QNetsoul::connectNetworkSignals(void)
{
	connect(this->_network, SIGNAL(handShaking(int, QStringList)), SLOT(processHandShaking(int, QStringList)));
	connect(this->_network, SIGNAL(message(const QString&, const QString&)),
			SLOT(showConversation(const QString&, const QString&)));
	connect(this->_network, SIGNAL(status(const QString&, const QString&, const QString&)),
			SLOT(changeStatus(const QString&, const QString&, const QString&)));
	connect(this->_network, SIGNAL(who(const QStringList&)), SLOT(updateContact(const QStringList&)));
	connect(this->_network, SIGNAL(typingStatus(const QString&, bool)), SLOT(notifyTypingStatus(const QString&, bool)));
}

Chat*	QNetsoul::createWindowChat(const QString& name)
{
	Chat*	chat = new Chat(name, this);
	this->_windowsChat.insert(name, chat);
	connect(chat, SIGNAL(msgToSend(const QString& , const QString&)), this, SLOT(transmitMsg(const QString&, const QString&)));
	connect(chat, SIGNAL(typingSignal(const QString&, bool)), this, SLOT(transmitTypingStatus(const QString&, bool)));
	return (chat);
}

void	QNetsoul::deleteAllWindowChats(void)
{
	QMap<QString, Chat*>::const_iterator    cit = this->_windowsChat.constBegin();

	for (; cit != this->_windowsChat.constEnd(); ++cit)
	{
		delete cit.value();
	}
}
