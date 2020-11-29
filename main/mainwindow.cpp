#include "mainwindow.h"
#include <QFileDialog>
#include <QUrl>
#include <QTimerEvent>
#include <QProcess>
#include <QDesktopWidget>
#include <QMouseEvent>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDataStream>
#include <QDir>
#include <QTextStream>
#include <stdio.h>
#include <Windows.h>
#include <Iphlpapi.h>
#include <Assert.h>
#include <QDate>
#include <QTime>
#include <QSettings>
#define COMPILERVERSION 100
#pragma comment(lib, "iphlpapi.lib")

char* getMAC() {
	PIP_ADAPTER_INFO AdapterInfo;
	DWORD dwBufLen = sizeof(IP_ADAPTER_INFO);
	char *mac_addr = (char*)malloc(18);

	AdapterInfo = (IP_ADAPTER_INFO *) malloc(sizeof(IP_ADAPTER_INFO));
	if (AdapterInfo == NULL) {
		printf("Error allocating memory needed to call GetAdaptersinfo\n");
		free(mac_addr);
		return NULL; // it is safe to call free(NULL)
	}

	// Make an initial call to GetAdaptersInfo to get the necessary size into the dwBufLen variable
	if (GetAdaptersInfo(AdapterInfo, &dwBufLen) == ERROR_BUFFER_OVERFLOW) {
		free(AdapterInfo);
		AdapterInfo = (IP_ADAPTER_INFO *) malloc(dwBufLen);
		if (AdapterInfo == NULL) {
			printf("Error allocating memory needed to call GetAdaptersinfo\n");
			free(mac_addr);
			return NULL;
		}
	}

	if (GetAdaptersInfo(AdapterInfo, &dwBufLen) == NO_ERROR) {
		// Contains pointer to current adapter info
		PIP_ADAPTER_INFO pAdapterInfo = AdapterInfo;
		do {
			// technically should look at pAdapterInfo->AddressLength
			//   and not assume it is 6.
			sprintf(mac_addr, "%02X:%02X:%02X:%02X:%02X:%02X",
				pAdapterInfo->Address[0], pAdapterInfo->Address[1],
				pAdapterInfo->Address[2], pAdapterInfo->Address[3],
				pAdapterInfo->Address[4], pAdapterInfo->Address[5]);
			printf("Address: %s, mac: %s\n", pAdapterInfo->IpAddressList.IpAddress.String, mac_addr);
			// print them all, return the last one.
			// return mac_addr;

			printf("\n");
//			QMessageBox::information(NULL,"",mac_addr) ;
			pAdapterInfo = pAdapterInfo->Next;        
			break ;
		} while(pAdapterInfo);                        
	}
	free(AdapterInfo);
	return mac_addr; // caller must free.
}

MainWindow::MainWindow(QWidget *parent, Qt::WindowFlags flags)
    : QMainWindow(parent, flags)
{
	ui.setupUi(this) ;
	if( !checkUSBDevice() )
	{
		QMessageBox::information(NULL,"", "Software can not run on this drive.") ;
		close() ;
		return ;
	}
	m_installPath = "C://tomsoftware" ;
	initUI() ;
	initConnection() ;
	loadPrograms() ;
	m_MACAddress = getMAC() ;
}

void MainWindow::initUI()
{
	setFixedSize(600,293) ;
	QDate dt = QDate::currentDate() ;
	ui.expireDate->setMinimumDate(dt) ;
	ui.expireDate->setDate(dt.addMonths(2)) ;
	QDir().mkpath(m_installPath) ;
	ui.installPath->setText(m_installPath) ;
	ui.cb_software->setStyleSheet("QListView { background:white;font: 16pt 'Impact'; }");
	ui.progressBar->hide() ;
}

void MainWindow::initConnection()
{
	connect( ui.cb_software, SIGNAL(currentIndexChanged(int)), this, SLOT(onCurrentIndexChanged(int)) ) ;
	connect( ui.tb_active, SIGNAL(clicked()), this, SLOT(onActive())) ;
	connect( ui.tb_deactive, SIGNAL(clicked()), this, SLOT(onDeactive())) ;
	connect( ui.tb_change_dir, SIGNAL(clicked()), this, SLOT(onChangeInstallPath())) ;
	connect( ui.installPath, SIGNAL(textChanged(QString)), this, SLOT(onInstallPathChanged(QString))) ;
}

void MainWindow::onCurrentIndexChanged( int id )
{
	QString software = ui.cb_software->currentText() ;
	ui.installPath->setText("C://tomsoftware/"+software) ;

	QFile configFile("data/src/"+ui.cb_software->currentText()+"/config.ini") ;
	if( configFile.exists() )
	{
		QTextStream configStream(&configFile) ;
		QString version ;
		qint64 hashKey ;
		if( configFile.open(QIODevice::ReadOnly) )
		{
			version = configStream.readLine() ;
			hashKey = configStream.readLine().toInt() ;
			configFile.close() ;
			QSettings settings(REGPATH, QSettings::NativeFormat) ;	
			QVariant var = settings.value(QString("%1").arg(hashKey)) ;
			bool activated = false ;
			if( !var.isNull() )
			{
				QString str = var.toString(); 
				SimpleCrypt processSimpleCrypt(hashKey);
				str = processSimpleCrypt.decryptToString(str);
				QStringList tok = str.split("::") ;
				if( tok.count() == 3 )
				{
					QDate expired = QDate::fromString(tok[1],"dd.MM.yyyy") ;
					ui.expireDate->setDate(expired) ;
					if( expired > QDate::currentDate() ) activated = true ;
				}
			}
			if( !activated )
			{
				ui.expireDate->setDate(QDate::currentDate().addMonths(2)) ;
				ui.tb_active->setVisible(true) ;
				ui.tb_deactive->setVisible(false) ;
			}
			else
			{
				ui.tb_active->setVisible(false) ;
				ui.tb_deactive->setVisible(true) ;
			}
		}
	}

}

void MainWindow::loadPrograms()
{
	QDir dir("data/src") ;
	foreach (QString d, dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
		ui.cb_software->addItem(d) ;
	}
	QFile file("res.dat") ;
	QDataStream in(&file) ;
	if( file.open(QIODevice::ReadOnly) )
	{
		int cnt ;
		int iVersion ;
		in >> cnt >> iVersion;

		for( int i = 0; i < cnt; i++ )
		{
			activateInfo info ;
			in >> info.softwareHashKey >> info.pcHardSN >> info.pcModelName >> info.pcProductName 
				>> info.activateDate >> info.expireDate ;
			m_activateInfoList << info ;
		}
		file.close() ;
	}
}

void MainWindow::onActive()
{
	if( ui.cb_software->count() == 0 ) return ;
	QString curSoft = ui.cb_software->currentText() ;
	QDir().mkdir(m_installPath) ;
	if( !QDir(m_installPath).exists() )
	{
		QMessageBox::information(NULL,"Error", "Install Path is not correct.") ;
		return ;
	}
	QString srcPath = "data/src/"+curSoft ;
	QDir srcDir(srcPath) ;
	if( !srcDir.exists() )
	{
		QMessageBox::information(NULL,"Error", "Source software does n't exist.") ;
		return ;
	}
	QFile configFile(srcPath+"/config.ini") ;
	if( !configFile.exists() )
	{
		QMessageBox::information(NULL,"Error", "Config file does n't exist.") ;
		return ;
	}
	QTextStream configStream(&configFile) ;
	QString qtVersion ;
	qint64 hashKey ;
	if( configFile.open(QIODevice::ReadOnly) )
	{
		qtVersion = configStream.readLine() ;
		hashKey = configStream.readLine().toInt() ;
		configFile.close() ;
	}
	if( !canActivate(hashKey) ) return ;
	QString freshPath = "data/fresh"+qtVersion ;
	m_totFileCount = 0 ;
	m_copiedFileCount = 0 ;
	if( qtVersion != "-1" ) m_totFileCount += fileCountInDir(freshPath)+fileCountInDir("data/src") ;
	QStringList extraFiles ;
	QFile extraFile(srcPath+"/extra.txt") ;
	if( extraFile.exists() )
	{
		extraFile.open(QIODevice::ReadOnly) ;
		QTextStream in(&extraFile) ;
		while( !in.atEnd() )
		{
			extraFiles << in.readLine() ;
		}
		extraFile.close() ;
	}
	m_totFileCount += extraFiles.count() ;
	QApplication::setOverrideCursor(Qt::WaitCursor) ;
	ui.progressBar->setMaximum(m_totFileCount) ;
	ui.progressBar->show() ;
	if( qtVersion != "-1" ) copyPath(freshPath,m_installPath) ;
	copyPath(srcPath,m_installPath) ;
	for( int i = 0; i < extraFiles.count(); i++ )
	{
		QFile::copy("data/common/"+extraFiles[i],m_installPath+"/"+extraFiles[i]) ;
		ui.progressBar->setValue(m_copiedFileCount++) ;
	}
	ui.progressBar->hide() ;
	QApplication::restoreOverrideCursor() ;
	QSettings settings(REGPATH, QSettings::NativeFormat);
	settings.setValue(QString::number(hashKey), squeezeString(m_MACAddress,ui.expireDate->date(),hashKey)) ;
	ui.tb_active->setVisible(false) ;
	ui.tb_deactive->setVisible(true) ;

	activateInfo info ;
	info.softwareHashKey = hashKey ;
	info.pcHardSN = getHDSerialNumber('C') ;
	info.pcModelName = getPCModelName() ;
	info.pcProductName = getPCProductName() ;
	info.activateDate = QDate::currentDate() ;
	info.expireDate = ui.expireDate->date() ;
	m_activateInfoList << info ;
	saveActivateInfo() ;
	QMessageBox::information(NULL,"Success", QString("%1 has been installed successfully.").arg(curSoft) ) ;
}

void MainWindow::onDeactive()
{
	QFile configFile("data/src/"+ui.cb_software->currentText()+"/config.ini") ;
	if( configFile.exists() )
	{
		QTextStream configStream(&configFile) ;
		QString version ;
		qint64 hashKey ;
		if( configFile.open(QIODevice::ReadOnly) )
		{
			version = configStream.readLine() ;
			hashKey = configStream.readLine().toInt() ;
			configFile.close() ;
			QSettings settings(REGPATH, QSettings::NativeFormat) ;
			settings.remove(QString("%1").arg(hashKey)) ;
		}
		QString hdSN = getHDSerialNumber('C') ;
		for( int i = m_activateInfoList.count()-1; i >= 0; i-- )
		{
			activateInfo info = m_activateInfoList[i] ;
			if( info.softwareHashKey == hashKey && hdSN == info.pcHardSN )
			{
				m_activateInfoList.takeAt(i) ;
			}
		}
		ui.expireDate->setDate(QDate::currentDate().addMonths(2)) ;
	}
	else
	{
		QMessageBox::information(NULL,"Error","Config file does n't exist.") ;
		return ;
	}
	ui.tb_active->setVisible(true) ;
	ui.tb_deactive->setVisible(false) ;
	saveActivateInfo() ;
	return ;
	/*
	QString FreeTrialStartsOn("i am tom");
	SimpleCrypt processSimpleCrypt(11111);
	SimpleCrypt processSimpleCrypt(89473829);
	QString encrypt = processSimpleCrypt.encryptToString(FreeTrialStartsOn);
	QMessageBox::information(NULL,"",encrypt) ;
	QString decrypt = processSimpleCrypt.decryptToString(encrypt);
	QMessageBox::information(NULL,"",decrypt) ;
	*/
}

void MainWindow::saveActivateInfo()
{
	QFile file("res.dat") ;
	QDataStream out(&file) ;
	file.open(QIODevice::WriteOnly) ;
	int cnt = m_activateInfoList.count() ;
	out << cnt ;
	out << COMPILERVERSION ;
	for( int i = 0; i < cnt; i++ )
	{
		activateInfo info = m_activateInfoList[i] ;
		out << info.softwareHashKey << info.pcHardSN << info.pcModelName 
			<< info.pcProductName << info.activateDate << info.expireDate ;
	}
	file.close() ;
}

bool MainWindow::checkUSBDevice()
{
	return true ;
}

void MainWindow::onChangeInstallPath()
{
	QString path = QFileDialog::getExistingDirectory(this, tr("Open Directory"));
	if( path.length() == 0 ) return ;
	//	ui.lbl_tar->setText(path) ;
	m_installPath = path ;
	ui.installPath->setText(path) ;
}


void MainWindow::copyPath(QString src, QString dst)
{
	QDir dir(src);
	if (! dir.exists())
		return;

	foreach (QString d, dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
		QString dst_path = dst + QDir::separator() + d;
		dir.mkpath(dst_path);
		copyPath(src+ QDir::separator() + d, dst_path);
	}

	foreach (QString f, dir.entryList(QDir::Files)) {
		QFile::copy(src + QDir::separator() + f, dst + QDir::separator() + f);
		ui.progressBar->setValue(m_copiedFileCount++) ;
	}
}

void MainWindow::onInstallPathChanged(QString path)
{
	m_installPath = path ;
}

bool MainWindow::canActivate( qint64 hashKey )
{
	int cnt = m_activateInfoList.count() ;
	QList<int> prevList ;
	for( int i = 0; i < cnt; i++ )
	{
//		QMessageBox::information(NULL,"", QString("%1 %2").arg(m_activateInfoList[i].softwareHashKey).arg(hashKey)) ;
		if( m_activateInfoList[i].softwareHashKey == hashKey ) prevList << i ;
	}
	if( prevList.count() >= 1 )
	{
		QString msg = "Already activated." ;
		for( int i = 0; i < prevList.count() ; i++ )
		{
			activateInfo info = m_activateInfoList[prevList[i]] ;
			msg.append("\n") ;
			msg.append(info.pcModelName+" "+info.pcProductName) ;
		}
		QMessageBox::information(NULL,"Warning", msg ) ;
		return false ;
	}
	return true ;
}

