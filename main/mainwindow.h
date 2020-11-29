#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QMediaPlayer>
#include <QBasicTimer>
#include "common.h"

class MainWindow : public QMainWindow
{
	Q_OBJECT
public:
    MainWindow(QWidget *parent = 0, Qt::WindowFlags flags = 0);
public slots:
	void onCurrentIndexChanged( int id ) ;
	void onActive() ;
	void onDeactive() ;
	void onChangeInstallPath() ;
	void onInstallPathChanged(QString path) ;
private:
	Ui::MainWindow ui ;
	bool canActivate( qint64 hashKey ) ;
	void initUI() ;
	void initConnection() ;
	void loadPrograms() ;
	bool checkUSBDevice() ;
	void saveActivateInfo() ;
	void copyPath(QString src, QString dst) ;
	QString m_installPath ;
	int m_totFileCount, m_copiedFileCount ;
	QString m_MACAddress ;
	QList<activateInfo> m_activateInfoList ;
};



#endif
