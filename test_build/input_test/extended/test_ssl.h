/*************************************************************************
*
* Copyright (C) 2012-2020 Barbara Geller & Ansel Sermersheim
*
* GNU Free Documentation License
*
*************************************************************************/

#ifndef QSSL_H
#define QSSL_H

namespace QSsl {

   enum KeyType {
      PrivateKey,
      PublicKey
   };

   enum KeyAlgorithm {
      Rsa,
      Dsa
   };

   void someFunction(QString data);
};

#endif
