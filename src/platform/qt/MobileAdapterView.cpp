#ifdef USE_LIBMOBILE

#include "MobileAdapterView.h"

#include "ConfigController.h"
#include "CoreController.h"
#include "ShortcutController.h"
#include "Window.h"
#include "utils.h"

#include <QtAlgorithms>
#include <QClipboard>
#include <QFileInfo>
#include <QFontMetrics>
#include <QMessageBox>
#include <QMultiMap>
#include <QSettings>
#include <QStringList>

using namespace QGBA;

MobileAdapterView::MobileAdapterView(std::shared_ptr<CoreController> controller, Window* window, QWidget* parent)
	: QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint)
	, m_tokenFilled(false)
	, m_controller(controller)
	, m_window(window)
{
	m_ui.setupUi(this);

#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
	int size = QFontMetrics(QFont()).height() / ((int) ceil(devicePixelRatioF()) * 12);
#else
	int size = QFontMetrics(QFont()).height() / (devicePixelRatio() * 12);
#endif
	if (!size) {
		size = 1;
	}

	QRegularExpression reToken("[\\dA-Fa-f]{32}?");
	QRegularExpressionValidator vToken(reToken, m_ui.setToken);
	m_ui.setToken->setValidator(&vToken);

	connect(m_ui.setType, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &MobileAdapterView::setType);
	connect(m_ui.setUnmetered, &QAbstractButton::toggled, this, &MobileAdapterView::setUnmetered);
	connect(m_ui.setDns1, &QLineEdit::editingFinished, this, &MobileAdapterView::setDns1);
	connect(m_ui.setDns2, &QLineEdit::editingFinished, this, &MobileAdapterView::setDns2);
	connect(m_ui.setPort, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &MobileAdapterView::setPort);
	connect(m_ui.setRelay, &QLineEdit::editingFinished, this, &MobileAdapterView::setRelay);
	connect(m_ui.setToken, &QLineEdit::editingFinished, this, &MobileAdapterView::setToken);
	connect(m_ui.copyToken, &QAbstractButton::clicked, this, &MobileAdapterView::copyToken);

	connect(m_controller.get(), &CoreController::frameAvailable, this, &MobileAdapterView::advanceFrameCounter);
	connect(controller.get(), &CoreController::stopping, this, &QWidget::close);

	m_controller->attachMobileAdapter();
	getConfig();
}

MobileAdapterView::~MobileAdapterView() {
	m_controller->detachMobileAdapter();
}

void MobileAdapterView::setType(int type) {
	m_controller->setMobileAdapterType(type);
	getConfig();
}

void MobileAdapterView::setUnmetered(bool unmetered) {
	m_controller->setMobileAdapterUnmetered(unmetered);
	getConfig();
}

void MobileAdapterView::setDns1() {
	unsigned port = MOBILE_DNS_PORT;
	QHostAddress qaddress;
	Address address;
	QString addrtext = m_ui.setDns1->text();
	if (addrtext.isEmpty()) {
		m_controller->clearMobileAdapterDns1();
		goto error;
	}
	if (!qaddress.setAddress(addrtext)) {
		QString porttext = addrtext.section(':', -1);
		addrtext = addrtext.section(':', 0, -2);
		bool ok = false;
		port = porttext.toUInt(&ok);
		if (!ok || addrtext.contains(':')) {
			if (!addrtext.startsWith('[')) goto error;
			if (!addrtext.endsWith(']')) {
				if (!porttext.endsWith(']')) goto error;
				addrtext += QString(':') + porttext;
				port = MOBILE_DNS_PORT;
			}
			addrtext.remove(0, 1);
			addrtext.remove(-1, 1);
		}
		qaddress.setAddress(addrtext);
	}
	convertAddress(&qaddress, &address);
	m_controller->setMobileAdapterDns1(address, port);
error:
	getConfig();
}

void MobileAdapterView::setDns2() {
	unsigned port = MOBILE_DNS_PORT;
	QHostAddress qaddress;
	Address address;
	QString addrtext = m_ui.setDns2->text();
	if (addrtext.isEmpty()) {
		m_controller->clearMobileAdapterDns2();
		goto error;
	}
	if (!qaddress.setAddress(addrtext)) {
		QString porttext = addrtext.section(':', -1);
		addrtext = addrtext.section(':', 0, -2);
		bool ok = false;
		port = porttext.toUInt(&ok);
		if (!ok || addrtext.contains(':')) {
			if (!addrtext.startsWith('[')) goto error;
			if (!addrtext.endsWith(']')) {
				if (!porttext.endsWith(']')) goto error;
				addrtext += QString(':') + porttext;
				port = MOBILE_DNS_PORT;
			}
			addrtext.remove(0, 1);
			addrtext.remove(-1, 1);
		}
		qaddress.setAddress(addrtext);
	}
	convertAddress(&qaddress, &address);
	m_controller->setMobileAdapterDns2(address, port);
error:
	getConfig();
}

void MobileAdapterView::setPort(int port) {
	m_controller->setMobileAdapterPort(port);
	getConfig();
}

void MobileAdapterView::setRelay() {
	unsigned port = MOBILE_DEFAULT_RELAY_PORT;
	QHostAddress qaddress;
	Address address;
	QString addrtext = m_ui.setRelay->text();
	if (addrtext.isEmpty()) {
		m_controller->clearMobileAdapterRelay();
		goto error;
	}
	if (!qaddress.setAddress(addrtext)) {
		QString porttext = addrtext.section(':', -1);
		addrtext = addrtext.section(':', 0, -2);
		bool ok = false;
		port = porttext.toUInt(&ok);
		if (!ok || addrtext.contains(':')) {
			if (!addrtext.startsWith('[')) goto error;
			if (!addrtext.endsWith(']')) {
				if (!porttext.endsWith(']')) goto error;
				addrtext += QString(':') + porttext;
				port = MOBILE_DEFAULT_RELAY_PORT;
			}
			addrtext.remove(0, 1);
			addrtext.remove(-1, 1);
		}
		qaddress.setAddress(addrtext);
	}
	convertAddress(&qaddress, &address);
	m_controller->setMobileAdapterRelay(address, port);
error:
	getConfig();
}

void MobileAdapterView::setToken() {
	QString token = m_ui.setToken->text();
	if (m_controller->setMobileAdapterToken(token)) {
		m_ui.setToken->setText(token);
		m_tokenFilled = true;
	} else {
		m_ui.setToken->setText("");
		m_tokenFilled = false;
	}
}

void MobileAdapterView::copyToken(bool checked) {
	UNUSED(checked);
	getConfig();
	QGuiApplication::clipboard()->setText(m_ui.setToken->text());
}

void MobileAdapterView::getConfig() {
	int type;
	bool unmetered;
	QString dns1, dns2;
	int port;
	QString relay;
	m_controller->getMobileAdapterConfig(&type, &unmetered, &dns1, &dns2, &port, &relay);
	m_ui.setType->setCurrentIndex(type);
	m_ui.setUnmetered->setChecked(unmetered);
	m_ui.setDns1->setText(dns1);
	m_ui.setDns2->setText(dns2);
	m_ui.setPort->setValue(port);
	m_ui.setRelay->setText(relay);
	advanceFrameCounter();
}

void MobileAdapterView::advanceFrameCounter() {
	QString userNumber, peerNumber, token;
	m_controller->updateMobileAdapter(&userNumber, &peerNumber, &token);
	m_ui.userNumber->setText(userNumber);
	m_ui.peerNumber->setText(peerNumber);
	if (!m_tokenFilled && m_ui.setToken->text() == "") {
		m_ui.setToken->setText(token);
		m_tokenFilled = token != "";
	}
}

#endif
