#ifndef COMMON_H
#define COMMON_H

#include <QMessageBox>
#include <QString>
#include <QObject>
#include <QTime>
#include <QPixmap>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <windows.h>
#include <ioapiset.h>
#include <iostream>
#include <string>
#include <QSettings>
#include "simplecrypt.h"

using namespace std ;

#define REGPATH "HKEY_CURRENT_USER\\Software\\TomSoftware"
#define REGSYSPATH "HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\SystemInformation"

QString  createUuid() ;
QString createSimpleUuid() ;
QString squeezeString( QString macAddress, QDate expireDate, qint64 hashKey ) ;
int fileCountInDir(QString path) ;
QString getHDSerialNumber( char d ) ;
QString getPCModelName() ;
QString getPCProductName() ;

struct activateInfo
{
	qint64 softwareHashKey ;
	QString pcHardSN ;
	QString pcModelName ;
	QString pcProductName ;
	QDate activateDate, expireDate ;
};

#endif // COMMON_H
