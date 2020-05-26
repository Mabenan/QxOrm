/****************************************************************************
**
** https://www.qxorm.com/
** Copyright (C) 2013 Lionel Marty (contact@qxorm.com)
**
** This file is part of the QxOrm library
**
** This software is provided 'as-is', without any express or implied
** warranty. In no event will the authors be held liable for any
** damages arising from the use of this software
**
** Commercial Usage
** Licensees holding valid commercial QxOrm licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Lionel Marty
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file 'license.gpl3.txt' included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met : http://www.gnu.org/copyleft/gpl.html
**
** If you are unsure which license is appropriate for your use, or
** if you have questions regarding the use of this file, please contact :
** contact@qxorm.com
**
****************************************************************************/

#include <QxPrecompiled.h>

#include <QxDao/QxSqlDatabase.h>

#include <QxDao/QxSqlGenerator/QxSqlGenerator.h>

#ifdef _QX_ENABLE_MONGODB
#include <QxDao/QxMongoDB/QxMongoDB_Helper.h>
#endif // _QX_ENABLE_MONGODB

#include <QxMemLeak/mem_leak.h>

#define QX_CONSTRUCT_QX_SQL_DATABASE()                                         \
  m_oDbMutex(QMutex::Recursive), m_iPort(-1), m_bTraceSqlQuery(true),          \
      m_bTraceSqlRecord(false), m_bTraceSqlBoundValues(false),                 \
      m_bTraceSqlBoundValuesOnError(true),                                     \
      m_ePlaceHolderStyle(QxSqlDatabase::ph_style_2_point_name),               \
      m_bSessionThrowable(false), m_bSessionAutoTransaction(true),             \
      m_bValidatorThrowable(false), m_bAutoReplaceSqlAliasIntoQuery(true),     \
      m_bVerifyOffsetRelation(false),                                          \
      m_bAddAutoIncrementIdToUpdateQuery(true),                                \
      m_bForceParentIdToAllChildren(false),                                    \
      m_bAddSqlSquareBracketsForTableName(false),                              \
      m_bAddSqlSquareBracketsForColumnName(false),                             \
      m_bFormatSqlQueryBeforeLogging(false),                                   \
      m_iTraceSqlOnlySlowQueriesDatabase(-1),                                  \
      m_iTraceSqlOnlySlowQueriesTotal(-1), m_bDisplayTimerDetails(false)

QX_DLL_EXPORT_QX_SINGLETON_CPP(qx::QxSqlDatabase)

namespace qx {

struct QxSqlDatabase::QxSqlDatabaseImpl {

  QxSqlDatabase
      *m_pParent; //!< Parent instance of the private implementation idiom
  QHash<Qt::HANDLE, QString>
      m_lstDbByThread;   //!< Collection of databases connexions by thread id
  QMutex m_oDbMutex;     //!< Mutex => 'QxSqlDatabase' is thread-safe
  QString m_sDriverName; //!< Driver name to connect to database
  QString m_sConnectOptions;          //!< Connect options to database
  QString m_sDatabaseName;            //!< Database name
  QString m_sUserName;                //!< Connection's user name
  QString m_sPassword;                //!< Connection's password
  QString m_sHostName;                //!< Connection's host name
  int m_iPort;                        //!< Connection's port number
  bool m_bTraceSqlQuery;              //!< Trace each sql query executed
  bool m_bTraceSqlRecord;             //!< Trace each sql record
  bool m_bTraceSqlBoundValues;        //!< Trace sql bound values
  bool m_bTraceSqlBoundValuesOnError; //!< Trace sql bound values (only when an
                                      //!< error occurred)
  QxSqlDatabase::ph_style
      m_ePlaceHolderStyle;  //!< Place holder style to build sql query
  bool m_bSessionThrowable; //!< An exception of type qx::dao::sql_error is
                            //!< thrown when a SQL error is appended to
                            //!< qx::QxSession object
  bool m_bSessionAutoTransaction; //!< A transaction is automatically beginned
                                  //!< when a qx::QxSession object is
                                  //!< instantiated
  bool
      m_bValidatorThrowable; //!< An exception of type qx::validator_error is
                             //!< thrown when invalid values are detected
                             //!< inserting or updating an element into database
  qx::dao::detail::IxSqlGenerator_ptr
      m_pSqlGenerator; //!< SQL generator to build SQL query specific for each
                       //!< database
  bool m_bAutoReplaceSqlAliasIntoQuery; //!< Replace all sql alias into sql
                                        //!< query automatically
  bool m_bVerifyOffsetRelation; //!< Only for debug purpose : assert if invalid
                                //!< offset detected fetching a relation
  bool m_bAddAutoIncrementIdToUpdateQuery; //!< For Microsoft SqlServer database
                                           //!< compatibility : add or not
                                           //!< auto-increment id to SQL update
                                           //!< query
  bool m_bForceParentIdToAllChildren; //!< Force parent id to all children (for
                                      //!< 1-n relationship for example)
  QxSqlDatabase::type_fct_db_open
      m_fctDatabaseOpen; //!< Callback function called when a new database
                         //!< connection is opened (can be used for example with
                         //!< SQLite database to define some PRAGMAs before
                         //!< executing any SQL query)
  bool
      m_bAddSqlSquareBracketsForTableName; //!< Add square brackets [] for table
                                           //!< name in SQL queries (to support
                                           //!< specific database keywords)
  bool m_bAddSqlSquareBracketsForColumnName; //!< Add square brackets [] for
                                             //!< column name in SQL queries (to
                                             //!< support specific database
                                             //!< keywords)
  bool m_bFormatSqlQueryBeforeLogging; //!< Format SQL query (pretty-printing)
                                       //!< before logging it (can be customized
                                       //!< creating a
                                       //!< qx::dao::detail::IxSqlGenerator
                                       //!< sub-class)
  QStringList
      m_lstSqlDelimiterForTableName; //!< Add delimiter characters for table
                                     //!< name in SQL queries (to support
                                     //!< specific database keywords) : for
                                     //!< example, use ` for MySQL database
  QStringList
      m_lstSqlDelimiterForColumnName; //!< Add delimiter characters for column
                                      //!< name in SQL queries (to support
                                      //!< specific database keywords) : for
                                      //!< example, use ` for MySQL database
  int m_iTraceSqlOnlySlowQueriesDatabase; //!< Trace only slow sql queries
                                          //!< (database execution time only, in
                                          //!< milliseconds)
  int m_iTraceSqlOnlySlowQueriesTotal;    //!< Trace only slow sql queries
                                          //!< (database execution time + C++
                                          //!< qx::dao execution time, in
                                          //!< milliseconds)
  bool m_bDisplayTimerDetails; //!< Display in logs all timers details (exec(),
                               //!< next(), prepare(), open(), etc...)

  QHash<QPair<Qt::HANDLE, QString>, QVariant>
      m_lstSettingsByThread; //!< List of settings per thread (override global
                             //!< settings) : can be useful to use different
                             //!< databases per thread (see
                             //!< bJustForCurrentThread parameter in all
                             //!< setXXXX() methods)
  QHash<Qt::HANDLE, qx::dao::detail::IxSqlGenerator_ptr>
      m_lstGeneratorByThread; //!< List of SQL generator per thread (override
                              //!< global settings) : can be useful to use
                              //!< different databases per thread (see
                              //!< bJustForCurrentThread parameter in
                              //!< setSqlGenerator() method)
  QHash<QPair<QString, QString>, QVariant>
      m_lstSettingsByDatabase; //!< List of settings per database (override
                               //!< global settings and thread settings) : can
                               //!< be useful to use different databases in a
                               //!< same thread (see pJustForThisDatabase
                               //!< parameter in all setXXXX() method)
  QHash<QString, qx::dao::detail::IxSqlGenerator_ptr>
      m_lstGeneratorByDatabase; //!< List of SQL generator per database
                                //!< (override global settings and thread
                                //!< settings) : can be useful to use different
                                //!< databases in a same thread (see
                                //!< pJustForThisDatabase parameter in
                                //!< setSqlGenerator() method)
  QHash<Qt::HANDLE, QString>
      m_lstCurrDatabaseKeyByThread; //!< Current database key (key = driverName
                                    //!< + hostName + databaseName +
                                    //!< connectionName) used per thread : this
                                    //!< list is filled by each
                                    //!< qx::dao::detail::IxDao_Helper instance
                                    //!< using RAII

  QxSqlDatabaseImpl(QxSqlDatabase *p)
      : m_pParent(p), QX_CONSTRUCT_QX_SQL_DATABASE() {
    ;
  }
  ~QxSqlDatabaseImpl() { ; }

  QSqlDatabase getDatabaseByCurrThreadId(QSqlError &dbError);
  QSqlDatabase createDatabase(QSqlError &dbError);

  void displayLastError(const QSqlDatabase &db, const QString &sDesc) const;
  QString formatLastError(const QSqlDatabase &db) const;

  bool isValid() const {
    return (m_pParent
                ? (!m_pParent->getDriverName().isEmpty() &&
                   !m_pParent->getDatabaseName().isEmpty())
                : (!m_sDriverName.isEmpty() && !m_sDatabaseName.isEmpty()));
  }
  QString computeDatabaseKey(QSqlDatabase *p) const {
    return (p ? (p->driverName() + p->hostName() + p->databaseName())
              : (m_sDriverName + m_sHostName + m_sDatabaseName));
  }

  QVariant getSetting(const QString &key) const {
    if (m_lstSettingsByDatabase.count() > 0) {
      Qt::HANDLE lCurrThreadId = QThread::currentThreadId();
      QString sDatabaseKey =
          (m_lstCurrDatabaseKeyByThread.contains(lCurrThreadId)
               ? m_lstCurrDatabaseKeyByThread.value(lCurrThreadId)
               : QString());
      QPair<QString, QString> pairDatabase = qMakePair(sDatabaseKey, key);
      if (m_lstSettingsByDatabase.contains(pairDatabase)) {
        return m_lstSettingsByDatabase.value(pairDatabase);
      }
    }
    if (m_lstSettingsByThread.count() > 0) {
      Qt::HANDLE lCurrThreadId = QThread::currentThreadId();
      QPair<Qt::HANDLE, QString> pairThread = qMakePair(lCurrThreadId, key);
      if (m_lstSettingsByThread.contains(pairThread)) {
        return m_lstSettingsByThread.value(pairThread);
      }
    }
    return QVariant();
  }

  bool setSetting(const QString &key, const QVariant &val,
                  bool bJustForCurrentThread,
                  QSqlDatabase *pJustForThisDatabase) {
    bool bUpdateGlobal = true;
    if (bJustForCurrentThread) {
      QMutexLocker locker(&m_oDbMutex);
      Qt::HANDLE lCurrThreadId = QThread::currentThreadId();
      QPair<Qt::HANDLE, QString> pairThread = qMakePair(lCurrThreadId, key);
      m_lstSettingsByThread.insert(pairThread, val);
      bUpdateGlobal = false;
    }
    if (pJustForThisDatabase != NULL) {
      QMutexLocker locker(&m_oDbMutex);
      QString sDatabaseKey = computeDatabaseKey(pJustForThisDatabase);
      if (sDatabaseKey.isEmpty()) {
        qDebug("[QxOrm] qx::QxSqlDatabase::setSetting() : database parameters "
               "are empty ==> cannot add setting database '%s'",
               qPrintable(key));
        qAssert(false);
        return false;
      }
      QPair<QString, QString> pairDatabase = qMakePair(sDatabaseKey, key);
      m_lstSettingsByDatabase.insert(pairDatabase, val);
      bUpdateGlobal = false;
    }
    return bUpdateGlobal;
  }
};

QxSqlDatabase::QxSqlDatabase()
    : QxSingleton<QxSqlDatabase>(QStringLiteral("qx::QxSqlDatabase"))
    , m_pImpl(new QxSqlDatabaseImpl(this))
{
    ;
}

QxSqlDatabase::~QxSqlDatabase() { ; }

QString QxSqlDatabase::getDriverName() const {
  if ((m_pImpl->m_lstSettingsByThread.count() <= 0) &&
      (m_pImpl->m_lstSettingsByDatabase.count() <= 0)) {
    return m_pImpl->m_sDriverName;
  }
  QVariant setting = m_pImpl->getSetting(QStringLiteral("DriverName"));
  if (!setting.isNull()) {
    return setting.toString();
  }
  return m_pImpl->m_sDriverName;
}

QString QxSqlDatabase::getConnectOptions() const {
  if ((m_pImpl->m_lstSettingsByThread.count() <= 0) &&
      (m_pImpl->m_lstSettingsByDatabase.count() <= 0)) {
    return m_pImpl->m_sConnectOptions;
  }
  QVariant setting = m_pImpl->getSetting(QStringLiteral("ConnectOptions"));
  if (!setting.isNull()) {
    return setting.toString();
  }
  return m_pImpl->m_sConnectOptions;
}

QString QxSqlDatabase::getDatabaseName() const {
  if ((m_pImpl->m_lstSettingsByThread.count() <= 0) &&
      (m_pImpl->m_lstSettingsByDatabase.count() <= 0)) {
    return m_pImpl->m_sDatabaseName;
  }
  QVariant setting = m_pImpl->getSetting(QStringLiteral("DatabaseName"));
  if (!setting.isNull()) {
    return setting.toString();
  }
  return m_pImpl->m_sDatabaseName;
}

QString QxSqlDatabase::getUserName() const {
  if ((m_pImpl->m_lstSettingsByThread.count() <= 0) &&
      (m_pImpl->m_lstSettingsByDatabase.count() <= 0)) {
    return m_pImpl->m_sUserName;
  }
  QVariant setting = m_pImpl->getSetting(QStringLiteral("UserName"));
  if (!setting.isNull()) {
    return setting.toString();
  }
  return m_pImpl->m_sUserName;
}

QString QxSqlDatabase::getPassword() const {
  if ((m_pImpl->m_lstSettingsByThread.count() <= 0) &&
      (m_pImpl->m_lstSettingsByDatabase.count() <= 0)) {
    return m_pImpl->m_sPassword;
  }
  QVariant setting = m_pImpl->getSetting(QStringLiteral("Password"));
  if (!setting.isNull()) {
    return setting.toString();
  }
  return m_pImpl->m_sPassword;
}

QString QxSqlDatabase::getHostName() const {
  if ((m_pImpl->m_lstSettingsByThread.count() <= 0) &&
      (m_pImpl->m_lstSettingsByDatabase.count() <= 0)) {
    return m_pImpl->m_sHostName;
  }
  QVariant setting = m_pImpl->getSetting(QStringLiteral("HostName"));
  if (!setting.isNull()) {
    return setting.toString();
  }
  return m_pImpl->m_sHostName;
}

int QxSqlDatabase::getPort() const {
  if ((m_pImpl->m_lstSettingsByThread.count() <= 0) &&
      (m_pImpl->m_lstSettingsByDatabase.count() <= 0)) {
    return m_pImpl->m_iPort;
  }
  QVariant setting = m_pImpl->getSetting(QStringLiteral("Port"));
  if (!setting.isNull()) {
    return setting.toInt();
  }
  return m_pImpl->m_iPort;
}

bool QxSqlDatabase::getTraceSqlQuery() const {
  if ((m_pImpl->m_lstSettingsByThread.count() <= 0) &&
      (m_pImpl->m_lstSettingsByDatabase.count() <= 0)) {
    return m_pImpl->m_bTraceSqlQuery;
  }
  QVariant setting = m_pImpl->getSetting(QStringLiteral("TraceSqlQuery"));
  if (!setting.isNull()) {
    return setting.toBool();
  }
  return m_pImpl->m_bTraceSqlQuery;
}

bool QxSqlDatabase::getTraceSqlRecord() const {
  if ((m_pImpl->m_lstSettingsByThread.count() <= 0) &&
      (m_pImpl->m_lstSettingsByDatabase.count() <= 0)) {
    return m_pImpl->m_bTraceSqlRecord;
  }
  QVariant setting = m_pImpl->getSetting(QStringLiteral("TraceSqlRecord"));
  if (!setting.isNull()) {
    return setting.toBool();
  }
  return m_pImpl->m_bTraceSqlRecord;
}

bool QxSqlDatabase::getTraceSqlBoundValues() const {
  if ((m_pImpl->m_lstSettingsByThread.count() <= 0) &&
      (m_pImpl->m_lstSettingsByDatabase.count() <= 0)) {
    return m_pImpl->m_bTraceSqlBoundValues;
  }
  QVariant setting = m_pImpl->getSetting(QStringLiteral("TraceSqlBoundValues"));
  if (!setting.isNull()) {
    return setting.toBool();
  }
  return m_pImpl->m_bTraceSqlBoundValues;
}

bool QxSqlDatabase::getTraceSqlBoundValuesOnError() const {
  if ((m_pImpl->m_lstSettingsByThread.count() <= 0) &&
      (m_pImpl->m_lstSettingsByDatabase.count() <= 0)) {
    return m_pImpl->m_bTraceSqlBoundValuesOnError;
  }
  QVariant setting = m_pImpl->getSetting(QStringLiteral("TraceSqlBoundValuesOnError"));
  if (!setting.isNull()) {
    return setting.toBool();
  }
  return m_pImpl->m_bTraceSqlBoundValuesOnError;
}

QxSqlDatabase::ph_style QxSqlDatabase::getSqlPlaceHolderStyle() const {
  if ((m_pImpl->m_lstSettingsByThread.count() <= 0) &&
      (m_pImpl->m_lstSettingsByDatabase.count() <= 0)) {
    return m_pImpl->m_ePlaceHolderStyle;
  }
  QVariant setting = m_pImpl->getSetting(QStringLiteral("SqlPlaceHolderStyle"));
  if (!setting.isNull()) {
    return static_cast<QxSqlDatabase::ph_style>(setting.toInt());
  }
  return m_pImpl->m_ePlaceHolderStyle;
}

bool QxSqlDatabase::getSessionThrowable() const {
  if ((m_pImpl->m_lstSettingsByThread.count() <= 0) &&
      (m_pImpl->m_lstSettingsByDatabase.count() <= 0)) {
    return m_pImpl->m_bSessionThrowable;
  }
  QVariant setting = m_pImpl->getSetting(QStringLiteral("SessionThrowable"));
  if (!setting.isNull()) {
    return setting.toBool();
  }
  return m_pImpl->m_bSessionThrowable;
}

bool QxSqlDatabase::getSessionAutoTransaction() const {
  if ((m_pImpl->m_lstSettingsByThread.count() <= 0) &&
      (m_pImpl->m_lstSettingsByDatabase.count() <= 0)) {
    return m_pImpl->m_bSessionAutoTransaction;
  }
  QVariant setting = m_pImpl->getSetting(QStringLiteral("SessionAutoTransaction"));
  if (!setting.isNull()) {
    return setting.toBool();
  }
  return m_pImpl->m_bSessionAutoTransaction;
}

bool QxSqlDatabase::getValidatorThrowable() const {
  if ((m_pImpl->m_lstSettingsByThread.count() <= 0) &&
      (m_pImpl->m_lstSettingsByDatabase.count() <= 0)) {
    return m_pImpl->m_bValidatorThrowable;
  }
  QVariant setting = m_pImpl->getSetting(QStringLiteral("ValidatorThrowable"));
  if (!setting.isNull()) {
    return setting.toBool();
  }
  return m_pImpl->m_bValidatorThrowable;
}

bool QxSqlDatabase::getAutoReplaceSqlAliasIntoQuery() const {
  if ((m_pImpl->m_lstSettingsByThread.count() <= 0) &&
      (m_pImpl->m_lstSettingsByDatabase.count() <= 0)) {
    return m_pImpl->m_bAutoReplaceSqlAliasIntoQuery;
  }
  QVariant setting = m_pImpl->getSetting(QStringLiteral("AutoReplaceSqlAliasIntoQuery"));
  if (!setting.isNull()) {
    return setting.toBool();
  }
  return m_pImpl->m_bAutoReplaceSqlAliasIntoQuery;
}

bool QxSqlDatabase::getVerifyOffsetRelation() const {
  if ((m_pImpl->m_lstSettingsByThread.count() <= 0) &&
      (m_pImpl->m_lstSettingsByDatabase.count() <= 0)) {
    return m_pImpl->m_bVerifyOffsetRelation;
  }
  QVariant setting = m_pImpl->getSetting(QStringLiteral("VerifyOffsetRelation"));
  if (!setting.isNull()) {
    return setting.toBool();
  }
  return m_pImpl->m_bVerifyOffsetRelation;
}

bool QxSqlDatabase::getAddAutoIncrementIdToUpdateQuery() const {
  if ((m_pImpl->m_lstSettingsByThread.count() <= 0) &&
      (m_pImpl->m_lstSettingsByDatabase.count() <= 0)) {
    return m_pImpl->m_bAddAutoIncrementIdToUpdateQuery;
  }
  QVariant setting = m_pImpl->getSetting(QStringLiteral("AddAutoIncrementIdToUpdateQuery"));
  if (!setting.isNull()) {
    return setting.toBool();
  }
  return m_pImpl->m_bAddAutoIncrementIdToUpdateQuery;
}

bool QxSqlDatabase::getForceParentIdToAllChildren() const {
  if ((m_pImpl->m_lstSettingsByThread.count() <= 0) &&
      (m_pImpl->m_lstSettingsByDatabase.count() <= 0)) {
    return m_pImpl->m_bForceParentIdToAllChildren;
  }
  QVariant setting = m_pImpl->getSetting(QStringLiteral("ForceParentIdToAllChildren"));
  if (!setting.isNull()) {
    return setting.toBool();
  }
  return m_pImpl->m_bForceParentIdToAllChildren;
}

QxSqlDatabase::type_fct_db_open QxSqlDatabase::getFctDatabaseOpen() const {
  return m_pImpl->m_fctDatabaseOpen;
}

bool QxSqlDatabase::getAddSqlSquareBracketsForTableName() const {
  if ((m_pImpl->m_lstSettingsByThread.count() <= 0) &&
      (m_pImpl->m_lstSettingsByDatabase.count() <= 0)) {
    return m_pImpl->m_bAddSqlSquareBracketsForTableName;
  }
  QVariant setting = m_pImpl->getSetting(QStringLiteral("AddSqlSquareBracketsForTableName"));
  if (!setting.isNull()) {
    return setting.toBool();
  }
  return m_pImpl->m_bAddSqlSquareBracketsForTableName;
}

bool QxSqlDatabase::getAddSqlSquareBracketsForColumnName() const {
  if ((m_pImpl->m_lstSettingsByThread.count() <= 0) &&
      (m_pImpl->m_lstSettingsByDatabase.count() <= 0)) {
    return m_pImpl->m_bAddSqlSquareBracketsForColumnName;
  }
  QVariant setting = m_pImpl->getSetting(QStringLiteral("AddSqlSquareBracketsForColumnName"));
  if (!setting.isNull()) {
    return setting.toBool();
  }
  return m_pImpl->m_bAddSqlSquareBracketsForColumnName;
}

bool QxSqlDatabase::getFormatSqlQueryBeforeLogging() const {
  if ((m_pImpl->m_lstSettingsByThread.count() <= 0) &&
      (m_pImpl->m_lstSettingsByDatabase.count() <= 0)) {
    return m_pImpl->m_bFormatSqlQueryBeforeLogging;
  }
  QVariant setting = m_pImpl->getSetting(QStringLiteral("FormatSqlQueryBeforeLogging"));
  if (!setting.isNull()) {
    return setting.toBool();
  }
  return m_pImpl->m_bFormatSqlQueryBeforeLogging;
}

QStringList QxSqlDatabase::getSqlDelimiterForTableName() const {
  if ((m_pImpl->m_lstSettingsByThread.count() <= 0) &&
      (m_pImpl->m_lstSettingsByDatabase.count() <= 0)) {
    return m_pImpl->m_lstSqlDelimiterForTableName;
  }
  QVariant setting = m_pImpl->getSetting(QStringLiteral("SqlDelimiterForTableName"));
  if (!setting.isNull()) {
    return setting.toStringList();
  }
  return m_pImpl->m_lstSqlDelimiterForTableName;
}

QStringList QxSqlDatabase::getSqlDelimiterForColumnName() const {
  if ((m_pImpl->m_lstSettingsByThread.count() <= 0) &&
      (m_pImpl->m_lstSettingsByDatabase.count() <= 0)) {
    return m_pImpl->m_lstSqlDelimiterForColumnName;
  }
  QVariant setting = m_pImpl->getSetting(QStringLiteral("SqlDelimiterForColumnName"));
  if (!setting.isNull()) {
    return setting.toStringList();
  }
  return m_pImpl->m_lstSqlDelimiterForColumnName;
}

int QxSqlDatabase::getTraceSqlOnlySlowQueriesDatabase() const {
  if ((m_pImpl->m_lstSettingsByThread.count() <= 0) &&
      (m_pImpl->m_lstSettingsByDatabase.count() <= 0)) {
    return m_pImpl->m_iTraceSqlOnlySlowQueriesDatabase;
  }
  QVariant setting = m_pImpl->getSetting(QStringLiteral("TraceSqlOnlySlowQueriesDatabase"));
  if (!setting.isNull()) {
    return setting.toInt();
  }
  return m_pImpl->m_iTraceSqlOnlySlowQueriesDatabase;
}

int QxSqlDatabase::getTraceSqlOnlySlowQueriesTotal() const {
  if ((m_pImpl->m_lstSettingsByThread.count() <= 0) &&
      (m_pImpl->m_lstSettingsByDatabase.count() <= 0)) {
    return m_pImpl->m_iTraceSqlOnlySlowQueriesTotal;
  }
  QVariant setting = m_pImpl->getSetting(QStringLiteral("TraceSqlOnlySlowQueriesTotal"));
  if (!setting.isNull()) {
    return setting.toInt();
  }
  return m_pImpl->m_iTraceSqlOnlySlowQueriesTotal;
}

bool QxSqlDatabase::getDisplayTimerDetails() const {
  if ((m_pImpl->m_lstSettingsByThread.count() <= 0) &&
      (m_pImpl->m_lstSettingsByDatabase.count() <= 0)) {
    return m_pImpl->m_bDisplayTimerDetails;
  }
  QVariant setting = m_pImpl->getSetting(QStringLiteral("DisplayTimerDetails"));
  if (!setting.isNull()) {
    return setting.toBool();
  }
  return m_pImpl->m_bDisplayTimerDetails;
}

void QxSqlDatabase::setDriverName(
    const QString &s, bool bJustForCurrentThread /* = false */,
    QSqlDatabase *pJustForThisDatabase /* = NULL */) {
    bool bUpdateGlobal = m_pImpl->setSetting(QStringLiteral("DriverName"),
                                             s,
                                             bJustForCurrentThread,
                                             pJustForThisDatabase);
    if (bUpdateGlobal) {
        m_pImpl->m_sDriverName = s;
  }
  getSqlGenerator();
}

void QxSqlDatabase::setConnectOptions(
    const QString &s, bool bJustForCurrentThread /* = false */,
    QSqlDatabase *pJustForThisDatabase /* = NULL */) {
  bool bUpdateGlobal = m_pImpl->setSetting(
      QStringLiteral("ConnectOptions"), s, bJustForCurrentThread, pJustForThisDatabase);
  if (bUpdateGlobal) {
    m_pImpl->m_sConnectOptions = s;
  }
}

void QxSqlDatabase::setDatabaseName(
    const QString &s, bool bJustForCurrentThread /* = false */,
    QSqlDatabase *pJustForThisDatabase /* = NULL */) {
    bool bUpdateGlobal = m_pImpl->setSetting(QStringLiteral("DatabaseName"),
                                             s,
                                             bJustForCurrentThread,
                                             pJustForThisDatabase);
    if (bUpdateGlobal) {
        m_pImpl->m_sDatabaseName = s;
  }
}

void QxSqlDatabase::setUserName(
    const QString &s, bool bJustForCurrentThread /* = false */,
    QSqlDatabase *pJustForThisDatabase /* = NULL */) {
  bool bUpdateGlobal = m_pImpl->setSetting(QStringLiteral("UserName"), s, bJustForCurrentThread,
                                           pJustForThisDatabase);
  if (bUpdateGlobal) {
    m_pImpl->m_sUserName = s;
  }
}

void QxSqlDatabase::setPassword(
    const QString &s, bool bJustForCurrentThread /* = false */,
    QSqlDatabase *pJustForThisDatabase /* = NULL */) {
    bool bUpdateGlobal = m_pImpl->setSetting(QStringLiteral("Password"),
                                             s,
                                             bJustForCurrentThread,
                                             pJustForThisDatabase);
    if (bUpdateGlobal) {
        m_pImpl->m_sPassword = s;
  }
}

void QxSqlDatabase::setHostName(
    const QString &s, bool bJustForCurrentThread /* = false */,
    QSqlDatabase *pJustForThisDatabase /* = NULL */) {
  bool bUpdateGlobal = m_pImpl->setSetting(QStringLiteral("HostName"), s, bJustForCurrentThread,
                                           pJustForThisDatabase);
  if (bUpdateGlobal) {
    m_pImpl->m_sHostName = s;
  }
}

void QxSqlDatabase::setPort(int i, bool bJustForCurrentThread /* = false */,
                            QSqlDatabase *pJustForThisDatabase /* = NULL */) {
    bool bUpdateGlobal = m_pImpl->setSetting(QStringLiteral("Port"),
                                             i,
                                             bJustForCurrentThread,
                                             pJustForThisDatabase);
    if (bUpdateGlobal) {
        m_pImpl->m_iPort = i;
  }
}

void QxSqlDatabase::setTraceSqlQuery(
    bool b, bool bJustForCurrentThread /* = false */,
    QSqlDatabase *pJustForThisDatabase /* = NULL */) {
  bool bUpdateGlobal = m_pImpl->setSetting(
      QStringLiteral("TraceSqlQuery"), b, bJustForCurrentThread, pJustForThisDatabase);
  if (bUpdateGlobal) {
    m_pImpl->m_bTraceSqlQuery = b;
  }
}

void QxSqlDatabase::setTraceSqlRecord(
    bool b, bool bJustForCurrentThread /* = false */,
    QSqlDatabase *pJustForThisDatabase /* = NULL */) {
    bool bUpdateGlobal = m_pImpl->setSetting(QStringLiteral("TraceSqlRecord"),
                                             b,
                                             bJustForCurrentThread,
                                             pJustForThisDatabase);
    if (bUpdateGlobal) {
        m_pImpl->m_bTraceSqlRecord = b;
  }
}

void QxSqlDatabase::setTraceSqlBoundValues(
    bool b, bool bJustForCurrentThread /* = false */,
    QSqlDatabase *pJustForThisDatabase /* = NULL */) {
  bool bUpdateGlobal = m_pImpl->setSetting(
      QStringLiteral("TraceSqlBoundValues"), b, bJustForCurrentThread, pJustForThisDatabase);
  if (bUpdateGlobal) {
    m_pImpl->m_bTraceSqlBoundValues = b;
  }
}

void QxSqlDatabase::setTraceSqlBoundValuesOnError(
    bool b, bool bJustForCurrentThread /* = false */,
    QSqlDatabase *pJustForThisDatabase /* = NULL */) {
    bool bUpdateGlobal = m_pImpl->setSetting(QStringLiteral("TraceSqlBoundValuesOnError"),
                                             b,
                                             bJustForCurrentThread,
                                             pJustForThisDatabase);
    if (bUpdateGlobal) {
        m_pImpl->m_bTraceSqlBoundValuesOnError = b;
  }
}

void QxSqlDatabase::setSqlPlaceHolderStyle(
    QxSqlDatabase::ph_style e, bool bJustForCurrentThread /* = false */,
    QSqlDatabase *pJustForThisDatabase /* = NULL */) {
  int i = static_cast<int>(e);
  bool bUpdateGlobal = m_pImpl->setSetting(
      QStringLiteral("SqlPlaceHolderStyle"), i, bJustForCurrentThread, pJustForThisDatabase);
  if (bUpdateGlobal) {
    m_pImpl->m_ePlaceHolderStyle = e;
  }
}

void QxSqlDatabase::setSessionThrowable(
    bool b, bool bJustForCurrentThread /* = false */,
    QSqlDatabase *pJustForThisDatabase /* = NULL */) {
    bool bUpdateGlobal = m_pImpl->setSetting(QStringLiteral("SessionThrowable"),
                                             b,
                                             bJustForCurrentThread,
                                             pJustForThisDatabase);
    if (bUpdateGlobal) {
        m_pImpl->m_bSessionThrowable = b;
  }
}

void QxSqlDatabase::setSessionAutoTransaction(
    bool b, bool bJustForCurrentThread /* = false */,
    QSqlDatabase *pJustForThisDatabase /* = NULL */) {
  bool bUpdateGlobal = m_pImpl->setSetting(
      QStringLiteral("SessionAutoTransaction"), b, bJustForCurrentThread, pJustForThisDatabase);
  if (bUpdateGlobal) {
    m_pImpl->m_bSessionAutoTransaction = b;
  }
}

void QxSqlDatabase::setValidatorThrowable(
    bool b, bool bJustForCurrentThread /* = false */,
    QSqlDatabase *pJustForThisDatabase /* = NULL */) {
    bool bUpdateGlobal = m_pImpl->setSetting(QStringLiteral("ValidatorThrowable"),
                                             b,
                                             bJustForCurrentThread,
                                             pJustForThisDatabase);
    if (bUpdateGlobal) {
        m_pImpl->m_bValidatorThrowable = b;
  }
}

void QxSqlDatabase::setSqlGenerator(
    qx::dao::detail::IxSqlGenerator_ptr p,
    bool bJustForCurrentThread /* = false */,
    QSqlDatabase *pJustForThisDatabase /* = NULL */) {
  bool bUpdateGlobal = true;
  if (p && bJustForCurrentThread) {
    QMutexLocker locker(&m_pImpl->m_oDbMutex);
    Qt::HANDLE lCurrThreadId = QThread::currentThreadId();
    m_pImpl->m_lstGeneratorByThread.insert(lCurrThreadId, p);
    bUpdateGlobal = false;
  }
  if (p && (pJustForThisDatabase != NULL)) {
    QMutexLocker locker(&m_pImpl->m_oDbMutex);
    QString sDatabaseKey = m_pImpl->computeDatabaseKey(pJustForThisDatabase);
    if (sDatabaseKey.isEmpty()) {
      qDebug("[QxOrm] qx::QxSqlDatabase::setSqlGenerator() : database "
             "parameters are empty ==> cannot add setting database '%s'",
             "SqlGenerator");
      qAssert(false);
      return;
    }
    m_pImpl->m_lstGeneratorByDatabase.insert(sDatabaseKey, p);
    bUpdateGlobal = false;
  }

  if (bUpdateGlobal) {
    m_pImpl->m_pSqlGenerator = p;
  }
  if (p) {
    p->init();
  }
}

void QxSqlDatabase::setAutoReplaceSqlAliasIntoQuery(
    bool b, bool bJustForCurrentThread /* = false */,
    QSqlDatabase *pJustForThisDatabase /* = NULL */) {
  bool bUpdateGlobal =
      m_pImpl->setSetting(QStringLiteral("AutoReplaceSqlAliasIntoQuery"), b,
                          bJustForCurrentThread, pJustForThisDatabase);
  if (bUpdateGlobal) {
    m_pImpl->m_bAutoReplaceSqlAliasIntoQuery = b;
  }
}

void QxSqlDatabase::setVerifyOffsetRelation(
    bool b, bool bJustForCurrentThread /* = false */,
    QSqlDatabase *pJustForThisDatabase /* = NULL */) {
    bool bUpdateGlobal = m_pImpl->setSetting(QStringLiteral("VerifyOffsetRelation"),
                                             b,
                                             bJustForCurrentThread,
                                             pJustForThisDatabase);
    if (bUpdateGlobal) {
        m_pImpl->m_bVerifyOffsetRelation = b;
  }
}

void QxSqlDatabase::setAddAutoIncrementIdToUpdateQuery(
    bool b, bool bJustForCurrentThread /* = false */,
    QSqlDatabase *pJustForThisDatabase /* = NULL */) {
  bool bUpdateGlobal =
      m_pImpl->setSetting(QStringLiteral("AddAutoIncrementIdToUpdateQuery"), b,
                          bJustForCurrentThread, pJustForThisDatabase);
  if (bUpdateGlobal) {
    m_pImpl->m_bAddAutoIncrementIdToUpdateQuery = b;
  }
}

void QxSqlDatabase::setForceParentIdToAllChildren(
    bool b, bool bJustForCurrentThread /* = false */,
    QSqlDatabase *pJustForThisDatabase /* = NULL */) {
    bool bUpdateGlobal = m_pImpl->setSetting(QStringLiteral("ForceParentIdToAllChildren"),
                                             b,
                                             bJustForCurrentThread,
                                             pJustForThisDatabase);
    if (bUpdateGlobal) {
        m_pImpl->m_bForceParentIdToAllChildren = b;
  }
}

void QxSqlDatabase::setFctDatabaseOpen(const type_fct_db_open &fct, bool bJustForCurrentThread, QSqlDatabase *pJustForThisDatabase) {
  Q_UNUSED(bJustForCurrentThread);
  Q_UNUSED(pJustForThisDatabase);
  m_pImpl->m_fctDatabaseOpen = fct;
}

void QxSqlDatabase::setAddSqlSquareBracketsForTableName(
    bool b, bool bJustForCurrentThread /* = false */,
    QSqlDatabase *pJustForThisDatabase /* = NULL */) {
    bool bUpdateGlobal = m_pImpl->setSetting(QStringLiteral("AddSqlSquareBracketsForTableName"),
                                             b,
                                             bJustForCurrentThread,
                                             pJustForThisDatabase);
    if (bUpdateGlobal) {
        m_pImpl->m_bAddSqlSquareBracketsForTableName = b;
  }
}

void QxSqlDatabase::setAddSqlSquareBracketsForColumnName(
    bool b, bool bJustForCurrentThread /* = false */,
    QSqlDatabase *pJustForThisDatabase /* = NULL */) {
  bool bUpdateGlobal =
      m_pImpl->setSetting(QStringLiteral("AddSqlSquareBracketsForColumnName"), b,
                          bJustForCurrentThread, pJustForThisDatabase);
  if (bUpdateGlobal) {
    m_pImpl->m_bAddSqlSquareBracketsForColumnName = b;
  }
}

void QxSqlDatabase::setFormatSqlQueryBeforeLogging(
    bool b, bool bJustForCurrentThread /* = false */,
    QSqlDatabase *pJustForThisDatabase /* = NULL */) {
    bool bUpdateGlobal = m_pImpl->setSetting(QStringLiteral("FormatSqlQueryBeforeLogging"),
                                             b,
                                             bJustForCurrentThread,
                                             pJustForThisDatabase);
    if (bUpdateGlobal) {
        m_pImpl->m_bFormatSqlQueryBeforeLogging = b;
  }
}

void QxSqlDatabase::setSqlDelimiterForTableName(
    const QStringList &lst, bool bJustForCurrentThread /* = false */,
    QSqlDatabase *pJustForThisDatabase /* = NULL */) {
  bool bUpdateGlobal =
      m_pImpl->setSetting(QStringLiteral("SqlDelimiterForTableName"), lst,
                          bJustForCurrentThread, pJustForThisDatabase);
  if (bUpdateGlobal) {
    m_pImpl->m_lstSqlDelimiterForTableName = lst;
  }
}

void QxSqlDatabase::setSqlDelimiterForColumnName(
    const QStringList &lst, bool bJustForCurrentThread /* = false */,
    QSqlDatabase *pJustForThisDatabase /* = NULL */) {
    bool bUpdateGlobal = m_pImpl->setSetting(QStringLiteral("SqlDelimiterForColumnName"),
                                             lst,
                                             bJustForCurrentThread,
                                             pJustForThisDatabase);
    if (bUpdateGlobal) {
        m_pImpl->m_lstSqlDelimiterForColumnName = lst;
  }
}

void QxSqlDatabase::setTraceSqlOnlySlowQueriesDatabase(
    int i, bool bJustForCurrentThread /* = false */,
    QSqlDatabase *pJustForThisDatabase /* = NULL */) {
  bool bUpdateGlobal =
      m_pImpl->setSetting(QStringLiteral("TraceSqlOnlySlowQueriesDatabase"), i,
                          bJustForCurrentThread, pJustForThisDatabase);
  if (bUpdateGlobal) {
    m_pImpl->m_iTraceSqlOnlySlowQueriesDatabase = i;
  }
}

void QxSqlDatabase::setTraceSqlOnlySlowQueriesTotal(
    int i, bool bJustForCurrentThread /* = false */,
    QSqlDatabase *pJustForThisDatabase /* = NULL */) {
    bool bUpdateGlobal = m_pImpl->setSetting(QStringLiteral("TraceSqlOnlySlowQueriesTotal"),
                                             i,
                                             bJustForCurrentThread,
                                             pJustForThisDatabase);
    if (bUpdateGlobal) {
        m_pImpl->m_iTraceSqlOnlySlowQueriesTotal = i;
  }
}

void QxSqlDatabase::setDisplayTimerDetails(
    bool b, bool bJustForCurrentThread /* = false */,
    QSqlDatabase *pJustForThisDatabase /* = NULL */) {
  bool bUpdateGlobal = m_pImpl->setSetting(
      QStringLiteral("DisplayTimerDetails"), b, bJustForCurrentThread, pJustForThisDatabase);
  if (bUpdateGlobal) {
    m_pImpl->m_bDisplayTimerDetails = b;
  }
}

QSqlDatabase QxSqlDatabase::getDatabase(QSqlError &dbError) {
  return QxSqlDatabase::getSingleton()->m_pImpl->getDatabaseByCurrThreadId(
      dbError);
}

QSqlDatabase QxSqlDatabase::getDatabase() {
  QSqlError dbError;
  Q_UNUSED(dbError);
  return QxSqlDatabase::getDatabase(dbError);
}

QSqlDatabase QxSqlDatabase::getDatabaseCloned() {
  QSqlError dbError;
  Q_UNUSED(dbError);
  QString sKeyClone = QUuid::createUuid().toString();
  return QSqlDatabase::cloneDatabase(QxSqlDatabase::getDatabase(dbError),
                                     sKeyClone);
}

QSqlDatabase QxSqlDatabase::checkDatabaseByThread() {
  QxSqlDatabase::QxSqlDatabaseImpl *pImpl =
      QxSqlDatabase::getSingleton()->m_pImpl.get();
  QMutexLocker locker(&pImpl->m_oDbMutex);
  Qt::HANDLE lCurrThreadId = QThread::currentThreadId();
  if (!pImpl->m_lstDbByThread.contains(lCurrThreadId)) {
    return QSqlDatabase();
  }
  QString sDbKey = pImpl->m_lstDbByThread.value(lCurrThreadId);
  if (!QSqlDatabase::contains(sDbKey)) {
    return QSqlDatabase();
  }
  return QSqlDatabase::database(sDbKey);
}

QSqlDatabase QxSqlDatabase::QxSqlDatabaseImpl::getDatabaseByCurrThreadId(
    QSqlError &dbError) {
  QMutexLocker locker(&m_oDbMutex);
  dbError = QSqlError();

  if (!isValid()) {
    qDebug("[QxOrm] qx::QxSqlDatabase : '%s'", "parameters are not valid");
    dbError = QSqlError(QStringLiteral("[QxOrm] qx::QxSqlDatabase : 'parameters are not valid'"),
                        QLatin1String(""), QSqlError::UnknownError);
    qAssert(false);
    return QSqlDatabase();
  }

  Qt::HANDLE lCurrThreadId = QThread::currentThreadId();
  if (!lCurrThreadId) {
    qDebug("[QxOrm] qx::QxSqlDatabase : '%s'",
           "unable to find current thread id");
    dbError = QSqlError(
        QStringLiteral("[QxOrm] qx::QxSqlDatabase : 'unable to find current thread id'"), QLatin1String(""),
    QSqlError::UnknownError);
    qAssert(false);
    return QSqlDatabase();
  }

  if (!m_lstDbByThread.contains(lCurrThreadId)) {
    return createDatabase(dbError);
  }
  QString sDbKey = m_lstDbByThread.value(lCurrThreadId);
  if (!QSqlDatabase::contains(sDbKey)) {
    return createDatabase(dbError);
  }
  return QSqlDatabase::database(sDbKey);
}

namespace helper {

template <typename T, bool bIsPointer> struct CvtQtHandle {
  static QString toString(T t) {
    return QString::number(static_cast<qlonglong>(t));
  }
};

template <typename T> struct CvtQtHandle<T, true> {
  static QString toString(T t) {
    const void *ptr = static_cast<const void *>(t);
    QString value;
    QTextStream stream(&value);
    stream << ptr;
    return value;
  }
};

} // namespace helper

QSqlDatabase
QxSqlDatabase::QxSqlDatabaseImpl::createDatabase(QSqlError &dbError) {
  Qt::HANDLE lCurrThreadId = QThread::currentThreadId();
  QString sCurrThreadId = qx::helper::CvtQtHandle<
      Qt::HANDLE, std::is_pointer<Qt::HANDLE>::value>::toString(lCurrThreadId);
  QString sDbKeyNew = QUuid::createUuid().toString();
  dbError = QSqlError();
  bool bError = false;

  {
    QSqlDatabase db =
        QSqlDatabase::addDatabase(m_pParent->getDriverName(), sDbKeyNew);
    db.setConnectOptions(m_pParent->getConnectOptions());
    db.setDatabaseName(m_pParent->getDatabaseName());
    db.setUserName(m_pParent->getUserName());
    db.setPassword(m_pParent->getPassword());
    db.setHostName(m_pParent->getHostName());
    if (m_pParent->getPort() != -1) {
      db.setPort(m_pParent->getPort());
    }
    if (!db.open()) {
        displayLastError(db, QStringLiteral("unable to open connection to database"));
        bError = true;
    }
    if (bError) {
      dbError = db.lastError();
    }
    if (bError && !dbError.isValid()) {
        dbError = QSqlError("[QxOrm] qx::QxSqlDatabase : 'unable to open "
                                           "connection to database'",
                            QLatin1String(""),
                            QSqlError::UnknownError);
    }
  }

  if (bError) {
    QSqlDatabase::removeDatabase(sDbKeyNew);
    return QSqlDatabase();
  }
  m_lstDbByThread.insert(lCurrThreadId, sDbKeyNew);
  qDebug("[QxOrm] qx::QxSqlDatabase : create new database connection in thread "
         "'%s' with key '%s'",
         qPrintable(sCurrThreadId), qPrintable(sDbKeyNew));
  QSqlDatabase dbconn = QSqlDatabase::database(sDbKeyNew);
  if (m_fctDatabaseOpen) {
    m_fctDatabaseOpen(dbconn);
  }
  return dbconn;
}

void QxSqlDatabase::QxSqlDatabaseImpl::displayLastError(
    const QSqlDatabase &db, const QString &sDesc) const {
  QString sLastError = formatLastError(db);
  if (sDesc.isEmpty()) {
    qDebug("[QxOrm] qx::QxSqlDatabase : '%s'", qPrintable(sLastError));
  } else {
    qDebug("[QxOrm] qx::QxSqlDatabase : '%s'\n%s", qPrintable(sDesc),
           qPrintable(sLastError));
  }
}

QString QxSqlDatabase::QxSqlDatabaseImpl::formatLastError(
    const QSqlDatabase &db) const {
  QString sLastError;
  if (db.lastError().nativeErrorCode().toInt() != -1) {
      sLastError += QStringLiteral("Error number '")
                    + db.lastError().nativeErrorCode() + QStringLiteral("' : ");
  }
  if (!db.lastError().text().isEmpty()) {
    sLastError += db.lastError().text();
  } else {
      sLastError += QStringLiteral("<no error description>");
  }
  return sLastError;
}

qx::dao::detail::IxSqlGenerator *QxSqlDatabase::getSqlGenerator() {
  if (m_pImpl->m_lstGeneratorByDatabase.count() > 0) {
    Qt::HANDLE lCurrThreadId = QThread::currentThreadId();
    QString sDatabaseKey =
        (m_pImpl->m_lstCurrDatabaseKeyByThread.contains(lCurrThreadId)
             ? m_pImpl->m_lstCurrDatabaseKeyByThread.value(lCurrThreadId)
             : QString());
    if (m_pImpl->m_lstGeneratorByDatabase.contains(sDatabaseKey)) {
      return m_pImpl->m_lstGeneratorByDatabase.value(sDatabaseKey).get();
    }
  }
  if (m_pImpl->m_lstGeneratorByThread.count() > 0) {
    Qt::HANDLE lCurrThreadId = QThread::currentThreadId();
    if (m_pImpl->m_lstGeneratorByThread.contains(lCurrThreadId)) {
      return m_pImpl->m_lstGeneratorByThread.value(lCurrThreadId).get();
    }
  }

  if (m_pImpl->m_pSqlGenerator) {
    return m_pImpl->m_pSqlGenerator.get();
  }
  QMutexLocker locker(&m_pImpl->m_oDbMutex);

  if (m_pImpl->m_sDriverName == QLatin1String("QMYSQL")) {
      m_pImpl->m_pSqlGenerator = std::make_shared<qx::dao::detail::QxSqlGenerator_MySQL>();
  } else if (m_pImpl->m_sDriverName == QLatin1String("QPSQL"))
      {
          m_pImpl->m_pSqlGenerator = std::make_shared<qx::dao::detail::QxSqlGenerator_PostgreSQL>();
  } else if (m_pImpl->m_sDriverName == QLatin1String("QSQLITE")) {
    m_pImpl->m_pSqlGenerator =
        std::make_shared<qx::dao::detail::QxSqlGenerator_SQLite>();
  } else if (m_pImpl->m_sDriverName == QLatin1String("QOCI")) {
    m_pImpl->m_pSqlGenerator =
        std::make_shared<qx::dao::detail::QxSqlGenerator_Oracle>();
  }

  if (!m_pImpl->m_pSqlGenerator) {
    m_pImpl->m_pSqlGenerator =
        std::make_shared<qx::dao::detail::QxSqlGenerator_Standard>();
  }
  m_pImpl->m_pSqlGenerator->init();
  return m_pImpl->m_pSqlGenerator.get();
}

void QxSqlDatabase::closeAllDatabases() {
  qx::QxSqlDatabase *pSingleton = qx::QxSqlDatabase::getSingleton();
  if (!pSingleton) {
    qAssert(false);
    return;
  }
  QMutexLocker locker(&pSingleton->m_pImpl->m_oDbMutex);

  Q_FOREACH (QString sDbKey, pSingleton->m_pImpl->m_lstDbByThread) {
    QSqlDatabase::database(sDbKey).close();
    QSqlDatabase::removeDatabase(sDbKey);
  }
  pSingleton->m_pImpl->m_lstDbByThread.clear();

#ifdef _QX_ENABLE_MONGODB
  qx::dao::mongodb::QxMongoDB_Helper::clearPoolConnection();
#endif // _QX_ENABLE_MONGODB
}

void QxSqlDatabase::clearAllDatabases() {
  qx::QxSqlDatabase *pSingleton = qx::QxSqlDatabase::getSingleton();
  if (!pSingleton) {
    qAssert(false);
    return;
  }
  QMutexLocker locker(&pSingleton->m_pImpl->m_oDbMutex);

  qx::QxSqlDatabase::closeAllDatabases();
  pSingleton->m_pImpl->m_sDriverName = QLatin1String("");
  pSingleton->m_pImpl->m_sConnectOptions = QLatin1String("");
  pSingleton->m_pImpl->m_sDatabaseName = QLatin1String("");
  pSingleton->m_pImpl->m_sUserName = QLatin1String("");
  pSingleton->m_pImpl->m_sPassword = QLatin1String("");
  pSingleton->m_pImpl->m_sHostName = QLatin1String("");
  pSingleton->m_pImpl->m_iPort = -1;
}

bool QxSqlDatabase::isEmpty() {
  qx::QxSqlDatabase *pSingleton = qx::QxSqlDatabase::getSingleton();
  if (!pSingleton) {
    qAssert(false);
    return true;
  }
  QMutexLocker locker(&pSingleton->m_pImpl->m_oDbMutex);
  return pSingleton->m_pImpl->m_lstDbByThread.isEmpty();
}

bool QxSqlDatabase::setCurrentDatabaseByThread(QSqlDatabase *p) {
  if (!p) {
    return false;
  }
  QString sDatabaseKey = m_pImpl->computeDatabaseKey(p);
  QString sCurrDatabaseKey = m_pImpl->computeDatabaseKey(NULL);
  if (sDatabaseKey.toUpper() == sCurrDatabaseKey.toUpper()) {
    return false;
  }

  QMutexLocker locker(&m_pImpl->m_oDbMutex);
  Qt::HANDLE lCurrThreadId = QThread::currentThreadId();
  if (!sDatabaseKey.isEmpty() &&
      !m_pImpl->m_lstCurrDatabaseKeyByThread.contains(lCurrThreadId)) {
    m_pImpl->m_lstCurrDatabaseKeyByThread.insert(lCurrThreadId, sDatabaseKey);
    return true;
  }
  return false;
}

void QxSqlDatabase::clearCurrentDatabaseByThread() {
  if (m_pImpl->m_lstCurrDatabaseKeyByThread.count() <= 0) {
    return;
  }
  QMutexLocker locker(&m_pImpl->m_oDbMutex);
  Qt::HANDLE lCurrThreadId = QThread::currentThreadId();
  m_pImpl->m_lstCurrDatabaseKeyByThread.remove(lCurrThreadId);
}

void QxSqlDatabase::clearAllSettingsForCurrentThread() {
  QMutexLocker locker(&m_pImpl->m_oDbMutex);
  Qt::HANDLE lCurrThreadId = QThread::currentThreadId();
  QMutableHashIterator<QPair<Qt::HANDLE, QString>, QVariant> itr(
      m_pImpl->m_lstSettingsByThread);
  while (itr.hasNext()) {
    itr.next();
    if (itr.key().first == lCurrThreadId) {
      itr.remove();
    }
  }
  QMutableHashIterator<Qt::HANDLE, qx::dao::detail::IxSqlGenerator_ptr> itr2(
      m_pImpl->m_lstGeneratorByThread);
  while (itr2.hasNext()) {
    itr2.next();
    if (itr2.key() == lCurrThreadId) {
      itr2.remove();
    }
  }
}

void QxSqlDatabase::clearAllSettingsForDatabase(QSqlDatabase *p) {
  if (!p) {
    return;
  }
  QMutexLocker locker(&m_pImpl->m_oDbMutex);
  QString sDatabaseKey = m_pImpl->computeDatabaseKey(p);
  QMutableHashIterator<QPair<QString, QString>, QVariant> itr(
      m_pImpl->m_lstSettingsByDatabase);
  while (itr.hasNext()) {
    itr.next();
    if (itr.key().first == sDatabaseKey) {
      itr.remove();
    }
  }
  QMutableHashIterator<QString, qx::dao::detail::IxSqlGenerator_ptr> itr2(
      m_pImpl->m_lstGeneratorByDatabase);
  while (itr2.hasNext()) {
    itr2.next();
    if (itr2.key() == sDatabaseKey) {
      itr2.remove();
    }
  }
}

} // namespace qx
