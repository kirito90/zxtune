/*
Abstract:
  Playlist items properties dialog implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "properties_dialog.h"
#include "properties_dialog.ui.h"
#include "playlist/supp/capabilities.h"
#include "playlist/ui/table_view.h"
#include "ui/utils.h"
#include "ui/tools/parameters_helpers.h"
//common includes
#include <contract.h>
#include <error.h>
//library includes
#include <core/core_parameters.h>
#include <core/module_attrs.h>
#include <sound/sound_parameters.h>
//boost includes
#include <boost/make_shared.hpp>
#include <boost/ref.hpp>
//qt includes
#include <QtGui/QAbstractButton>
#include <QtGui/QComboBox>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QToolButton>

namespace
{
  class ItemPropertiesContainer : public Parameters::Container
  {
  public:
    ItemPropertiesContainer(Parameters::Container::Ptr adjusted, Parameters::Accessor::Ptr native)
      : Adjusted(adjusted)
      , Merged(Parameters::CreateMergedAccessor(adjusted, native))
    {
    }

    virtual bool FindValue(const Parameters::NameType& name, Parameters::IntType& val) const
    {
      return Merged->FindValue(name, val);
    }

    virtual bool FindValue(const Parameters::NameType& name, Parameters::StringType& val) const
    {
      return Merged->FindValue(name, val);
    }

    virtual bool FindValue(const Parameters::NameType& name, Parameters::DataType& val) const
    {
      return Merged->FindValue(name, val);
    }

    virtual void Process(Parameters::Visitor& visitor) const
    {
      return Merged->Process(visitor);
    }

    virtual void SetValue(const Parameters::NameType& name, Parameters::IntType val)
    {
      return Adjusted->SetValue(name, val);
    }

    virtual void SetValue(const Parameters::NameType& name, const Parameters::StringType& val)
    {
      return Adjusted->SetValue(name, val);
    }

    virtual void SetValue(const Parameters::NameType& name, const Parameters::DataType& val)
    {
      return Adjusted->SetValue(name, val);
    }

    virtual void RemoveValue(const Parameters::NameType& name)
    {
      return Adjusted->RemoveValue(name);
    }
  private:
    const Parameters::Container::Ptr Adjusted;
    const Parameters::Accessor::Ptr Merged;
  };

  class PropertiesDialogImpl : public Playlist::UI::PropertiesDialog
                             , public Playlist::UI::Ui_PropertiesDialog
  {
  public:
    explicit PropertiesDialogImpl(QWidget& parent, Playlist::Item::Data::Ptr item)
      : Playlist::UI::PropertiesDialog(parent)
    {
      //setup self
      setupUi(this);
      setWindowTitle(ToQString(item->GetFullPath()));

      //TODO: query only item
      const ZXTune::Module::Holder::Ptr module = item->GetModule();
      const Parameters::Accessor::Ptr nativeProps = module->GetModuleProperties();
      const Parameters::Container::Ptr adjustedProps = item->GetAdjustedParameters();
      Properties = boost::make_shared<ItemPropertiesContainer>(adjustedProps, nativeProps);

      const Playlist::Item::Capabilities caps(item);
      FillProperties(caps);
      itemsLayout->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding), itemsLayout->rowCount(), 0);

      connect(buttons, SIGNAL(clicked(QAbstractButton*)), SLOT(ButtonClicked(QAbstractButton*)));
    }

    virtual void ButtonClicked(QAbstractButton* button)
    {
      switch (buttons->buttonRole(button))
      {
      case QDialogButtonBox::ResetRole:
        emit ResetToDefaults();
      default:
        return;
      }
    }
  private:
    void FillProperties(const Playlist::Item::Capabilities& caps)
    {
      AddStringProperty(Playlist::UI::PropertiesDialog::tr("Title"), ZXTune::Module::ATTR_TITLE);
      AddStringProperty(Playlist::UI::PropertiesDialog::tr("Author"), ZXTune::Module::ATTR_AUTHOR);
      AddStringProperty(Playlist::UI::PropertiesDialog::tr("Comment"), ZXTune::Module::ATTR_COMMENT);

      QStringList valuesOffOn;
      valuesOffOn << Playlist::UI::PropertiesDialog::tr("Off") << Playlist::UI::PropertiesDialog::tr("On");

      if (caps.IsAYM())
      {
        QStringList chipTypes;
        chipTypes << QLatin1String("AY-3-8910/12") << QLatin1String("YM2149");
        AddSetProperty(Playlist::UI::PropertiesDialog::tr("Chip type"), Parameters::ZXTune::Core::AYM::TYPE, chipTypes);
        QStringList layouts;
        layouts
          << QLatin1String("ABC")
          << QLatin1String("ACB")
          << QLatin1String("BAC")
          << QLatin1String("BCA")
          << QLatin1String("CAB")
          << QLatin1String("CBA")
          << Playlist::UI::PropertiesDialog::tr("Mono");
        AddSetProperty(Playlist::UI::PropertiesDialog::tr("Layout"), Parameters::ZXTune::Core::AYM::LAYOUT, layouts);
        AddSetProperty(Playlist::UI::PropertiesDialog::tr("Interpolation"), Parameters::ZXTune::Core::AYM::INTERPOLATION, valuesOffOn);
        const Parameters::IntegerTraits clockRate(Parameters::ZXTune::Core::AYM::CLOCKRATE, -1, Parameters::ZXTune::Core::AYM::CLOCKRATE_MIN, Parameters::ZXTune::Core::AYM::CLOCKRATE_MAX);
        AddIntegerProperty(Playlist::UI::PropertiesDialog::tr("Clockrate, Hz"), clockRate);
        const Parameters::IntegerTraits frameDuration(Parameters::ZXTune::Sound::FRAMEDURATION, -1, Parameters::ZXTune::Sound::FRAMEDURATION_MIN, Parameters::ZXTune::Sound::FRAMEDURATION_MAX);
        AddIntegerProperty(Playlist::UI::PropertiesDialog::tr("Frame duration, uS"), frameDuration);
      }
      if (caps.IsDAC())
      {
        AddSetProperty(Playlist::UI::PropertiesDialog::tr("Interpolation"), Parameters::ZXTune::Core::DAC::INTERPOLATION, valuesOffOn);
      }
    }

    void AddStringProperty(const QString& title, const Parameters::NameType& name)
    {
      QLineEdit* const wid = new QLineEdit(this);
      Parameters::Value* const value = Parameters::StringValue::Bind(*wid, *Properties, name, Parameters::StringType());
      AddProperty(title, wid, value);
    }

    void AddSetProperty(const QString& title, const Parameters::NameType& name, const QStringList& values)
    {
      QComboBox* const wid = new QComboBox(this);
      wid->addItems(values);
      Parameters::Value* const value = Parameters::IntegerValue::Bind(*wid, *Properties, name, -1);
      AddProperty(title, wid, value);
    }

    void AddIntegerProperty(const QString& title, const Parameters::IntegerTraits& traits)
    {
      QLineEdit* const wid = new QLineEdit(this);
      Parameters::Value* const value = Parameters::BigIntegerValue::Bind(*wid, *Properties, traits);
      AddProperty(title, wid, value);
    }

    void AddProperty(const QString& title, QWidget* widget, Parameters::Value* value)
    {
      QToolButton* const resetButton = new QToolButton(this);
      resetButton->setArrowType(Qt::DownArrow);
      resetButton->setToolTip(Playlist::UI::PropertiesDialog::tr("Reset value"));
      const int row = itemsLayout->rowCount();
      itemsLayout->addWidget(new QLabel(title, this), row, 0);
      itemsLayout->addWidget(widget, row, 1);
      itemsLayout->addWidget(resetButton, row, 2);
      Require(value->connect(this, SIGNAL(ResetToDefaults()), SLOT(Reset())));
      Require(value->connect(resetButton, SIGNAL(clicked()), SLOT(Reset())));
    }
  private:
    Parameters::Container::Ptr Properties;
  };
}

namespace Playlist
{
  namespace UI
  {
    PropertiesDialog::PropertiesDialog(QWidget& parent) : QDialog(&parent)
    {
    }

    PropertiesDialog::Ptr PropertiesDialog::Create(QWidget& parent, Item::Data::Ptr item)
    {
      return boost::make_shared<PropertiesDialogImpl>(boost::ref(parent), item);
    }

    void ExecutePropertiesDialog(QWidget& parent, Model::Ptr model, Model::IndexSetPtr scope)
    {
      if (scope->size() != 1)
      {
        return;
      }
      const Item::Data::Ptr item = model->GetItem(*scope->begin());
      if (!item->GetState())
      {
        const PropertiesDialog::Ptr dialog = PropertiesDialog::Create(parent, item);
        dialog->exec();
      }
    }
  }
}
