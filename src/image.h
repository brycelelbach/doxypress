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

#ifndef IMAGE_H
#define IMAGE_H

#include <QByteArray>

/** Class representing a bitmap image generated by DoxyPress. */
class Image
{
 public:
   Image(int w, int h);
   ~Image();

   void setPixel(int x, int y, uchar val);
   uchar getPixel(int x, int y) const;
   void writeChar(int x, int y, char c, uchar fg);
   void writeString(int x, int y, const QString &text, uchar fg);
   void drawHorzLine(int y, int xs, int xe, uchar colIndex, uint mask);
   void drawHorzArrow(int y, int xs, int xe, uchar colIndex, uint mask);
   void drawVertLine(int x, int ys, int ye, uchar colIndex, uint mask);
   void drawVertArrow(int x, int ys, int ye, uchar colIndex, uint mask);
   void drawRect(int x, int y, int width, int height, uchar colIndex, uint mask);
   void fillRect(int x, int y, int width, int height, uchar colIndex, uint mask);

   QByteArray convert(int mode = 0);

   uint getWidth() const {
      return width;
   }

   uint getHeight() const {
      return height;
   }

   uchar *getData() const {
      return data;
   }

   static uint stringLength(const QString &s);

 private:
   int width;
   int height;
   uchar *data;
};

/** Class representing a bitmap image colored based on hue/sat/gamma settings. */
class ColoredImage
{
 public:
   ColoredImage(int width, int height, const uchar *greyLevels, const uchar *alphaLevels,
                int saturation, int hue, int gamma);

   ~ColoredImage();

   QByteArray convert();

   static void hsl2rgb(double h, double s, double l, double *pRed, double *pGreen, double *pBlue);

 private:
   int m_width;
   int m_height;
   uchar *m_data;
   bool m_hasAlpha;
};

#endif
