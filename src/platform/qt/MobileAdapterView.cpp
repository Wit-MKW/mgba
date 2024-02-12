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

static int mobileConvertAddr(const QString& addr, Address *output, unsigned *port) {
	QHostAddress qaddress;
	if (addr.isEmpty()) {
		return 0;
	}
	bool hasPort = false;
	unsigned portInt = *port;
	if (!qaddress.setAddress(addr)) {
		QString portText = addr.section(':', -1);
		QString mainText = addr.section(':', 0, -2);
		hasPort = false;
		portInt = portText.toUInt(&hasPort);
		if (!hasPort || mainText.contains(':')) {
			if (!mainText.startsWith('[')) {
				return -1;
			}
			if (!mainText.endsWith(']')) {
				if (!portText.endsWith(']')) {
					return -1;
				}
				mainText += QString(':') + portText;
				hasPort = false;
			}
			mainText.remove(0, 1);
			mainText.remove(-1, 1);
		}
		if (!qaddress.setAddress(mainText)) {
			return -1;
		}
	}
	if (!convertAddress(&qaddress, output)) {
		return -1;
	}
	if (hasPort) {
		*port = portInt;
	}
	return 1;
}

MobileAdapterView::MobileAdapterView(std::shared_ptr<CoreController> controller, Window* window, QWidget* parent)
	: QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint)
	, m_tokenFilled(false)
	, m_controller(controller)
	, m_window(window)
{
	m_ui.setupUi(this);

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
	QString text = m_ui.setDns1->text();
	Address addr;
	int res = mobileConvertAddr(text, &addr, &port);
	if (res == 1) {
		m_controller->setMobileAdapterDns1(addr, port);
	} else if (res == 0) {
		m_controller->clearMobileAdapterDns1();
	}
	getConfig();
}

void MobileAdapterView::setDns2() {
	unsigned port = MOBILE_DNS_PORT;
	QString text = m_ui.setDns2->text();
	Address addr;
	int res = mobileConvertAddr(text, &addr, &port);
	if (res == 1) {
		m_controller->setMobileAdapterDns2(addr, port);
	} else if (res == 0) {
		m_controller->clearMobileAdapterDns2();
	}
	getConfig();
}

void MobileAdapterView::setPort(int port) {
	m_controller->setMobileAdapterPort(port);
	getConfig();
}

void MobileAdapterView::setRelay() {
	unsigned port = MOBILE_DEFAULT_RELAY_PORT;
	QString text = m_ui.setRelay->text();
	Address addr;
	int res = mobileConvertAddr(text, &addr, &port);
	if (res == 1) {
		m_controller->setMobileAdapterRelay(addr, port);
	} else if (res == 0) {
		m_controller->clearMobileAdapterRelay();
	}
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
	QString statusText, userNumber, peerNumber, token;
	m_controller->updateMobileAdapter(&statusText, &userNumber, &peerNumber, &token);
	m_ui.statusText->setText(statusText);
	m_ui.userNumber->setText(userNumber);
	m_ui.peerNumber->setText(peerNumber);
	if (!m_tokenFilled && m_ui.setToken->text() == "") {
		m_ui.setToken->setText(token);
		m_tokenFilled = token != "";
	}
}

#endif
