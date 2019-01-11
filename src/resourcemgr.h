/************************************************************************
*
* Copyright (C) 2014-2019 Barbara Geller & Ansel Sermersheim
* Copyright (C) 1997-2014 by Dimitri van Heesch
*
* DoxyPress is free software: you can redistribute it and/or
* modify it under the terms of the GNU General Public License version 2
* as published by the Free Software Foundation.
*
* DoxyPress is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*
* Documents produced by DoxyPress are derivative works derived from the
* input used in their production; they are not affected by this license.
*
*************************************************************************/

#ifndef RESOURCEMGR_H
#define RESOURCEMGR_H

#include <QByteArray>
#include <QString>

// manages resources compiled into an executable
class ResourceMgr
{
 public:
   static ResourceMgr &instance();
   QByteArray getAsString(const QString &fName) const;

   // Copies a registered resource to a given target directory under a given target name
   bool copyResourceAs(const QString &fName, const QString &targetDir, const QString &targetName) const;

 private:
   ResourceMgr();
   ~ResourceMgr();

   enum Type { Verbatim, Luminance, LumAlpha, CSS };
};

#endif
