/*
Abstract:
  Status control widget implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "status_control.h"
#include "status_control_ui.h"
#include "status_control_moc.h"
#include "ui/utils.h"
//common includes
#include <parameters.h>
//library includes
#include <core/module_attrs.h>
//qt includes
#include <QtGui/QGridLayout>
#include <QtGui/QLabel>

namespace
{
  const QString EMPTY_TEXT(QString::fromUtf8("-"));

  class StatusControlImpl : public StatusControl
                          , private Ui::StatusControl
  {
  public:
    explicit StatusControlImpl(QWidget* parent)
    {
      setParent(parent);
      setupUi(this);
    }

    virtual void UpdateState(const ZXTune::Module::State& state)
    {
      textPosition->setText(QString::number(state.Track.Position));
      textPattern->setText(QString::number(state.Track.Pattern));
      textLine->setText(QString::number(state.Track.Line));
      textFrame->setText(QString::number(state.Track.Quirk));
      textChannels->setText(QString::number(state.Track.Channels));
      textTempo->setText(QString::number(state.Reference.Quirk));
    }

    virtual void CloseState()
    {
      textPosition->setText(EMPTY_TEXT);
      textPattern->setText(EMPTY_TEXT);
      textLine->setText(EMPTY_TEXT);
      textFrame->setText(EMPTY_TEXT);
      textChannels->setText(EMPTY_TEXT);
      textTempo->setText(EMPTY_TEXT);
    }
  };
}

StatusControl* StatusControl::Create(QWidget* parent)
{
  assert(parent);
  return new StatusControlImpl(parent);
}
