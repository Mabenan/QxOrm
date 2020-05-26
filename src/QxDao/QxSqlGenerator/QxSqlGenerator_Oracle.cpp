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

#include <QxDao/QxSqlGenerator/QxSqlGenerator_Oracle.h>

#include <QxDao/QxSqlDatabase.h>
#include <QxDao/IxDao_Helper.h>

#include <QxRegister/QxClassX.h>

#include <QxMemLeak/mem_leak.h>

namespace qx {
namespace dao {
namespace detail {

QxSqlGenerator_Oracle::QxSqlGenerator_Oracle() : QxSqlGenerator_Standard(), m_bOldLimitSyntax(false), m_bManageLastInsertId(false) { this->initSqlTypeByClassName(); }

QxSqlGenerator_Oracle::QxSqlGenerator_Oracle(bool bManageLastInsertId) : QxSqlGenerator_Standard(), m_bOldLimitSyntax(false), m_bManageLastInsertId(bManageLastInsertId) { this->initSqlTypeByClassName(); }

QxSqlGenerator_Oracle::~QxSqlGenerator_Oracle() { ; }

bool QxSqlGenerator_Oracle::getOldLimitSyntax() const { return m_bOldLimitSyntax; }

void QxSqlGenerator_Oracle::setOldLimitSyntax(bool b) { m_bOldLimitSyntax = b; }

bool QxSqlGenerator_Oracle::getManageLastInsertId() const { return m_bManageLastInsertId; }

void QxSqlGenerator_Oracle::setManageLastInsertId(bool b) { m_bManageLastInsertId = b; }

QString QxSqlGenerator_Oracle::getTableAliasSep() const
{
    return QStringLiteral(" ");
}

QString QxSqlGenerator_Oracle::getLimit(const QxSqlLimit *pLimit) const
{
    Q_UNUSED(pLimit);
    return QLatin1String("");
}

void QxSqlGenerator_Oracle::resolveLimit(QSqlQuery & query, const QxSqlLimit * pLimit) const
{
   if (! m_bOldLimitSyntax) { return; }
   if (! pLimit) { qAssert(false); return; }
   QString sMinRow = pLimit->getStartRow_ParamKey();
   QString sMaxRow = pLimit->getMaxRow_ParamKey();
   int iMinRow(pLimit->getStartRow()), iMaxRow(pLimit->getMaxRow());
   bool bQuestionMark = (qx::QxSqlDatabase::getSingleton()->getSqlPlaceHolderStyle() == qx::QxSqlDatabase::ph_style_question_mark);
   if (bQuestionMark) { query.addBindValue(iMaxRow); query.addBindValue(iMinRow); }
   else { query.bindValue(sMaxRow, iMaxRow); query.bindValue(sMinRow, iMinRow); }
}

void QxSqlGenerator_Oracle::postProcess(QString & sql, const QxSqlLimit * pLimit) const
{
   if (! pLimit) { qAssert(false); return; }
   QString sMinRow = pLimit->getStartRow_ParamKey();
   QString sMaxRow = pLimit->getMaxRow_ParamKey();
   int iMinRow(pLimit->getStartRow()), iRowsCount(pLimit->getRowsCount());
   bool bWithTies = pLimit->getWithTies();

   if (m_bOldLimitSyntax)
   {
      QString sqlPaging;
      QString sReplace = QStringLiteral("%SQL_QUERY%");
      sqlPaging += QLatin1String("SELECT * FROM ");
      sqlPaging += QLatin1String("   ( SELECT a.*, ROWNUM rnum FROM ");
      sqlPaging += "      ( " + sReplace + " ) a ";
      sqlPaging += "     WHERE ROWNUM <= " + sMaxRow + " ) ";
      sqlPaging += "WHERE rnum >= " + sMinRow;
      sqlPaging.replace(sReplace, sql);
      sql = sqlPaging;
      return;
   }

   if (iMinRow <= 0)
   {
      if (bWithTies) { sql += " FETCH FIRST " + QString::number(iRowsCount) + " ROWS WITH TIES"; }
      else { sql += " FETCH FIRST " + QString::number(iRowsCount) + " ROWS ONLY"; }
   }
   else
   {
      if (bWithTies) { sql += " OFFSET " + QString::number(iMinRow) + " ROWS FETCH NEXT " + QString::number(iRowsCount) + " ROWS WITH TIES"; }
      else { sql += " OFFSET " + QString::number(iMinRow) + " ROWS FETCH NEXT " + QString::number(iRowsCount) + " ROWS ONLY"; }
   }
}

void QxSqlGenerator_Oracle::checkSqlInsert(IxDao_Helper * pDaoHelper, QString & sql) const
{
   if (! m_bManageLastInsertId) { return; }
   if (! pDaoHelper) { qAssert(false); return; }
   if (! pDaoHelper->getDataId()) { return; }
   qx::IxDataMember * pId = pDaoHelper->getDataId();
   if (! pId->getAutoIncrement()) { return; }
   if (pId->getNameCount() > 1) { qAssert(false); return; }
   QString sqlToAdd = QStringLiteral(" RETURNING ID INTO :ID; END;");
   if (sql.right(sqlToAdd.size()) == sqlToAdd) { return; }
   sql = "BEGIN " + sql + sqlToAdd;
   pDaoHelper->builder().setSqlQuery(sql);
}

void QxSqlGenerator_Oracle::onBeforeInsert(IxDao_Helper * pDaoHelper, void * pOwner) const
{
   if (! m_bManageLastInsertId) { return; }
   if (! pDaoHelper || ! pOwner) { qAssert(false); return; }
   if (! pDaoHelper->getDataId()) { return; }
   qx::IxDataMember * pId = pDaoHelper->getDataId();
   if (! pId->getAutoIncrement()) { return; }
   if (pId->getNameCount() > 1) { qAssert(false); return; }
   pDaoHelper->query().bindValue(QStringLiteral(":ID"), 0, QSql::InOut);
}

void QxSqlGenerator_Oracle::onAfterInsert(IxDao_Helper * pDaoHelper, void * pOwner) const
{
   if (! m_bManageLastInsertId) { return; }
   if (! pDaoHelper || ! pOwner) { qAssert(false); return; }
   if (! pDaoHelper->getDataId()) { return; }
   qx::IxDataMember * pId = pDaoHelper->getDataId();
   if (! pId->getAutoIncrement()) { return; }
   if (pId->getNameCount() > 1) { qAssert(false); return; }
   QVariant vId = pDaoHelper->query().boundValue(QStringLiteral(":ID"));
   pId->fromVariant(pOwner, vId, -1, qx::cvt::context::e_database);
}

void QxSqlGenerator_Oracle::initSqlTypeByClassName() const
{
   QHash<QString, QString> * lstSqlType = qx::QxClassX::getAllSqlTypeByClassName();
   if (! lstSqlType) { qAssert(false); return; }

   lstSqlType->insert(QStringLiteral("bool"), QStringLiteral("SMALLINT"));
   lstSqlType->insert(QStringLiteral("qx_bool"), QStringLiteral("VARCHAR2(4000)"));
   lstSqlType->insert(QStringLiteral("short"), QStringLiteral("SMALLINT"));
   lstSqlType->insert(QStringLiteral("int"), QStringLiteral("INTEGER"));
   lstSqlType->insert(QStringLiteral("long"), QStringLiteral("INTEGER"));
   lstSqlType->insert(QStringLiteral("long long"), QStringLiteral("INTEGER"));
   lstSqlType->insert(QStringLiteral("float"), QStringLiteral("FLOAT"));
   lstSqlType->insert(QStringLiteral("double"), QStringLiteral("FLOAT"));
   lstSqlType->insert(QStringLiteral("long double"), QStringLiteral("FLOAT"));
   lstSqlType->insert(QStringLiteral("unsigned short"), QStringLiteral("SMALLINT"));
   lstSqlType->insert(QStringLiteral("unsigned int"), QStringLiteral("INTEGER"));
   lstSqlType->insert(QStringLiteral("unsigned long"), QStringLiteral("INTEGER"));
   lstSqlType->insert(QStringLiteral("unsigned long long"), QStringLiteral("INTEGER"));
   lstSqlType->insert(QStringLiteral("std::string"), QStringLiteral("VARCHAR2(4000)"));
   lstSqlType->insert(QStringLiteral("std::wstring"), QStringLiteral("VARCHAR2(4000)"));
   lstSqlType->insert(QStringLiteral("QString"), QStringLiteral("VARCHAR2(4000)"));
   lstSqlType->insert(QStringLiteral("QVariant"), QStringLiteral("CLOB"));
   lstSqlType->insert(QStringLiteral("QUuid"), QStringLiteral("VARCHAR2(255)"));
   lstSqlType->insert(QStringLiteral("QDate"), QStringLiteral("DATE"));
   lstSqlType->insert(QStringLiteral("QTime"), QStringLiteral("DATE"));
   lstSqlType->insert(QStringLiteral("QDateTime"), QStringLiteral("TIMESTAMP"));
   lstSqlType->insert(QStringLiteral("QByteArray"), QStringLiteral("BLOB"));
   lstSqlType->insert(QStringLiteral("qx::QxDateNeutral"), QStringLiteral("VARCHAR2(8)"));
   lstSqlType->insert(QStringLiteral("qx::QxTimeNeutral"), QStringLiteral("VARCHAR2(6)"));
   lstSqlType->insert(QStringLiteral("qx::QxDateTimeNeutral"), QStringLiteral("VARCHAR2(14)"));
}

} // namespace detail
} // namespace dao
} // namespace qx
