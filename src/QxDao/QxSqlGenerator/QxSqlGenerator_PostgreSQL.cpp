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

#include <QxDao/QxSqlGenerator/QxSqlGenerator_PostgreSQL.h>

#include <QxDao/QxSqlDatabase.h>
#include <QxDao/IxDao_Helper.h>

#include <QxRegister/QxClassX.h>

#include <QxMemLeak/mem_leak.h>

namespace qx {
namespace dao {
namespace detail {

QxSqlGenerator_PostgreSQL::QxSqlGenerator_PostgreSQL() : QxSqlGenerator_Standard() { this->initSqlTypeByClassName(); }

QxSqlGenerator_PostgreSQL::~QxSqlGenerator_PostgreSQL() { ; }

void QxSqlGenerator_PostgreSQL::checkSqlInsert(IxDao_Helper * pDaoHelper, QString & sql) const
{
   if (! pDaoHelper) { qAssert(false); return; }
   if (! pDaoHelper->getDataId()) { return; }
   qx::IxDataMember * pId = pDaoHelper->getDataId();
   if (! pId->getAutoIncrement()) { return; }
   if (pId->getNameCount() > 1) { qAssert(false); return; }
   QString sqlToAdd = " RETURNING " + pId->getName();
   if (sql.right(sqlToAdd.size()) == sqlToAdd) { return; }
   sql += sqlToAdd;
   pDaoHelper->builder().setSqlQuery(sql);
}

void QxSqlGenerator_PostgreSQL::onAfterInsert(IxDao_Helper * pDaoHelper, void * pOwner) const
{
   if (! pDaoHelper || ! pOwner) { qAssert(false); return; }
   if (! pDaoHelper->getDataId()) { return; }
   qx::IxDataMember * pId = pDaoHelper->getDataId();
   if (! pId->getAutoIncrement()) { return; }
   if (pId->getNameCount() > 1) { qAssert(false); return; }
   if (! pDaoHelper->nextRecord()) { qAssert(false); return; }
   QVariant vId = pDaoHelper->query().value(0);
   pId->fromVariant(pOwner, vId, -1, qx::cvt::context::e_database);
}

void QxSqlGenerator_PostgreSQL::initSqlTypeByClassName() const
{
   QHash<QString, QString> * lstSqlType = qx::QxClassX::getAllSqlTypeByClassName();
   if (! lstSqlType) { qAssert(false); return; }

   lstSqlType->insert(QStringLiteral("bool"), QStringLiteral("BOOLEAN"));
   lstSqlType->insert(QStringLiteral("qx_bool"), QStringLiteral("TEXT"));
   lstSqlType->insert(QStringLiteral("short"), QStringLiteral("SMALLINT"));
   lstSqlType->insert(QStringLiteral("int"), QStringLiteral("INTEGER"));
   lstSqlType->insert(QStringLiteral("long"), QStringLiteral("INTEGER"));
   lstSqlType->insert(QStringLiteral("long long"), QStringLiteral("BIGINT"));
   lstSqlType->insert(QStringLiteral("float"), QStringLiteral("FLOAT"));
   lstSqlType->insert(QStringLiteral("double"), QStringLiteral("FLOAT"));
   lstSqlType->insert(QStringLiteral("long double"), QStringLiteral("FLOAT"));
   lstSqlType->insert(QStringLiteral("unsigned short"), QStringLiteral("SMALLINT"));
   lstSqlType->insert(QStringLiteral("unsigned int"), QStringLiteral("INTEGER"));
   lstSqlType->insert(QStringLiteral("unsigned long"), QStringLiteral("INTEGER"));
   lstSqlType->insert(QStringLiteral("unsigned long long"), QStringLiteral("BIGINT"));
   lstSqlType->insert(QStringLiteral("std::string"), QStringLiteral("TEXT"));
   lstSqlType->insert(QStringLiteral("std::wstring"), QStringLiteral("TEXT"));
   lstSqlType->insert(QStringLiteral("QString"), QStringLiteral("TEXT"));
   lstSqlType->insert(QStringLiteral("QVariant"), QStringLiteral("TEXT"));
   lstSqlType->insert(QStringLiteral("QUuid"), QStringLiteral("TEXT"));
   lstSqlType->insert(QStringLiteral("QDate"), QStringLiteral("DATE"));
   lstSqlType->insert(QStringLiteral("QTime"), QStringLiteral("TIME"));
   lstSqlType->insert(QStringLiteral("QDateTime"), QStringLiteral("TIMESTAMP"));
   lstSqlType->insert(QStringLiteral("QByteArray"), QStringLiteral("BYTEA"));
   lstSqlType->insert(QStringLiteral("qx::QxDateNeutral"), QStringLiteral("TEXT"));
   lstSqlType->insert(QStringLiteral("qx::QxTimeNeutral"), QStringLiteral("TEXT"));
   lstSqlType->insert(QStringLiteral("qx::QxDateTimeNeutral"), QStringLiteral("TEXT"));
}

} // namespace detail
} // namespace dao
} // namespace qx
