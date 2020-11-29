#include "common.h"
#include <QUuid>
#include <QProcess>

QString createUuid()
{
	return QUuid::createUuid().toString() ;
}

QString createSimpleUuid()
{
	QString ret = createUuid() ;
	ret.chop(1) ;
	ret = ret.right(ret.length()-1) ;
	return ret ;
}

int fileCountInDir(QString path)
{
	QDir dir( path );

	dir.setFilter( QDir::AllEntries | QDir::NoDotAndDotDot );
	int total_files = dir.count();
	return total_files ;
}

QString squeezeString( QString macAddress, QDate expireDate, qint64 hashKey )
{
	QString plain = QString("%1::%2::%3").arg(macAddress).arg(expireDate.toString("dd.MM.yyyy")).arg(hashKey) ;
	SimpleCrypt processSimpleCrypt(hashKey);
	return processSimpleCrypt.encryptToString(plain);
}


QString getHDSerialNumber( char d ) {
	//	QMessageBox::information(NULL,"",getSerialNumberOfUSB('E')) ;
	string strSerialNumber ;
	char cmd[5] ;
	cmd[0] = d, cmd[1] = ':', cmd[2] = 0 ;
	char text[100] = "\\\\.\\" ;
	strcat(text,cmd) ;
	QString str = text ;
	//	QMessageBox::information(NULL,"Running Drive", str) ;
	wchar_t wtext[30];
	mbstowcs(wtext, text, strlen(text)+1);//Plus null
	LPCWSTR sDriveName = wtext ;


	HANDLE hVolume = CreateFileW(
		sDriveName,                            // lpFileName
		0,                          // dwDesiredAccess
		FILE_SHARE_READ || FILE_SHARE_WRITE,   // dwShareMode
		NULL,                                  // lpSecurityAttributes
		OPEN_EXISTING,                         // dwCreationDisposition
		0,                 // dwFlagsAndAttributes
		NULL                                   // hTemplateFile
		);
	if (hVolume == INVALID_HANDLE_VALUE) {
		// print GetLastError()
		//		QMessageBox::information(NULL,"GET LAST ERROR", str) ;
		return "";
	}

	// set the STORAGE_PROPERTY_QUERY input
	STORAGE_PROPERTY_QUERY PropertyQuery;
	ZeroMemory(&PropertyQuery, sizeof(STORAGE_PROPERTY_QUERY));
	PropertyQuery.PropertyId = StorageDeviceProperty;
	PropertyQuery.QueryType = PropertyStandardQuery;

	// first call to DeviceIocontrol to get the size of the output
	STORAGE_DEVICE_DESCRIPTOR  DeviceDescriptor = { 0 };
	DWORD nBytesDeviceDescriptor = 0;
	if ( !DeviceIoControl(
		hVolume,                              // hDevice
		IOCTL_STORAGE_QUERY_PROPERTY,         // dwIoControlCode
		&PropertyQuery,                       // lpInBuffer
		sizeof(STORAGE_PROPERTY_QUERY),       // nInBufferSize
		&DeviceDescriptor,                    // lpOutBuffer
		sizeof(STORAGE_DESCRIPTOR_HEADER),    // nOutBufferSize
		&nBytesDeviceDescriptor,              // lpBytesReturned
		NULL                                  // lpOverlapped
		)) {
			// print GetLastError()
			CloseHandle(hVolume);
			//			QMessageBox::information(NULL,"","GET LAST ERROR1") ;
			return "" ;
			//			return FALSE;
	}

	// allocate the output
	const DWORD dwOutBufferSize = DeviceDescriptor.Size;
	char* pOutBuffer = new char[dwOutBufferSize];
	ZeroMemory(pOutBuffer, dwOutBufferSize);
	STORAGE_DEVICE_DESCRIPTOR* pDeviceDescriptor = reinterpret_cast<STORAGE_DEVICE_DESCRIPTOR*>(pOutBuffer);

	// second call to DeviceIocontrol to get the actual output STORAGE_DEVICE_DESCRIPTOR
	if (!DeviceIoControl(
		hVolume,
		IOCTL_STORAGE_QUERY_PROPERTY,
		&PropertyQuery,
		sizeof(PropertyQuery),
		pDeviceDescriptor,
		dwOutBufferSize,
		&nBytesDeviceDescriptor,
		NULL
		)) {
			// print GetLastError()
			delete[] pOutBuffer;
			CloseHandle(hVolume);
			//			QMessageBox::information(NULL,"","GET LAST ERROR3") ;
			//			return FALSE;
			return "" ;
	}

	const DWORD nSerialNumberOffset = pDeviceDescriptor->SerialNumberOffset;
	if (nSerialNumberOffset == 0) {
		// print GetLastError()
		delete[] pOutBuffer;
		CloseHandle(hVolume);
		//		return FALSE;
		//		QMessageBox::information(NULL,"","GET LAST ERROR4") ;
		return "" ;
	}
	strSerialNumber = static_cast<std::string>(pOutBuffer + nSerialNumberOffset);

	delete[] pOutBuffer;
	CloseHandle(hVolume);
	return QString::fromStdString(strSerialNumber) ;
	//	return (strSerialNumber.empty()) ? FALSE : TRUE;
}

QString getPCModelName()
{
	QSettings settings(REGSYSPATH, QSettings::NativeFormat) ;	
	return settings.value("SystemManufacturer").toString() ;
}

QString getPCProductName()
{
	QSettings settings(REGSYSPATH, QSettings::NativeFormat) ;	
	return settings.value("SystemProductName").toString() ;
}

