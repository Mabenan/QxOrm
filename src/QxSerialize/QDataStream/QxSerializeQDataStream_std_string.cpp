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

#include <QxSerialize/QDataStream/QxSerializeQDataStream_std_string.h>

#include <QxConvert/QxConvert.h>
#include <QxConvert/QxConvert_Impl.h>

#include <QxMemLeak/mem_leak.h>

QDataStream & operator<< (QDataStream & stream, const std::string & t)
{
    QString tmp = qx::cvt::detail::QxConvert_ToString<std::string>::toString(
        t, QLatin1String(""), 0, qx::cvt::context::e_no_context);
    stream << tmp;
    return stream;
}

QDataStream & operator>> (QDataStream & stream, std::string & t)
{
   QString tmp;
   stream >> tmp;
   qx::cvt::detail::QxConvert_FromString<std::string>::fromString(tmp, t, QLatin1String(""), 0, qx::cvt::context::e_no_context);
   return stream;
}

QDataStream & operator<< (QDataStream & stream, const std::wstring & t)
{
    QString tmp = qx::cvt::detail::QxConvert_ToString<std::wstring>::toString(t, QLatin1String(""), 0, qx::cvt::context::e_no_context);
    stream << tmp;
    return stream;
}

QDataStream & operator>> (QDataStream & stream, std::wstring & t)
{
   QString tmp;
   stream >> tmp;
   qx::cvt::detail::QxConvert_FromString<std::wstring>::fromString(tmp, t, QLatin1String(""), 0, qx::cvt::context::e_no_context);
   return stream;
}
