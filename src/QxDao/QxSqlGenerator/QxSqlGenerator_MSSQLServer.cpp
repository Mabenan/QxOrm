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

#include <QxDao/QxSqlGenerator/QxSqlGenerator_MSSQLServer.h>

#include <QxDao/QxSqlDatabase.h>

#include <QxRegister/QxClassX.h>

#include <QxMemLeak/mem_leak.h>

namespace qx {
namespace dao {
namespace detail {

QxSqlGenerator_MSSQLServer::QxSqlGenerator_MSSQLServer() : QxSqlGenerator_Standard() { this->initSqlTypeByClassName(); }

QxSqlGenerator_MSSQLServer::~QxSqlGenerator_MSSQLServer() { ; }

void QxSqlGenerator_MSSQLServer::init() { qx::QxSqlDatabase::getSingleton()->setAddAutoIncrementIdToUpdateQuery(false); }

QString QxSqlGenerator_MSSQLServer::getLimit(const QxSqlLimit *pLimit) const
{
    Q_UNUSED(pLimit); return QLatin1String("");
}

void QxSqlGenerator_MSSQLServer::resolveLimit(QSqlQuery & query, const QxSqlLimit * pLimit) const
{
   if (! pLimit) { qAssert(false); return; }
   QString sRowsCount = pLimit->getRowsCount_ParamKey();
   int iRowsCount(pLimit->getRowsCount());
   bool bQuestionMark = (qx::QxSqlDatabase::getSingleton()->getSqlPlaceHolderStyle() == qx::QxSqlDatabase::ph_style_question_mark);
   if (bQuestionMark) { query.addBindValue(iRowsCount); }
   else { query.bindValue(sRowsCount, iRowsCount); }
}

void QxSqlGenerator_MSSQLServer::postProcess(QString & sql, const QxSqlLimit * pLimit) const
{
   if (! pLimit) { qAssert(false); return; }
   if (!sql.leftRef(7).contains(QLatin1String("SELECT "), Qt::CaseInsensitive))
       {
           qAssert(false);
           return;
       }
   QString sRowsCount = pLimit->getRowsCount_ParamKey();
   sql = sql.right(sql.size() - 7);
   sql = "SELECT TOP " + sRowsCount + " " + sql;
}

void QxSqlGenerator_MSSQLServer::initSqlTypeByClassName() const
{
   QHash<QString, QString> * lstSqlType = qx::QxClassX::getAllSqlTypeByClassName();
   if (! lstSqlType) { qAssert(false); return; }

   lstSqlType->insert(QStringLiteral("bool"), QStringLiteral("TINYINT"));
   lstSqlType->insert(QStringLiteral("qx_bool"), QStringLiteral("TEXT"));
   lstSqlType->insert(QStringLiteral("short"), QStringLiteral("SMALLINT"));
   lstSqlType->insert(QStringLiteral("int"), QStringLiteral("INT"));
   lstSqlType->insert(QStringLiteral("long"), QStringLiteral("INT"));
   lstSqlType->insert(QStringLiteral("long long"), QStringLiteral("BIGINT"));
   lstSqlType->insert(QStringLiteral("float"), QStringLiteral("FLOAT"));
   lstSqlType->insert(QStringLiteral("double"), QStringLiteral("FLOAT"));
   lstSqlType->insert(QStringLiteral("long double"), QStringLiteral("FLOAT"));
   lstSqlType->insert(QStringLiteral("unsigned short"), QStringLiteral("SMALLINT"));
   lstSqlType->insert(QStringLiteral("unsigned int"), QStringLiteral("INT"));
   lstSqlType->insert(QStringLiteral("unsigned long"), QStringLiteral("INT"));
   lstSqlType->insert(QStringLiteral("unsigned long long"), QStringLiteral("BIGINT"));
   lstSqlType->insert(QStringLiteral("std::string"), QStringLiteral("TEXT"));
   lstSqlType->insert(QStringLiteral("std::wstring"), QStringLiteral("TEXT"));
   lstSqlType->insert(QStringLiteral("QString"), QStringLiteral("TEXT"));
   lstSqlType->insert(QStringLiteral("QVariant"), QStringLiteral("TEXT"));
   lstSqlType->insert(QStringLiteral("QUuid"), QStringLiteral("TEXT"));
   lstSqlType->insert(QStringLiteral("QDate"), QStringLiteral("DATE"));
   lstSqlType->insert(QStringLiteral("QTime"), QStringLiteral("TIME"));
   lstSqlType->insert(QStringLiteral("QDateTime"), QStringLiteral("TIMESTAMP"));
   lstSqlType->insert(QStringLiteral("QByteArray"), QStringLiteral("IMAGE"));
   lstSqlType->insert(QStringLiteral("qx::QxDateNeutral"), QStringLiteral("TEXT"));
   lstSqlType->insert(QStringLiteral("qx::QxTimeNeutral"), QStringLiteral("TEXT"));
   lstSqlType->insert(QStringLiteral("qx::QxDateTimeNeutral"), QStringLiteral("TEXT"));
}

} // namespace detail
} // namespace dao
} // namespace qx
