/*
Abstract:
  Playlist scanner interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#ifndef ZXTUNE_QT_PLAYLIST_SUPP_SCANNER_H_DEFINED
#define ZXTUNE_QT_PLAYLIST_SUPP_SCANNER_H_DEFINED

//local includes
#include "supp/playitems_provider.h"
//qt includes
#include <QtCore/QThread>

namespace Playlist
{
  class Scanner : public QThread
  {
    Q_OBJECT
  protected:
    explicit Scanner(QObject& parent);
  public:
    static Scanner* Create(QObject& parent, PlayitemsProvider::Ptr provider);

    virtual void AddItems(const QStringList& items, bool deepScan) = 0;
    virtual void AddItems(Playitem::Iterator::Ptr items, int countHint = -1) = 0;
  public slots:
    //asynchronous, doesn't wait until real stop
    virtual void Cancel() = 0;
  signals:
    //for UI
    void OnScanStart();
    void OnProgressStatus(unsigned progress, unsigned itemsDone, unsigned totalItems);
    void OnProgressMessage(const QString& message, const QString& item);
    void OnScanStop();
    void OnResolvingStart();
    void OnResolvingStop();
    //for BL
    void OnGetItem(Playitem::Ptr);
  };
}

#endif //ZXTUNE_QT_PLAYLIST_SUPP_SCANNER_H_DEFINED
