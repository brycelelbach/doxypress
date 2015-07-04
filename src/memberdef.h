/*************************************************************************
 *
 * Copyright (C) 2014-2015 Barbara Geller & Ansel Sermersheim documentation().
 * Copyright (C) 1997-2014 by Dimitri van Heesch.
 * All rights reserved.    
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation under the terms of the GNU General Public License version 2
 * is hereby granted. No representations are made about the suitability of
 * this software for any purpose. It is provided "as is" without express or
 * implied warranty. See the GNU General Public License for more details.
 *
 * Documents produced by DoxyPress are derivative works derived from the
 * input used in their production; they are not affected by this license.
 *
*************************************************************************/

#ifndef MEMBERDEF_H
#define MEMBERDEF_H

#include <QCryptographicHash>
#include <QList>
#include <QStringList>
#include <QTextStream>

#include <sys/types.h>
#include <types.h>

#include <definition.h>
#include <entry.h>

class ClassDef;
class NamespaceDef;
class GroupDef;
class FileDef;
class MemberList;
class MemberGroup;
class ExampleSDict;
class OutputList;
class GroupDef;
class ArgumentList;
class MemberDefImpl;

struct TagInfo;

/** A model of a class/file/namespace member symbol. */
class MemberDef : public Definition
{
 public:
   MemberDef(const char *defFileName, int defLine, int defColumn, const QString &type, const QString &name, 
             const QString &args, const QString &excp, Protection prot, Specifier virt, bool stat,
             Relationship related, MemberType t, const ArgumentList *tal, const ArgumentList *al);

   ~MemberDef();

   DefType definitionType() const override {
      return TypeMember;
   }

   // move this member into a different scope
   QSharedPointer<MemberDef> deepCopy() const;
   void moveTo(QSharedPointer<Definition> scope);
  
   // link id
   QString getOutputFileBase() const override;
   QByteArray getReference() const;
   QString anchor() const override;

   const char *declaration() const;
   const char *definition() const;
   const char *typeString() const;
   const char *argsString() const;
   const char *excpString() const;
   const char *bitfieldString() const;
   const char *extraTypeChars() const;
   const QByteArray &initializer() const;
   int initializerLines() const;
   Entry::SpecifierFlags getMemberSpecifiers() const;

   QSharedPointer<MemberList> getSectionList(QSharedPointer<Definition> d) const;
   QByteArray    displayDefinition() const;

   // scope query members
   QSharedPointer<ClassDef> getClassDef() const;
   QSharedPointer<FileDef>  getFileDef() const;
   QSharedPointer<NamespaceDef> getNamespaceDef() const;
   QSharedPointer<ClassDef> accessorClass() const;

   // property get
   const QByteArray &getPropertyRead() const;
   const QByteArray &getPropertyWrite() const;

   // copperspice - additional properties
   const QByteArray &getPropertyReset() const;
   const QByteArray &getPropertyNotify() const;

   // querying the grouping definition
   QSharedPointer<GroupDef> getGroupDef() const;
   Grouping::GroupPri_t getGroupPri() const;
   const char *getGroupFileName() const;
   int getGroupStartLine() const;
   bool getGroupHasDocs() const;
   QString qualifiedName() const;
   QByteArray objCMethodName(bool localLink, bool showStatic) const;

   // direct kind info
   Protection protection() const;
   Specifier virtualness(int count = 0) const;
   MemberType memberType() const;
   QByteArray   memberTypeName() const;

   // get methods
   bool isSignal() const;
   bool isSlot() const;
   bool isVariable() const;
   bool isEnumerate() const;
   bool isEnumValue() const;
   bool isTypedef() const;
   bool isFunction() const;
   bool isFunctionPtr() const;
   bool isDefine() const;
   bool isFriend() const;
   bool isDCOP() const;
   bool isProperty() const;
   bool isEvent() const;
   bool isRelated() const;
   bool isForeign() const;
   bool isStatic() const;
   bool isInline() const;
   bool isExplicit() const;
   bool isDeprecated() const;
   bool isMutable() const;
   bool isGettable() const;
   bool isPrivateGettable() const;
   bool isProtectedGettable() const;
   bool isSettable() const;
   bool isPrivateSettable() const;
   bool isProtectedSettable() const;

   bool isReadable() const;
   bool isWritable() const;
   bool isNotify() const;
   bool isReset() const;
   bool isRevision() const;
   bool isDesignable() const;
   bool isScriptable() const;
   bool isStored() const;
   bool isUser() const;
   bool isConstant() const;
   bool isFinal() const;
 
   bool isAddable() const;
   bool isRemovable() const;
   bool isRaisable() const;   
   bool isAbstract() const;
   bool isOverride() const;
   bool isInitonly() const;
   bool isOptional() const;
   bool isRequired() const;
   bool isNonAtomic() const;
   bool isCopy() const;
   bool isAssign() const;
   bool isRetain() const;
   bool isWeak() const;
   bool isStrong() const;
   bool isUnretained() const;
   bool isNew() const;
   bool isSealed() const;
   bool isImplementation() const;
   bool isExternal() const;
   bool isAlias() const;
   bool isDefault() const;
   bool isDelete() const;
   bool isNoExcept() const;
   bool isAttribute() const; // UNO IDL attribute
   bool isUNOProperty() const; // UNO IDL property
   bool isReadonly() const;
   bool isBound() const;
   bool isConstrained() const;
   bool isTransient() const;
   bool isMaybeVoid() const;
   bool isMaybeDefault() const;
   bool isMaybeAmbiguous() const;
   bool isPublished() const; // UNO IDL published
   bool isTemplateSpecialization() const;
   bool hasDocumentedParams() const;
   bool hasDocumentedReturnType() const;
   bool isObjCMethod() const;
   bool isObjCProperty() const;
   bool isConstructor() const;
   bool isDestructor() const;
   bool hasOneLineInitializer() const;
   bool hasMultiLineInitializer() const;
   bool protectionVisible() const;
   bool showInCallGraph() const;
   bool isStrongEnumValue() const;

   // derived get methods
   bool isFriendToHide() const;
   bool isNotFriend() const;
   bool isFunctionOrSignalSlot() const;
   bool isRelatedOrFriend() const;

   // output info
   bool isLinkableInProject() const;
   bool isLinkable() const;

   bool hasDocumentation() const override;
   bool isDeleted() const;
  
   bool isBriefSectionVisible() const;
   bool isDetailedSectionVisible(bool inGroup, bool inFile) const;
   bool isDetailedSectionLinkable() const;
   bool isFriendClass() const;
   bool isDocumentedFriendClass() const;

   QSharedPointer<MemberDef> reimplements() const;
   MemberList *reimplementedBy() const;
   bool isReimplementedBy(QSharedPointer<ClassDef> cd) const;
 
   QSharedPointer<ClassDef> relatedAlso() const;

   bool hasDocumentedEnumValues() const;
   QSharedPointer<MemberDef> getAnonymousEnumType() const;
   bool isDocsForDefinition() const;
   QSharedPointer<MemberDef> getEnumScope() const;
   QSharedPointer<MemberList> enumFieldList() const;

   void setEnumBaseType(const QByteArray &type);
   QByteArray enumBaseType() const;

   bool hasExamples();
   ExampleSDict *getExamples() const;
   bool isPrototype() const;

   // argument related members
   ArgumentList *argumentList() const;
   ArgumentList *declArgumentList() const;
   ArgumentList *templateArguments() const;
   QList<ArgumentList> *definitionTemplateParameterLists() const;

   // member group related members
   int getMemberGroupId() const;
   MemberGroup *getMemberGroup() const;

   bool fromAnonymousScope() const;
   bool anonymousDeclShown() const;
   QSharedPointer<MemberDef> fromAnonymousMember() const;

   // callgraph related members
   bool hasCallGraph() const;
   bool hasCallerGraph() const;
   bool visibleMemberGroup(bool hideNoHeader);

   QSharedPointer<MemberDef> templateMaster() const;
   QByteArray getScopeString() const;
   QSharedPointer<ClassDef> getClassDefOfAnonymousType();

   // cached typedef functions
   bool isTypedefValCached() const;
   QSharedPointer<ClassDef> getCachedTypedefVal() const;
   QByteArray getCachedTypedefTemplSpec() const;
   QByteArray getCachedResolvedTypedef() const;

   QSharedPointer<MemberDef> memberDefinition() const;
   QSharedPointer<MemberDef> memberDeclaration() const;
   QSharedPointer<MemberDef> inheritsDocsFrom() const;
   QSharedPointer<MemberDef> getGroupAlias() const;

   QSharedPointer<ClassDef> category() const;
   QSharedPointer<MemberDef> categoryRelation() const;

   QString displayName(bool = true) const override;
   QByteArray getDeclType() const;
   void getLabels(QStringList &sl, QSharedPointer<Definition> container) const;

   const ArgumentList *typeConstraints() const;

   //
   QByteArray documentation() const;
   QByteArray briefDescription(bool abbr = false) const;
   QByteArray fieldType() const;

   // set functions
   void setMemberType(MemberType t);
   void setDefinition(const char *d);
   void setFileDef(QSharedPointer<FileDef> fd);
   void setAnchor();
   void setProtection(Protection p);
   void setMemberSpecifiers(Entry::SpecifierFlags s);
   void mergeMemberSpecifiers(Entry::SpecifierFlags s);
   void setInitializer(const char *i);
   void setBitfields(const char *s);
   void setMaxInitLines(int lines);
   void setMemberClass(QSharedPointer<ClassDef> cd);
   void setSectionList(QSharedPointer<Definition> d, QSharedPointer<MemberList> sl);

   void setGroupDef(QSharedPointer<GroupDef> gd, Grouping::GroupPri_t pri,
                    const QByteArray &fileName, int startLine, bool hasDocs, 
                    QSharedPointer<MemberDef> member = QSharedPointer<MemberDef>());

   void setExplicitExternal(bool b);

   // property set
   void setPropertyRead(const char *data);
   void setPropertyWrite(const char *data);

   // copperspice - additional properties
   void setPropertyReset(const char *data);
   void setPropertyNotify(const char *data);

   void setTemplateSpecialization(bool b);

   void makeRelated();
   void makeForeign();
   void setHasDocumentedParams(bool b);
   void setHasDocumentedReturnType(bool b);
   void setInheritsDocsFrom(QSharedPointer<MemberDef> md);
   void setTagInfo(TagInfo *i);
   void setArgsString(const char *as);

   // relation to other members
   void setReimplements(QSharedPointer<MemberDef> md);
   void insertReimplementedBy(QSharedPointer<MemberDef> md);

   // in-body documentation
   //void setInbodyDocumentation(const char *docs,const char *file,int line);

   void setRelatedAlso(QSharedPointer<ClassDef> cd);

   // enumeration specific members
   void insertEnumField(QSharedPointer<MemberDef> md);
   void setEnumScope(QSharedPointer<MemberDef> md, bool livesInsideEnum = false);
   void setEnumClassScope(QSharedPointer<ClassDef> cd);
   void setDocumentedEnumValues(bool value);
   void setAnonymousEnumType(QSharedPointer<MemberDef> md);

   // example related members
   bool addExample(const char *anchor, const char *name, const char *file);

   // prototype related members
   void setPrototype(bool p);

   // argument related members
   void setArgumentList(ArgumentList *al);
   void setDeclArgumentList(ArgumentList *al);
   void setDefinitionTemplateParameterLists(QList<ArgumentList> *lists);
   void setTypeConstraints(ArgumentList *al);
   void setType(const char *t);
   void setAccessorType(QSharedPointer<ClassDef> cd, const QString &t);

   // namespace related members
   void setNamespace(QSharedPointer<NamespaceDef> nd);

   // member group related members
   void setMemberGroup(MemberGroup *grp);
   void setMemberGroupId(int id);
   void makeImplementationDetail();

   // anonymous scope members
   void setFromAnonymousScope(bool b);
   void setFromAnonymousMember(QSharedPointer<MemberDef> m);

   void enableCallGraph(bool e);
   void enableCallerGraph(bool e);

   void setTemplateMaster(QSharedPointer<MemberDef> mt);
   void addListReference(QSharedPointer<Definition> d);
   void setDocsForDefinition(bool b);
   void setGroupAlias(QSharedPointer<MemberDef> md);

   void cacheTypedefVal(QSharedPointer<ClassDef> val, const QByteArray &templSpec, const QByteArray &resolvedType);
   void invalidateTypedefValCache();

   void invalidateCachedArgumentTypes();

   // declaration <-> definition relation
   void setMemberDefinition(QSharedPointer<MemberDef> md);
   void setMemberDeclaration(QSharedPointer<MemberDef> md);

   void setAnonymousUsed();
   void copyArgumentNames(QSharedPointer<MemberDef> bmd);

   void setCategory(QSharedPointer<ClassDef> cd);
   void setCategoryRelation(QSharedPointer<MemberDef> md);

   void setDocumentation(const char *d, const char *docFile, int docLine, bool stripWhiteSpace = true);
   void setBriefDescription(const char *b, const char *briefFile, int briefLine);
   void setInbodyDocumentation(const char *d, const char *inbodyFile, int inbodyLine);

   void setHidden(bool b);
   
   // output generation
   void writeDeclaration(OutputList &ol, QSharedPointer<ClassDef> cd, QSharedPointer<NamespaceDef> nd,
                 QSharedPointer<FileDef> fd, QSharedPointer<GroupDef> gd,
                 bool inGroup, QSharedPointer<ClassDef> inheritFrom = QSharedPointer<ClassDef>(), 
                 const char *inheritId = 0);

   void writeDocumentation(MemberList *ml, OutputList &ol, const char *scopeName, QSharedPointer<Definition> container,
                 bool inGroup, bool showEnumValues = false, bool showInline = false);

   void writeMemberDocSimple(OutputList &ol, QSharedPointer<Definition> container);

   void writeEnumDeclaration(OutputList &typeDecl, QSharedPointer<ClassDef> cd, 
                  QSharedPointer<NamespaceDef> nd, QSharedPointer<FileDef> fd, QSharedPointer<GroupDef> gd);

   void writeTagFile(QTextStream &);
   void warnIfUndocumented();

   QSharedPointer<MemberDef> createTemplateInstanceMember(ArgumentList *formalArgs, ArgumentList *actualArgs);

   void findSectionsInDocumentation();

   bool visited;

 protected:
   void flushToDisk() const;
   void loadFromDisk() const;

 private:
   MemberDef(const MemberDef &);  

   template<class T, class ...Ts>
   friend QSharedPointer<typename std::enable_if<std::is_base_of<EnableSharedFromThis, T>::value, T>::type> QMakeShared(Ts &&...vs);

   void lock() const;
   void unlock() const;
   void saveToDisk() const;
   void makeResident() const;
   void _computeLinkableInProject();
   void _computeIsConstructor();
   void _computeIsDestructor();
   void _writeGroupInclude(OutputList &ol, bool inGroup);
   void _writeCallGraph(OutputList &ol);
   void _writeCallerGraph(OutputList &ol);
   void _writeReimplements(OutputList &ol);
   void _writeReimplementedBy(OutputList &ol);
   void _writeExamples(OutputList &ol);
   void _writeTypeConstraints(OutputList &ol);

   void _writeEnumValues(OutputList &ol, QSharedPointer<Definition> container,
                         const QByteArray &cfname, const QByteArray &ciname, const QByteArray &cname);

   void _writeCategoryRelation(OutputList &ol);
   void _writeTagData(const DefType);
   void _addToSearchIndex();

   static int s_indentLevel;
    
   void writeLink(OutputList &ol, QSharedPointer<ClassDef> cd, QSharedPointer<NamespaceDef> nd, 
                  QSharedPointer<FileDef> fd, QSharedPointer<GroupDef> gd, bool onlyText = false);

   // unsure if this is needed
   MemberDef &operator=(const MemberDef &);

   QScopedPointer<MemberDefImpl> m_impl;

   uchar m_isLinkableCached;    // 0 = not cached, 1=false, 2=true
   uchar m_isConstructorCached; // 0 = not cached, 1=false, 2=true
   uchar m_isDestructorCached;  // 0 = not cached, 1=false, 2=true
};

void combineDeclarationAndDefinition(QSharedPointer<MemberDef> mdec, QSharedPointer<MemberDef> mdef);

#endif
