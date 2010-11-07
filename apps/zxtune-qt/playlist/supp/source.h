/*
Abstract:
  Playlist data source interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#ifndef ZXTUNE_QT_PLAYLIST_SOURCE_H_DEFINED
#define ZXTUNE_QT_PLAYLIST_SOURCE_H_DEFINED

//local includes
#include "supp/playitems_provider.h"
//qt includes
#include <QtCore/QStringList>

class ScannerCallback
{
public:
  virtual ~ScannerCallback() {}

  virtual bool IsCanceled() const = 0;

  virtual void OnPlayitem(const Playitem::Ptr& item) = 0;
  virtual void OnProgress(unsigned progress, unsigned curItem) = 0;
  virtual void OnReport(const QString& report, const QString& item) = 0;
  virtual void OnError(const class Error& err) = 0;
};

class ScannerSource
{
public:
  typedef boost::shared_ptr<ScannerSource> Ptr;

  virtual ~ScannerSource() {}

  virtual unsigned Resolve() = 0;
  virtual void Process() = 0;

  static Ptr CreateOpenFileSource(PlayitemsProvider& provider, ScannerCallback& callback, const QStringList& items);
  static Ptr CreateDetectFileSource(PlayitemsProvider& provider, ScannerCallback& callback, const QStringList& items);
};

#endif //ZXTUNE_QT_PLAYLIST_SOURCE_H_DEFINED
