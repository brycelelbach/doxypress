/*************************************************************************
 *
 * Copyright (C) 1997-2014 by Dimitri van Heesch. 
 * Copyright (C) 2014-2015 Barbara Geller & Ansel Sermersheim 
 * All rights reserved.    
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation under the terms of the GNU General Public License version 2
 * is hereby granted. No representations are made about the suitability of
 * this software for any purpose. It is provided "as is" without express or
 * implied warranty. See the GNU General Public License for more details.
 *
 * Documents produced by Doxygen are derivative works derived from the
 * input used in their production; they are not affected by this license.
 *
*************************************************************************/

#include <QRegExp>
#include <QCryptographicHash>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <config.h>
#include <definition.h>
#include <doxygen.h>
#include <language.h>
#include <message.h>
#include <outputlist.h>
#include <code.h>
#include <util.h>
#include <groupdef.h>
#include <pagedef.h>
#include <section.h>
#include <htags.h>
#include <parserintf.h>
#include <marshal.h>
#include <debug.h>
#include <memberlist.h>
#include <namespacedef.h>
#include <filedef.h>
#include <dirdef.h>

#define START_MARKER 0x4445465B // DEF[
#define END_MARKER   0x4445465D // DEF]

//-----------------------------------------------------------------------------------------


/** Private data associated with a Symbol Definition object. */
class DefinitionImpl
{
 public:
   DefinitionImpl();
   ~DefinitionImpl();
   void init(const char *df, const char *n);

   SectionDict *sectionDict;  // dictionary of all sections, not accessible

   MemberSDict *sourceRefByDict;
   MemberSDict *sourceRefsDict;

   QList<ListItemInfo> *xrefListItems;
   SortedList<GroupDef *> *partOfGroups;

   DocInfo   *details;    // not exported
   DocInfo   *inbodyDocs; // not exported
   BriefInfo *brief;      // not exported
   BodyInfo  *body;       // not exported
   QByteArray   briefSignatures;
   QByteArray   docSignatures;

   QByteArray localName;      // local (unqualified) name of the definition
   // in the future m_name should become m_localName
   QByteArray qualifiedName;
   QByteArray ref;   // reference to external documentation

   bool hidden;
   bool isArtificial;

   Definition *outerScope;  // not owner

   // where the item was found
   QByteArray defFileName;
   QByteArray defFileExt;

   SrcLangExt lang;

   QByteArray id; // clang unique id
};

DefinitionImpl::DefinitionImpl()
   : sectionDict(0), sourceRefByDict(0), sourceRefsDict(0),
     xrefListItems(0), partOfGroups(0),
     details(0), inbodyDocs(0), brief(0), body(0), hidden(false), isArtificial(false),
     outerScope(0), lang(SrcLangExt_Unknown)
{
}

DefinitionImpl::~DefinitionImpl()
{
   delete sectionDict;
   delete sourceRefByDict;
   delete sourceRefsDict;
   delete partOfGroups;
   delete xrefListItems;
   delete brief;
   delete details;
   delete body;
   delete inbodyDocs;
}

void DefinitionImpl::init(const char *df, const char *n)
{
   defFileName = df;
   int lastDot = defFileName.lastIndexOf('.');

   if (lastDot != -1) {
      defFileExt = defFileName.mid(lastDot);
   }

   QByteArray name = n;
   if (name != "<globalScope>") {
      //extractNamespaceName(m_name,m_localName,ns);
      localName = stripScope(n);

   } else {
      localName = n;
   }

   //printf("m_localName=%s\n",m_localName.data());

   brief           = 0;
   details         = 0;
   body            = 0;
   inbodyDocs      = 0;
   sourceRefByDict = 0;
   sourceRefsDict  = 0;
   sectionDict     = 0,
   outerScope      = Doxygen::globalScope;
   partOfGroups    = 0;
   xrefListItems   = 0;
   hidden          = false;
   isArtificial    = false;
   lang            = SrcLangExt_Unknown;
}

//-----------------------------------------------------------------------------------------

static bool matchExcludedSymbols(const char *name)
{
   static QStringList &exclSyms = Config_getList("EXCLUDE_SYMBOLS");

   if (exclSyms.count() == 0) {
      return false;   // nothing specified
   }

   QByteArray symName = name;

   for (auto item : exclSyms) { 

      QString pat = *item; 
      QByteArray pattern = pat.toLatin1();

      bool forceStart = false;
      bool forceEnd   = false;

      if (pattern.at(0) == '^') {
         pattern = pattern.mid(1), forceStart = true;
      }

      if (pattern.at(pattern.length() - 1) == '$') {
         pattern = pattern.left(pattern.length() - 1), forceEnd = true;
      }

      if (pattern.indexOf('*') != -1) { 
         // wildcard mode
         QRegExp re(substitute(pattern, "*", ".*"), Qt::CaseSensitive);

         int i, pl;
         i = re.match(symName, 0, &pl);

         //printf("  %d = re.match(%s) pattern=%s\n",i,symName.data(),pattern.data());

         if (i != -1) { 
            // wildcard match
            int sl = symName.length();

            // check if it is a whole word match
            if ((i == 0     || pattern.at(0) == '*'    || (!isId(symName.at(i - 1))  && !forceStart)) &&
                  (i + pl == sl || pattern.at(i + pl) == '*' || (!isId(symName.at(i + pl)) && !forceEnd)) ) {             
               return true;
            }
         }

      } else if (! pattern.isEmpty()) { 
         // match words
         int i = symName.indexOf(pattern);

         if (i != -1) { 
            // we have a match!
            int pl = pattern.length();
            int sl = symName.length();

            // check if it is a whole word match
            if ((i == 0  || (!isId(symName.at(i - 1))  && !forceStart)) && (i + pl == sl || (!isId(symName.at(i + pl)) && !forceEnd)) ) {               
               return true;
            }
         }
      }      
   }

   //printf("--> name=%s: no match\n",name);
   return false;
}

void Definition::addToMap(const char *name, Definition *d)
{
   bool vhdlOpt = Config_getBool("OPTIMIZE_OUTPUT_VHDL");

   QByteArray symbolName = name;
   int index = computeQualifiedIndex(symbolName);

   if (! vhdlOpt && index != -1) {
      symbolName = symbolName.mid(index + 2);
   }

   if (! symbolName.isEmpty()) {     
      DefinitionIntf *di = Doxygen::symbolMap->find(symbolName);

      if (di == 0) { 
         // new Symbol         
         Doxygen::symbolMap->insert(symbolName, d);

      } else {
         // existing symbol
        
         if (di->definitionType() == DefinitionIntf::TypeSymbolList) { 
            // already multiple symbols
            DefinitionList *dl = (DefinitionList *)di;
            dl->append(d);

         } else { 
            // going from one to two symbols
            Doxygen::symbolMap->take(symbolName);

            DefinitionList *dl = new DefinitionList;
          
            dl->append((Definition *)di);
            dl->append(d);
            Doxygen::symbolMap->insert(symbolName, dl);
         }
      }

      // auto resize if needed
      static int sizeIndex = 9;

      if (Doxygen::symbolMap->size() > SDict_primes[sizeIndex]) {
         Doxygen::symbolMap->resize(SDict_primes[++sizeIndex]);
      }

      d->_setSymbolName(symbolName);
   }
}

void Definition::removeFromMap(Definition *d)
{
   QByteArray symbolName = d->m_symbolName;

   if (!symbolName.isEmpty()) {
      //printf("******* removing symbol `%s' (%p)\n",symbolName.data(),d);
      DefinitionIntf *di = Doxygen::symbolMap->find(symbolName);

      if (di) {
         if (di != d) { // symbolName not unique
            //printf("  removing from list: %p!\n",di);
            DefinitionList *dl = (DefinitionList *)di;
            bool b = dl->removeRef(d);

            assert(b == true);

            if (dl->isEmpty()) {
               Doxygen::symbolMap->take(symbolName);
            }

         } else { // symbolName unique
            //printf("  removing symbol %p\n",di);
            Doxygen::symbolMap->take(symbolName);
         }
      }
   }
}

Definition::Definition(const char *df, int dl, int dc, const char *name, const char *b, const char *d, bool isSymbol)
{
   m_name      = name;
   m_defLine   = dl;
   m_defColumn = dc;

   m_impl = new DefinitionImpl;

   m_impl->init(df, name);
   m_isSymbol = isSymbol;

   if (isSymbol) {
      addToMap(name, this);
   }

   _setBriefDescription(b, df, dl);
   _setDocumentation(d, df, dl, true, false);

   if (matchExcludedSymbols(name)) {
      m_impl->hidden = true;
   }
}

Definition::Definition(const Definition &d) : DefinitionIntf()
{
   m_name = d.m_name;
   m_defLine = d.m_defLine;
   m_impl = new DefinitionImpl;
   *m_impl = *d.m_impl;
   m_impl->sectionDict = 0;
   m_impl->sourceRefByDict = 0;
   m_impl->sourceRefsDict = 0;
   m_impl->partOfGroups = 0;
   m_impl->xrefListItems = 0;
   m_impl->brief = 0;
   m_impl->details = 0;
   m_impl->body = 0;
   m_impl->inbodyDocs = 0;

   if (d.m_impl->sectionDict) {
      m_impl->sectionDict = new SectionDict();

      StringMap<QSharedPointer<SectionInfo>>::Iterator it(*d.m_impl->sectionDict);
      SectionInfo *si;

      for (it.toFirst(); (si = it.current()); ++it) {
         m_impl->sectionDict->append(si->label, si);
      }
   }

   if (d.m_impl->sourceRefByDict) {
      m_impl->sourceRefByDict = new MemberSDict;
      MemberSDict::IteratorDict it(*d.m_impl->sourceRefByDict);
      MemberDef *md;
      for (it.toFirst(); (md = it.current()); ++it) {
         m_impl->sourceRefByDict->append(it.currentKey(), md);
      }
   }

   if (d.m_impl->sourceRefsDict) {
      m_impl->sourceRefsDict = new MemberSDict;
      MemberSDict::IteratorDict it(*d.m_impl->sourceRefsDict);
      MemberDef *md;
      for (it.toFirst(); (md = it.current()); ++it) {
         m_impl->sourceRefsDict->append(it.currentKey(), md);
      }
   }
   if (d.m_impl->partOfGroups) {
      GroupListIterator it(*d.m_impl->partOfGroups);
      GroupDef *gd;
      for (it.toFirst(); (gd = it.current()); ++it) {
         makePartOfGroup(gd);
      }
   }
   if (d.m_impl->xrefListItems) {
      setRefItems(d.m_impl->xrefListItems);
   }
   if (d.m_impl->brief) {
      m_impl->brief = new BriefInfo(*d.m_impl->brief);
   }
   if (d.m_impl->details) {
      m_impl->details = new DocInfo(*d.m_impl->details);
   }
   if (d.m_impl->body) {
      m_impl->body = new BodyInfo(*d.m_impl->body);
   }
   if (d.m_impl->inbodyDocs) {
      m_impl->inbodyDocs = new DocInfo(*d.m_impl->inbodyDocs);
   }

   m_isSymbol = d.m_isSymbol;
   if (m_isSymbol) {
      addToMap(m_name, this);
   }
}

Definition::~Definition()
{
   if (m_isSymbol) {
      removeFromMap(this);
   }
   if (m_impl) {
      delete m_impl;
      m_impl = 0;
   }
}

void Definition::setName(const char *name)
{
   if (name == 0) {
      return;
   }
   m_name = name;
}

void Definition::setId(const char *id)
{
   if (id == 0) {
      return;
   }
   m_impl->id = id;
   if (Doxygen::clangUsrMap) {
      //printf("Definition::setId '%s'->'%s'\n",id,m_name.data());
      Doxygen::clangUsrMap->insert(id, this);
   }
}

QByteArray Definition::id() const
{
   return m_impl->id;
}

void Definition::addSectionsToDefinition(QList<SectionInfo> *anchorList)
{
   if (!anchorList) {
      return;
   }
   //printf("%s: addSectionsToDefinition(%d)\n",name().data(),anchorList->count());
   QListIterator<SectionInfo> it(*anchorList);
   SectionInfo *si;
   for (; (si = it.current()); ++it) {
      //printf("Add section `%s' to definition `%s'\n",
      //    si->label.data(),name().data());

      SectionInfo *gsi = Doxygen::sectionDict->find(si->label);

      //printf("===== label=%s gsi=%p\n",si->label.data(),gsi);
      if (gsi == 0) {
         gsi = new SectionInfo(*si);
         Doxygen::sectionDict->append(si->label, gsi);
      }

      if (m_impl->sectionDict == 0) {
         m_impl->sectionDict = new SectionDict();
      }

      if (m_impl->sectionDict->find(gsi->label) == 0) {
         m_impl->sectionDict->append(gsi->label, gsi);
         gsi->definition = this;
      }
   }
}

bool Definition::hasSections() const
{
   //printf("Definition::hasSections(%s) #sections=%d\n",name().data(),
   //    m_impl->sectionDict ? m_impl->sectionDict->count() : 0);

   if (m_impl->sectionDict == 0) {
      return false;
   }

   StringMap<QSharedPointer<SectionInfo>>::Iterator li(*m_impl->sectionDict);
   SectionInfo *si;

   for (li.toFirst(); (si = li.current()); ++li) {

      if (si->type == SectionInfo::Section ||
            si->type == SectionInfo::Subsection ||
            si->type == SectionInfo::Subsubsection ||
            si->type == SectionInfo::Paragraph) {
         return true;
      }
   }

   return false;
}

void Definition::addSectionsToIndex()
{
   if (m_impl->sectionDict == 0) {
      return;
   }
   
   //printf("Definition::addSectionsToIndex()\n");
   StringMap<QSharedPointer<SectionInfo>>::Iterator li(*m_impl->sectionDict);

   SectionInfo *si;
   int level = 1;

   for (li.toFirst(); (si = li.current()); ++li) {

      if (si->type == SectionInfo::Section         || si->type == SectionInfo::Subsection    ||
            si->type == SectionInfo::Subsubsection || si->type == SectionInfo::Paragraph) {

         //printf("  level=%d title=%s\n",level,si->title.data());

         int nextLevel = (int)si->type;
         int i;

         if (nextLevel > level) {
            for (i = level; i < nextLevel; i++) {
               Doxygen::indexList->incContentsDepth();
            }

         } else if (nextLevel < level) {
            for (i = nextLevel; i < level; i++) {
               Doxygen::indexList->decContentsDepth();
            }
         }

         QByteArray title = si->title;
         if (title.isEmpty()) {
            title = si->label;
         }

         Doxygen::indexList->addContentsItem(true, title, getReference(), getOutputFileBase(),
                                             si->label, false, true);
         level = nextLevel;
      }
   }

   while (level > 1) {
      Doxygen::indexList->decContentsDepth();
      level--;
   }
}

void Definition::writeDocAnchorsToTagFile(FTextStream &tagFile)
{
   if (m_impl->sectionDict) {
      //printf("%s: writeDocAnchorsToTagFile(%d)\n",name().data(),m_impl->sectionDict->count());

      StringMap<QSharedPointer<SectionInfo>>::Iterator sdi(*m_impl->sectionDict);
      SectionInfo *si;

      for (; (si = sdi.current()); ++sdi) {

         if (!si->generated) {
            //printf("write an entry!\n");
            if (definitionType() == TypeMember) {
               tagFile << "  ";
            }

            tagFile << "    <docanchor file=\"" << si->fileName << "\"";
            if (!si->title.isEmpty()) {
               tagFile << " title=\"" << convertToXML(si->title) << "\"";
            }

            tagFile << ">" << si->label << "</docanchor>" << endl;
         }
      }
   }
}

bool Definition::_docsAlreadyAdded(const QByteArray &doc, QByteArray &sigList)
{  
   // to avoid mismatches due to differences in indenting, we first remove
   // double whitespaces...

   QByteArray docStr = doc.simplified();

   QByteArray sigStr;
   sigStr = QCryptographicHash::hash(docStr, QCryptographicHash::Md5).toHex();  

   if (sigList.find(sigStr) == -1) { // new docs, add signature to prevent re-adding it
      sigList += ":" + sigStr;
      return false;

   } else {
      return true;

   }
}

void Definition::_setDocumentation(const char *d, const char *docFile, int docLine,
                                   bool stripWhiteSpace, bool atTop)
{
   //printf("%s::setDocumentation(%s,%s,%d,%d)\n",name().data(),d,docFile,docLine,stripWhiteSpace);

   if (d == 0) {
      return;
   }

   QByteArray doc = d;

   if (stripWhiteSpace) {
      doc = stripLeadingAndTrailingEmptyLines(doc, docLine);
   } else { // don't strip whitespace
      doc = d;
   }

   if (!_docsAlreadyAdded(doc, m_impl->docSignatures)) {
      //printf("setting docs for %s: `%s'\n",name().data(),m_doc.data());
      if (m_impl->details == 0) {
         m_impl->details = new DocInfo;
      }
      if (m_impl->details->doc.isEmpty()) { // fresh detailed description
         m_impl->details->doc = doc;
      } else if (atTop) { // another detailed description, append it to the start
         m_impl->details->doc = doc + "\n\n" + m_impl->details->doc;
      } else { // another detailed description, append it to the end
         m_impl->details->doc += "\n\n" + doc;
      }
      if (docLine != -1) { // store location if valid
         m_impl->details->file = docFile;
         m_impl->details->line = docLine;
      } else {
         m_impl->details->file = docFile;
         m_impl->details->line = 1;
      }
   }
}

void Definition::setDocumentation(const char *d, const char *docFile, int docLine, bool stripWhiteSpace)
{
   if (d == 0) {
      return;
   }
   _setDocumentation(d, docFile, docLine, stripWhiteSpace, false);
}

#define uni_isupper(c) (QChar(c).category()==QChar::Letter_Uppercase)

// do a UTF-8 aware search for the last real character and return true
// if that is a multibyte one.
static bool lastCharIsMultibyte(const QByteArray &s)
{
   int l = s.length();
   int p = 0;
   int pp = -1;
   while ((p = nextUtf8CharPosition(s, l, p)) < l) {
      pp = p;
   }
   if (pp == -1 || ((uchar)s[pp]) < 0x80) {
      return false;
   }
   return true;
}

void Definition::_setBriefDescription(const char *b, const char *briefFile, int briefLine)
{
   static QByteArray outputLanguage = Config_getEnum("OUTPUT_LANGUAGE");
   static bool needsDot = outputLanguage != "Japanese" &&
                          outputLanguage != "Chinese" &&
                          outputLanguage != "Korean";
   QByteArray brief = b;
   brief = brief.trimmed();
   if (brief.isEmpty()) {
      return;
   }
   int bl = brief.length();
   if (bl > 0 && needsDot) { // add punctuation if needed
      int c = brief.at(bl - 1);
      switch (c) {
         case '.':
         case '!':
         case '?':
         case '>':
         case ':':
         case ')':
            break;
         default:
            if (uni_isupper(brief.at(0)) && !lastCharIsMultibyte(brief)) {
               brief += '.';
            }
            break;
      }
   }

   if (!_docsAlreadyAdded(brief, m_impl->briefSignatures)) {
      if (m_impl->brief && !m_impl->brief->doc.isEmpty()) {
         //printf("adding to details\n");
         _setDocumentation(brief, briefFile, briefLine, false, true);
      } else {
         //fprintf(stderr,"Definition::setBriefDescription(%s,%s,%d)\n",b,briefFile,briefLine);
         if (m_impl->brief == 0) {
            m_impl->brief = new BriefInfo;
         }
         m_impl->brief->doc = brief;
         if (briefLine != -1) {
            m_impl->brief->file = briefFile;
            m_impl->brief->line = briefLine;
         } else {
            m_impl->brief->file = briefFile;
            m_impl->brief->line = 1;
         }
      }
   } else {
      //printf("do nothing!\n");
   }
}

void Definition::setBriefDescription(const char *b, const char *briefFile, int briefLine)
{
   if (b == 0) {
      return;
   }
   _setBriefDescription(b, briefFile, briefLine);
}

void Definition::_setInbodyDocumentation(const char *doc, const char *inbodyFile, int inbodyLine)
{
   if (m_impl->inbodyDocs == 0) {
      m_impl->inbodyDocs = new DocInfo;
   }
   if (m_impl->inbodyDocs->doc.isEmpty()) { // fresh inbody docs
      m_impl->inbodyDocs->doc  = doc;
      m_impl->inbodyDocs->file = inbodyFile;
      m_impl->inbodyDocs->line = inbodyLine;
   } else { // another inbody documentation fragment, append this to the end
      m_impl->inbodyDocs->doc += QByteArray("\n\n") + doc;
   }
}

void Definition::setInbodyDocumentation(const char *d, const char *inbodyFile, int inbodyLine)
{
   if (d == 0) {
      return;
   }
   _setInbodyDocumentation(d, inbodyFile, inbodyLine);
}

/*! Reads a fragment of code from file \a fileName starting at
 * line \a startLine and ending at line \a endLine (inclusive). The fragment is
 * stored in \a result. If false is returned the code fragment could not be
 * found.
 *
 * The file is scanned for a opening bracket ('{') from \a startLine onward
 * The line actually containing the bracket is returned via startLine.
 * The file is scanned for a closing bracket ('}') from \a endLine backward.
 * The line actually containing the bracket is returned via endLine.
 * Note that for VHDL code the bracket search is not done.
 */
bool readCodeFragment(const char *fileName,
                      int &startLine, int &endLine, QByteArray &result)
{
   static bool filterSourceFiles = Config_getBool("FILTER_SOURCE_FILES");
   static int tabSize = Config_getInt("TAB_SIZE");
   //printf("readCodeFragment(%s,%d,%d)\n",fileName,startLine,endLine);
   if (fileName == 0 || fileName[0] == 0) {
      return false;   // not a valid file name
   }
   QByteArray filter = getFileFilter(fileName, true);
   FILE *f = 0;
   bool usePipe = !filter.isEmpty() && filterSourceFiles;
   SrcLangExt lang = getLanguageFromFileName(fileName);
   if (!usePipe) { // no filter given or wanted
      f = portable_fopen(fileName, "r");
   } else { // use filter
      QByteArray cmd = filter + " \"" + fileName + "\"";
      Debug::print(Debug::ExtCmd, 0, "Executing popen(`%s`)\n", cmd.data());
      f = portable_popen(cmd, "r");
   }
   bool found = lang == SrcLangExt_VHDL   ||
                lang == SrcLangExt_Tcl    ||
                lang == SrcLangExt_Python ||
                lang == SrcLangExt_Fortran;
   // for VHDL, TCL, Python, and Fortran no bracket search is possible
   if (f) {
      int c = 0;
      int col = 0;
      int lineNr = 1;
      // skip until the startLine has reached
      while (lineNr < startLine && !feof(f)) {
         while ((c = fgetc(f)) != '\n' && c != EOF) /* skip */;
         lineNr++;
      }
      if (!feof(f)) {
         // skip until the opening bracket or lonely : is found
         char cn = 0;
         while (lineNr <= endLine && !feof(f) && !found) {
            int pc = 0;
            while ((c = fgetc(f)) != '{' && c != ':' && c != EOF) {
               //printf("parsing char `%c'\n",c);
               if (c == '\n') {
                  lineNr++, col = 0;
               } else if (c == '\t') {
                  col += tabSize - (col % tabSize);
               } else if (pc == '/' && c == '/') { // skip single line comment
                  while ((c = fgetc(f)) != '\n' && c != EOF) {
                     pc = c;
                  }
                  if (c == '\n') {
                     lineNr++, col = 0;
                  }
               } else if (pc == '/' && c == '*') { // skip C style comment
                  while (((c = fgetc(f)) != '/' || pc != '*') && c != EOF) {
                     if (c == '\n') {
                        lineNr++, col = 0;
                     }
                     pc = c;
                  }
               } else {
                  col++;
               }
               pc = c;
            }
            if (c == ':') {
               cn = fgetc(f);
               if (cn != ':') {
                  found = true;
               }
            } else if (c == '{') { // } so vi matching brackets has no problem
               found = true;
            }
         }
         //printf(" -> readCodeFragment(%s,%d,%d) lineNr=%d\n",fileName,startLine,endLine,lineNr);
         if (found) {
            // For code with more than one line,
            // fill the line with spaces until we are at the right column
            // so that the opening brace lines up with the closing brace
            if (endLine != startLine) {
               QByteArray spaces;
               spaces.fill(' ', col);
               result += spaces;
            }
            // copy until end of line
            result += c;
            startLine = lineNr;
            if (c == ':') {
               result += cn;
               if (cn == '\n') {
                  lineNr++;
               }
            }
            const int maxLineLength = 4096;
            char lineStr[maxLineLength];
            do {
               //printf("reading line %d in range %d-%d\n",lineNr,startLine,endLine);
               int size_read;
               do {
                  // read up to maxLineLength-1 bytes, the last byte being zero
                  char *p = fgets(lineStr, maxLineLength, f);
                  //printf("  read %s",p);
                  if (p) {
                     size_read = qstrlen(p);
                  } else { // nothing read
                     size_read = -1;
                     lineStr[0] = '\0';
                  }
                  result += lineStr;
               } while (size_read == (maxLineLength - 1));

               lineNr++;
            } while (lineNr <= endLine && !feof(f));

            // strip stuff after closing bracket
            int newLineIndex = result.lastIndexOf('\n');
            int braceIndex   = result.lastIndexOf('}');

            if (braceIndex > newLineIndex) {
               result.truncate(braceIndex + 1);
            }

            endLine = lineNr - 1;
         }
      }
      if (usePipe) {
         portable_pclose(f);
         Debug::print(Debug::FilterOutput, 0, "Filter output\n");
         Debug::print(Debug::FilterOutput, 0, "-------------\n%s\n-------------\n", result.data());
      } else {
         fclose(f);
      }
   }
   result = transcodeCharacterStringToUTF8(result);
   //fprintf(stderr,"readCodeFragement(%d-%d)=%s\n",startLine,endLine,result.data());
   return found;
}

QByteArray Definition::getSourceFileBase() const
{
   assert(definitionType() != Definition::TypeFile); // file overloads this method
   QByteArray fn;
   static bool sourceBrowser = Config_getBool("SOURCE_BROWSER");
   if (sourceBrowser &&
         m_impl->body && m_impl->body->startLine != -1 && m_impl->body->fileDef) {
      fn = m_impl->body->fileDef->getSourceFileBase();
   }
   return fn;
}

QByteArray Definition::getSourceAnchor() const
{
   const int maxAnchorStrLen = 20;
   char anchorStr[maxAnchorStrLen];
   if (m_impl->body && m_impl->body->startLine != -1) {
      if (Htags::useHtags) {
         qsnprintf(anchorStr, maxAnchorStrLen, "L%d", m_impl->body->startLine);
      } else {
         qsnprintf(anchorStr, maxAnchorStrLen, "l%05d", m_impl->body->startLine);
      }
   }
   return anchorStr;
}

/*! Write a reference to the source code defining this definition */
void Definition::writeSourceDef(OutputList &ol, const char *)
{
   static bool latexSourceCode = Config_getBool("LATEX_SOURCE_CODE");
   ol.pushGeneratorState();
   //printf("Definition::writeSourceRef %d %p\n",bodyLine,bodyDef);
   QByteArray fn = getSourceFileBase();
   if (!fn.isEmpty()) {
      QByteArray refText = theTranslator->trDefinedAtLineInSourceFile();
      int lineMarkerPos = refText.find("@0");
      int fileMarkerPos = refText.find("@1");
      if (lineMarkerPos != -1 && fileMarkerPos != -1) { // should always pass this.
         QByteArray lineStr;
         lineStr.sprintf("%d", m_impl->body->startLine);
         QByteArray anchorStr = getSourceAnchor();
         ol.startParagraph();
         if (lineMarkerPos < fileMarkerPos) { // line marker before file marker
            // write text left from linePos marker
            ol.parseText(refText.left(lineMarkerPos));
            ol.pushGeneratorState();
            ol.disable(OutputGenerator::RTF);
            ol.disable(OutputGenerator::Man);
            if (!latexSourceCode) {
               ol.disable(OutputGenerator::Latex);
            }
            // write line link (HTML, LaTeX optionally)
            ol.writeObjectLink(0, fn, anchorStr, lineStr);
            ol.enableAll();
            ol.disable(OutputGenerator::Html);
            if (latexSourceCode) {
               ol.disable(OutputGenerator::Latex);
            }
            // write normal text (Man/RTF, Latex optionally)
            ol.docify(lineStr);
            ol.popGeneratorState();

            // write text between markers
            ol.parseText(refText.mid(lineMarkerPos + 2,
                                     fileMarkerPos - lineMarkerPos - 2));

            ol.pushGeneratorState();
            ol.disable(OutputGenerator::RTF);
            ol.disable(OutputGenerator::Man);
            if (!latexSourceCode) {
               ol.disable(OutputGenerator::Latex);
            }
            // write file link (HTML, LaTeX optionally)
            ol.writeObjectLink(0, fn, 0, m_impl->body->fileDef->name());
            ol.enableAll();
            ol.disable(OutputGenerator::Html);
            if (latexSourceCode) {
               ol.disable(OutputGenerator::Latex);
            }
            // write normal text (Man/RTF, Latex optionally)
            ol.docify(m_impl->body->fileDef->name());
            ol.popGeneratorState();

            // write text right from file marker
            ol.parseText(refText.right(
                            refText.length() - fileMarkerPos - 2));
         } else { // file marker before line marker
            // write text left from file marker
            ol.parseText(refText.left(fileMarkerPos));
            ol.pushGeneratorState();
            ol.disable(OutputGenerator::RTF);
            ol.disable(OutputGenerator::Man);
            if (!latexSourceCode) {
               ol.disable(OutputGenerator::Latex);
            }
            // write file link (HTML only)
            ol.writeObjectLink(0, fn, 0, m_impl->body->fileDef->name());
            ol.enableAll();
            ol.disable(OutputGenerator::Html);
            if (latexSourceCode) {
               ol.disable(OutputGenerator::Latex);
            }
            // write normal text (Latex/Man only)
            ol.docify(m_impl->body->fileDef->name());
            ol.popGeneratorState();

            // write text between markers
            ol.parseText(refText.mid(fileMarkerPos + 2,
                                     lineMarkerPos - fileMarkerPos - 2));

            ol.pushGeneratorState();
            ol.disable(OutputGenerator::RTF);
            ol.disable(OutputGenerator::Man);
            if (!latexSourceCode) {
               ol.disable(OutputGenerator::Latex);
            }
            ol.disableAllBut(OutputGenerator::Html);
            // write line link (HTML only)
            ol.writeObjectLink(0, fn, anchorStr, lineStr);
            ol.enableAll();
            ol.disable(OutputGenerator::Html);
            if (latexSourceCode) {
               ol.disable(OutputGenerator::Latex);
            }
            // write normal text (Latex/Man only)
            ol.docify(lineStr);
            ol.popGeneratorState();

            // write text right from linePos marker
            ol.parseText(refText.right(
                            refText.length() - lineMarkerPos - 2));
         }
         ol.endParagraph();
      } else {
         err("translation error: invalid markers in trDefinedInSourceFile()\n");
      }
   }
   ol.popGeneratorState();
}

void Definition::setBodySegment(int bls, int ble)
{
   //printf("setBodySegment(%d,%d) for %s\n",bls,ble,name().data());
   if (m_impl->body == 0) {
      m_impl->body = new BodyInfo;
   }
   m_impl->body->startLine = bls;
   m_impl->body->endLine = ble;
}

void Definition::setBodyDef(FileDef *fd)
{
   if (m_impl->body == 0) {
      m_impl->body = new BodyInfo;
   }
   m_impl->body->fileDef = fd;
}

bool Definition::hasSources() const
{
   return m_impl->body && m_impl->body->startLine != -1 &&
          m_impl->body->endLine >= m_impl->body->startLine &&
          m_impl->body->fileDef;
}

/*! Write code of this definition into the documentation */
void Definition::writeInlineCode(OutputList &ol, const char *scopeName)
{
   static bool inlineSources = Config_getBool("INLINE_SOURCES");
   ol.pushGeneratorState();
   //printf("Source Fragment %s: %d-%d bodyDef=%p\n",name().data(),
   //        m_startBodyLine,m_endBodyLine,m_bodyDef);
   if (inlineSources && hasSources()) {
      QByteArray codeFragment;
      int actualStart = m_impl->body->startLine, actualEnd = m_impl->body->endLine;
      if (readCodeFragment(m_impl->body->fileDef->absoluteFilePath(),
                           actualStart, actualEnd, codeFragment)
         ) {
         //printf("Adding code fragement '%s' ext='%s'\n",
         //    codeFragment.data(),m_impl->defFileExt.data());
         ParserInterface *pIntf = Doxygen::parserManager->getParser(m_impl->defFileExt);
         pIntf->resetCodeParserState();
         //printf("Read:\n`%s'\n\n",codeFragment.data());
         MemberDef *thisMd = 0;
         if (definitionType() == TypeMember) {
            thisMd = (MemberDef *)this;
         }

         ol.startCodeFragment();
         pIntf->parseCode(ol,               // codeOutIntf
                          scopeName,        // scope
                          codeFragment,     // input
                          m_impl->lang,     // lang
                          false,            // isExample
                          0,                // exampleName
                          m_impl->body->fileDef,  // fileDef
                          actualStart,      // startLine
                          actualEnd,        // endLine
                          true,             // inlineFragment
                          thisMd,           // memberDef
                          true              // show line numbers
                         );
         ol.endCodeFragment();
      }
   }
   ol.popGeneratorState();
}

/*! Write a reference to the source code fragments in which this
 *  definition is used.
 */
void Definition::_writeSourceRefList(OutputList &ol, const char *scopeName,
                                     const QByteArray &text, MemberSDict *members, bool /*funcOnly*/)
{
   static bool latexSourceCode = Config_getBool("LATEX_SOURCE_CODE");
   static bool sourceBrowser   = Config_getBool("SOURCE_BROWSER");
   static bool refLinkSource   = Config_getBool("REFERENCES_LINK_SOURCE");
   ol.pushGeneratorState();
   if (members) {
      members->sort();

      ol.startParagraph();
      ol.parseText(text);
      ol.docify(" ");

      QByteArray ldefLine = theTranslator->trWriteList(members->count());

      QRegExp marker("@[0-9]+");
      int index = 0, newIndex, matchLen;

      // now replace all markers in inheritLine with links to the classes
      while ((newIndex = marker.match(ldefLine, index, &matchLen)) != -1) {
         bool ok;
         ol.parseText(ldefLine.mid(index, newIndex - index));
         uint entryIndex = ldefLine.mid(newIndex + 1, matchLen - 1).toUInt(&ok);
         MemberDef *md = members->at(entryIndex);

         if (ok && md) {
            QByteArray scope = md->getScopeString();
            QByteArray name = md->name();

            //printf("class=%p scope=%s scopeName=%s\n",md->getClassDef(),scope.data(),scopeName);
            if (!scope.isEmpty() && scope != scopeName) {
               name.prepend(scope + getLanguageSpecificSeparator(m_impl->lang));
            }
            if (!md->isObjCMethod() &&
                  (md->isFunction() || md->isSlot() ||
                   md->isPrototype() || md->isSignal()
                  )
               ) {
               name += "()";
            }
            //Definition *d = md->getOutputFileBase();
            //if (d==Doxygen::globalScope) d=md->getBodyDef();
            if (sourceBrowser &&
                  !(md->isLinkable() && !refLinkSource) &&
                  md->getStartBodyLine() != -1 &&
                  md->getBodyDef()
               ) {
               //printf("md->getBodyDef()=%p global=%p\n",md->getBodyDef(),Doxygen::globalScope);
               // for HTML write a real link
               ol.pushGeneratorState();
               //ol.disableAllBut(OutputGenerator::Html);

               ol.disable(OutputGenerator::RTF);
               ol.disable(OutputGenerator::Man);
               if (!latexSourceCode) {
                  ol.disable(OutputGenerator::Latex);
               }
               const int maxLineNrStr = 10;
               char anchorStr[maxLineNrStr];
               qsnprintf(anchorStr, maxLineNrStr, "l%05d", md->getStartBodyLine());
               //printf("Write object link to %s\n",md->getBodyDef()->getSourceFileBase().data());
               ol.writeObjectLink(0, md->getBodyDef()->getSourceFileBase(), anchorStr, name);
               ol.popGeneratorState();

               // for the other output formats just mention the name
               ol.pushGeneratorState();
               ol.disable(OutputGenerator::Html);
               if (latexSourceCode) {
                  ol.disable(OutputGenerator::Latex);
               }
               ol.docify(name);
               ol.popGeneratorState();
            } else if (md->isLinkable() /*&& d && d->isLinkable()*/) {
               // for HTML write a real link
               ol.pushGeneratorState();
               //ol.disableAllBut(OutputGenerator::Html);
               ol.disable(OutputGenerator::RTF);
               ol.disable(OutputGenerator::Man);
               if (!latexSourceCode) {
                  ol.disable(OutputGenerator::Latex);
               }

               ol.writeObjectLink(md->getReference(),
                                  md->getOutputFileBase(),
                                  md->anchor(), name);
               ol.popGeneratorState();

               // for the other output formats just mention the name
               ol.pushGeneratorState();
               ol.disable(OutputGenerator::Html);
               if (latexSourceCode) {
                  ol.disable(OutputGenerator::Latex);
               }
               ol.docify(name);
               ol.popGeneratorState();
            } else {
               ol.docify(name);
            }
         }
         index = newIndex + matchLen;
      }
      ol.parseText(ldefLine.right(ldefLine.length() - index));
      ol.writeString(".");
      ol.endParagraph();
   }
   ol.popGeneratorState();
}

void Definition::writeSourceReffedBy(OutputList &ol, const char *scopeName)
{
   if (Config_getBool("REFERENCED_BY_RELATION")) {
      _writeSourceRefList(ol, scopeName, theTranslator->trReferencedBy(), m_impl->sourceRefByDict, false);
   }
}

void Definition::writeSourceRefs(OutputList &ol, const char *scopeName)
{
   if (Config_getBool("REFERENCES_RELATION")) {
      _writeSourceRefList(ol, scopeName, theTranslator->trReferences(), m_impl->sourceRefsDict, true);
   }
}

bool Definition::hasDocumentation() const
{
   static bool extractAll    = Config_getBool("EXTRACT_ALL");
   //static bool sourceBrowser = Config_getBool("SOURCE_BROWSER");
   bool hasDocs =
      (m_impl->details    && !m_impl->details->doc.isEmpty())    || // has detailed docs
      (m_impl->brief      && !m_impl->brief->doc.isEmpty())      || // has brief description
      (m_impl->inbodyDocs && !m_impl->inbodyDocs->doc.isEmpty()) || // has inbody docs
      extractAll //||                   // extract everything
      //       (sourceBrowser && m_impl->body &&
      //        m_impl->body->startLine!=-1 && m_impl->body->fileDef)
      ; // link to definition
   return hasDocs;
}

bool Definition::hasUserDocumentation() const
{
   bool hasDocs =
      (m_impl->details    && !m_impl->details->doc.isEmpty()) ||
      (m_impl->brief      && !m_impl->brief->doc.isEmpty())   ||
      (m_impl->inbodyDocs && !m_impl->inbodyDocs->doc.isEmpty());
   return hasDocs;
}


void Definition::addSourceReferencedBy(MemberDef *md)
{
   if (md) {
      QByteArray name  = md->name();
      QByteArray scope = md->getScopeString();

      if (!scope.isEmpty()) {
         name.prepend(scope + "::");
      }

      if (m_impl->sourceRefByDict == 0) {
         m_impl->sourceRefByDict = new MemberSDict;
      }
      if (m_impl->sourceRefByDict->find(name) == 0) {
         m_impl->sourceRefByDict->append(name, md);
      }
   }
}

void Definition::addSourceReferences(MemberDef *md)
{
   QByteArray name  = md->name();
   QByteArray scope = md->getScopeString();

   if (md) {
      QByteArray name  = md->name();
      QByteArray scope = md->getScopeString();

      if (!scope.isEmpty()) {
         name.prepend(scope + "::");
      }

      if (m_impl->sourceRefsDict == 0) {
         m_impl->sourceRefsDict = new MemberSDict;
      }
      if (m_impl->sourceRefsDict->find(name) == 0) {
         m_impl->sourceRefsDict->append(name, md);
      }
   }
}

Definition *Definition::findInnerCompound(const char *)
{
   return 0;
}

QByteArray Definition::qualifiedName() const
{
   //static int count=0;
   //count++;
   if (!m_impl->qualifiedName.isEmpty()) {
      //count--;
      return m_impl->qualifiedName;
   }

   //printf("start %s::qualifiedName() localName=%s\n",name().data(),m_impl->localName.data());
   if (m_impl->outerScope == 0) {
      if (m_impl->localName == "<globalScope>") {
         //count--;
         return "";
      } else {
         //count--;
         return m_impl->localName;
      }
   }

   if (m_impl->outerScope->name() == "<globalScope>") {
      m_impl->qualifiedName = m_impl->localName;

   } else {
      m_impl->qualifiedName = m_impl->outerScope->qualifiedName() + getLanguageSpecificSeparator(getLanguage()) + m_impl->localName;
   }
   
   return m_impl->qualifiedName;
}

void Definition::setOuterScope(Definition *d)
{
   //printf("%s::setOuterScope(%s)\n",name().data(),d?d->name().data():"<none>");
   Definition *p = m_impl->outerScope;
   bool found = false;
   // make sure that we are not creating a recursive scope relation.
   while (p && !found) {
      found = (p == d);
      p = p->m_impl->outerScope;
   }
   if (!found) {
      m_impl->qualifiedName.resize(0); // flush cached scope name
      m_impl->outerScope = d;
   }
   m_impl->hidden = m_impl->hidden || d->isHidden();
}

QByteArray Definition::localName() const
{
   return m_impl->localName;
}

void Definition::makePartOfGroup(GroupDef *gd)
{
   if (m_impl->partOfGroups == 0) {
      m_impl->partOfGroups = new SortedList<GroupDef *>;
   }

   m_impl->partOfGroups->append(gd);
}

void Definition::setRefItems(const QList<ListItemInfo> *sli)
{
   
   if (sli) {
      // deep copy the list
      if (m_impl->xrefListItems == 0) {
         m_impl->xrefListItems = new QList<ListItemInfo>;        
      }

      QListIterator<ListItemInfo> slii(*sli);
      ListItemInfo *lii;

      for (slii.toFirst(); (lii = slii.current()); ++slii) {
         m_impl->xrefListItems->append(new ListItemInfo(*lii));
      }
   }
}

void Definition::mergeRefItems(Definition *d)
{
   //printf("%s::mergeRefItems()\n",name().data());
   QList<ListItemInfo> *xrefList = d->xrefListItems();

   if (xrefList != 0) {
      // deep copy the list
      if (m_impl->xrefListItems == 0) {
         m_impl->xrefListItems = new QList<ListItemInfo>;         
      }

      QListIterator<ListItemInfo> slii(*xrefList);
      ListItemInfo *lii;

      for (slii.toFirst(); (lii = slii.current()); ++slii) {
         if (_getXRefListId(lii->type) == -1) {
            m_impl->xrefListItems->append(new ListItemInfo(*lii));
         }
      }
   }
}

int Definition::_getXRefListId(const char *listName) const
{
   if (m_impl->xrefListItems) {
      QListIterator<ListItemInfo> slii(*m_impl->xrefListItems);
      ListItemInfo *lii;
      for (slii.toFirst(); (lii = slii.current()); ++slii) {
         if (qstrcmp(lii->type, listName) == 0) {
            return lii->itemId;
         }
      }
   }
   return -1;
}

QList<ListItemInfo> *Definition::xrefListItems() const
{
   return m_impl->xrefListItems;
}


QByteArray Definition::convertNameToFile(const char *name, bool allowDots) const
{
   if (!m_impl->ref.isEmpty()) {
      return name;
   } else {
      return ::convertNameToFile(name, allowDots);
   }
}

QByteArray Definition::pathFragment() const
{
   QByteArray result;
   if (m_impl->outerScope && m_impl->outerScope != Doxygen::globalScope) {
      result = m_impl->outerScope->pathFragment();
   }
   if (isLinkable()) {
      if (!result.isEmpty()) {
         result += "/";
      }
      if (definitionType() == Definition::TypeGroup && ((const GroupDef *)this)->groupTitle()) {
         result += ((const GroupDef *)this)->groupTitle();
      } else if (definitionType() == Definition::TypePage && !((const PageDef *)this)->title().isEmpty()) {
         result += ((const PageDef *)this)->title();
      } else {
         result += m_impl->localName;
      }
   } else {
      result += m_impl->localName;
   }
   return result;
}

//----------------------------------------------------------------------------------------

// TODO: move to htmlgen
/*! Returns the string used in the footer for $navpath when
 *  GENERATE_TREEVIEW is enabled
 */
QByteArray Definition::navigationPathAsString() const
{
   QByteArray result;
   Definition *outerScope = getOuterScope();
   QByteArray locName = localName();
   if (outerScope && outerScope != Doxygen::globalScope) {
      result += outerScope->navigationPathAsString();
   } else if (definitionType() == Definition::TypeFile && ((const FileDef *)this)->getDirDef()) {
      result += ((const FileDef *)this)->getDirDef()->navigationPathAsString();
   }
   result += "<li class=\"navelem\">";
   if (isLinkable()) {
      if (definitionType() == Definition::TypeGroup && ((const GroupDef *)this)->groupTitle()) {
         result += "<a class=\"el\" href=\"$relpath^" + getOutputFileBase() + Doxygen::htmlFileExtension + "\">" +
                   ((const GroupDef *)this)->groupTitle() + "</a>";
      } else if (definitionType() == Definition::TypePage && !((const PageDef *)this)->title().isEmpty()) {
         result += "<a class=\"el\" href=\"$relpath^" + getOutputFileBase() + Doxygen::htmlFileExtension + "\">" +
                   ((const PageDef *)this)->title() + "</a>";
      } else if (definitionType() == Definition::TypeClass) {
         QByteArray name = locName;
         if (name.right(2) == "-p" /*|| name.right(2)=="-g"*/) {
            name = name.left(name.length() - 2);
         }
         result += "<a class=\"el\" href=\"$relpath^" + getOutputFileBase() + Doxygen::htmlFileExtension;
         if (!anchor().isEmpty()) {
            result += "#" + anchor();
         }
         result += "\">" + name + "</a>";
      } else {
         result += "<a class=\"el\" href=\"$relpath^" + getOutputFileBase() + Doxygen::htmlFileExtension + "\">" +
                   locName + "</a>";
      }
   } else {
      result += "<b>" + locName + "</b>";
   }
   result += "</li>";
   return result;
}

// TODO: move to htmlgen
void Definition::writeNavigationPath(OutputList &ol) const
{
   ol.pushGeneratorState();
   ol.disableAllBut(OutputGenerator::Html);

   QByteArray navPath;
   navPath += "<div id=\"nav-path\" class=\"navpath\">\n"
              "  <ul>\n";
   navPath += navigationPathAsString();
   navPath += "  </ul>\n"
              "</div>\n";
   ol.writeNavigationPath(navPath);

   ol.popGeneratorState();
}

// TODO: move to htmlgen
void Definition::writeToc(OutputList &ol)
{
   SectionDict *sectionDict = m_impl->sectionDict;
   if (sectionDict == 0) {
      return;
   }
   ol.pushGeneratorState();
   ol.disableAllBut(OutputGenerator::Html);
   ol.writeString("<div class=\"toc\">");
   ol.writeString("<h3>");
   ol.writeString(theTranslator->trRTFTableOfContents());
   ol.writeString("</h3>\n");
   ol.writeString("<ul>");

   StringMap<QSharedPointer<SectionInfo>>::Iterator li(*sectionDict);
   SectionInfo *si;

   int level = 1, l;
   char cs[2];
   cs[1] = '\0';

   bool inLi[5] = { false, false, false, false };

   for (li.toFirst(); (si = li.current()); ++li) {
      if (si->type == SectionInfo::Section       ||
            si->type == SectionInfo::Subsection    ||
            si->type == SectionInfo::Subsubsection ||
            si->type == SectionInfo::Paragraph) {
         //printf("  level=%d title=%s\n",level,si->title.data());
         int nextLevel = (int)si->type;
         if (nextLevel > level) {
            for (l = level; l < nextLevel; l++) {
               ol.writeString("<ul>");
            }
         } else if (nextLevel < level) {
            for (l = level; l > nextLevel; l--) {
               if (inLi[l]) {
                  ol.writeString("</li>\n");
               }
               inLi[l] = false;
               ol.writeString("</ul>\n");
            }
         }
         cs[0] = '0' + nextLevel;
         if (inLi[nextLevel]) {
            ol.writeString("</li>\n");
         }
         ol.writeString("<li class=\"level" + QByteArray(cs) + "\"><a href=\"#" + si->label + "\">" + (si->title.isEmpty() ? si->label : si->title) + "</a>");
         inLi[nextLevel] = true;
         level = nextLevel;
      }
   }
   while (level > 1) {
      if (inLi[level]) {
         ol.writeString("</li>\n");
      }
      inLi[level] = false;
      ol.writeString("</ul>\n");
      level--;
   }
   if (inLi[level]) {
      ol.writeString("</li>\n");
   }
   inLi[level] = false;
   ol.writeString("</ul>\n");
   ol.writeString("</div>\n");
   ol.popGeneratorState();
}

//----------------------------------------------------------------------------------------


QByteArray Definition::symbolName() const
{
   return m_symbolName;
}

//----------------------

QByteArray Definition::documentation() const
{
   return m_impl->details ? m_impl->details->doc : QByteArray("");
}

int Definition::docLine() const
{
   return m_impl->details ? m_impl->details->line : 1;
}

QByteArray Definition::docFile() const
{
   return m_impl->details ? m_impl->details->file : QByteArray("<" + m_name + ">");
}

//----------------------------------------------------------------------------
// strips w from s iff s starts with w
static bool stripWord(QByteArray &s, QByteArray w)
{
   bool success = false;
   if (s.left(w.length()) == w) {
      success = true;
      s = s.right(s.length() - w.length());
   }
   return success;
}

//----------------------------------------------------------------------------
// some quasi intelligent brief description abbreviator :^)
QByteArray abbreviate(const char *s, const char *name)
{
   QByteArray scopelessName = name;
   int i = scopelessName.lastIndexOf("::");

   if (i != -1) {
      scopelessName = scopelessName.mid(i + 2);
   }

   QByteArray result = s;
   result = result.trimmed();

   // strip trailing .
   if (!result.isEmpty() && result.at(result.length() - 1) == '.') {
      result = result.left(result.length() - 1);
   }

   // strip any predefined prefix
   QStringList &briefDescAbbrev = Config_getList("ABBREVIATE_BRIEF");
   const char *p = briefDescAbbrev.first();

   while (p) {
      QByteArray s = p;
      s.replace(QRegExp("\\$name"), scopelessName);     // replace $name with entity name
      s += " ";
      stripWord(result, s);
      p = briefDescAbbrev.next();
   }

   // capitalize first word
   if (!result.isEmpty()) {
      int c = result[0];
      if (c >= 'a' && c <= 'z') {
         c += 'A' - 'a';
      }
      result[0] = c;
   }
   return result;
}


//----------------------

QByteArray Definition::briefDescription(bool abbr) const
{
   //printf("%s::briefDescription(%d)='%s'\n",name().data(),abbr,m_impl->brief?m_impl->brief->doc.data():"<none>");
   return m_impl->brief ?
          (abbr ? abbreviate(m_impl->brief->doc, displayName()) : m_impl->brief->doc) :
          QByteArray("");
}

QByteArray Definition::briefDescriptionAsTooltip() const
{
   if (m_impl->brief) {
      if (m_impl->brief->tooltip.isEmpty() && !m_impl->brief->doc.isEmpty()) {
         static bool reentering = false;
         if (!reentering) {
            MemberDef *md = definitionType() == TypeMember ? (MemberDef *)this : 0;
            const Definition *scope = definitionType() == TypeMember ? getOuterScope() : this;
            reentering = true; // prevent requests for tooltips while parsing a tooltip
            m_impl->brief->tooltip = parseCommentAsText(
                                        scope, md,
                                        m_impl->brief->doc,
                                        m_impl->brief->file,
                                        m_impl->brief->line);
            reentering = false;
         }
      }
      return m_impl->brief->tooltip;
   }
   return QByteArray("");
}

int Definition::briefLine() const
{
   return m_impl->brief ? m_impl->brief->line : 1;
}

QByteArray Definition::briefFile() const
{
   return m_impl->brief ? m_impl->brief->file : QByteArray("<" + m_name + ">");
}

//----------------------

QByteArray Definition::inbodyDocumentation() const
{
   return m_impl->inbodyDocs ? m_impl->inbodyDocs->doc : QByteArray("");
}

int Definition::inbodyLine() const
{
   return m_impl->inbodyDocs ? m_impl->inbodyDocs->line : 1;
}

QByteArray Definition::inbodyFile() const
{
   return m_impl->inbodyDocs ? m_impl->inbodyDocs->file : QByteArray("<" + m_name + ">");
}


//----------------------

QByteArray Definition::getDefFileName() const
{
   return m_impl->defFileName;
}

QByteArray Definition::getDefFileExtension() const
{
   return m_impl->defFileExt;
}

bool Definition::isHidden() const
{
   return m_impl->hidden;
}

bool Definition::isVisibleInProject() const
{
   return isLinkableInProject() && !m_impl->hidden;
}

bool Definition::isVisible() const
{
   return isLinkable() && !m_impl->hidden;
}

bool Definition::isArtificial() const
{
   return m_impl->isArtificial;
}

QByteArray Definition::getReference() const
{
   return m_impl->ref;
}

bool Definition::isReference() const
{
   return !m_impl->ref.isEmpty();
}

int Definition::getStartBodyLine() const
{
   return m_impl->body ? m_impl->body->startLine : -1;
}

int Definition::getEndBodyLine() const
{
   return m_impl->body ? m_impl->body->endLine : -1;
}

FileDef *Definition::getBodyDef() const
{
   return m_impl->body ? m_impl->body->fileDef : 0;
}

SortedList<GroupDef *> *Definition::partOfGroups() const
{
   return m_impl->partOfGroups;
}

Definition *Definition::getOuterScope() const
{
   return m_impl->outerScope;
}

MemberSDict *Definition::getReferencesMembers() const
{
   return m_impl->sourceRefsDict;
}

MemberSDict *Definition::getReferencedByMembers() const
{
   return m_impl->sourceRefByDict;
}

void Definition::setReference(const char *r)
{
   m_impl->ref = r;
}

SrcLangExt Definition::getLanguage() const
{
   return m_impl->lang;
}

void Definition::setHidden(bool b)
{
   m_impl->hidden = m_impl->hidden || b;
}

void Definition::setArtificial(bool b)
{
   m_impl->isArtificial = b;
}

void Definition::setLocalName(const QByteArray name)
{
   m_impl->localName = name;
}

void Definition::setLanguage(SrcLangExt lang)
{
   m_impl->lang = lang;
}


void Definition::_setSymbolName(const QByteArray &name)
{
   m_symbolName = name;
}

bool Definition::hasBriefDescription() const
{
   static bool briefMemberDesc = Config_getBool("BRIEF_MEMBER_DESC");
   return !briefDescription().isEmpty() && briefMemberDesc;
}


