#include "page_forge_labels.h"
#include "../../helpers/bitwise.h"

ScriptEditorPageForgeLabels::ScriptEditorPageForgeLabels(QWidget* parent) : QWidget(parent) {
   ui.setupUi(this);
   //
   this->ui.name->setLimitedToSingleLanguageStrings(true);
   //
   auto& editor = ReachEditorState::get();
   //
   QObject::connect(&editor, &ReachEditorState::variantAcquired, this, &ScriptEditorPageForgeLabels::updateListFromVariant);
   QObject::connect(this->ui.list, &QListWidget::currentRowChanged, this, &ScriptEditorPageForgeLabels::selectLabel);
   //
   QObject::connect(this->ui.reqFlagTeam, &QCheckBox::stateChanged, [this](int state) {
      if (auto label = this->getLabel()) {
         cobb::modify_bit(label->requirements, Megalo::ReachForgeLabel::requirement_flags::assigned_team, state == Qt::CheckState::Checked);
         this->ui.reqTeam->setDisabled(!label->requires_assigned_team());
      }
   });
   QObject::connect(this->ui.reqFlagObjectType, &QCheckBox::stateChanged, [this](int state) {
      if (auto label = this->getLabel()) {
         cobb::modify_bit(label->requirements, Megalo::ReachForgeLabel::requirement_flags::objects_of_type, state == Qt::CheckState::Checked);
         this->ui.reqObjectType->setDisabled(!label->requires_object_type());
      }
   });
   QObject::connect(this->ui.reqFlagNumber, &QCheckBox::stateChanged, [this](int state) {
      if (auto label = this->getLabel()) {
         cobb::modify_bit(label->requirements, Megalo::ReachForgeLabel::requirement_flags::number, state == Qt::CheckState::Checked);
         this->ui.reqNumber->setDisabled(!label->requires_number());
      }
   });
   QObject::connect(this->ui.reqTeam, &MegaloConstTeamIndexCombobox::currentDataChanged, [this](Megalo::const_team t) {
      if (auto label = this->getLabel())
         label->requiredTeam = t;
   });
   QObject::connect(this->ui.reqObjectType, &MPObjectTypeCombobox::currentDataChanged, [this](uint32_t objectType) {
      if (auto label = this->getLabel())
         label->requiredObjectType = objectType;
   });
   QObject::connect(this->ui.reqNumber, QOverload<int>::of(&QSpinBox::valueChanged), [this](int value) {
      if (auto label = this->getLabel())
         label->requiredNumber = value;
   });
   QObject::connect(this->ui.minCount, QOverload<int>::of(&QSpinBox::valueChanged), [this](int value) {
      if (auto label = this->getLabel())
         label->mapMustHaveAtLeast = value;
   });
   //
   this->updateListFromVariant(nullptr);
}
void ScriptEditorPageForgeLabels::selectLabel(int32_t labelIndex) {
   this->currentForgeLabel = labelIndex;
   this->updateLabelFromVariant();
}
Megalo::ReachForgeLabel* ScriptEditorPageForgeLabels::getLabel() const noexcept {
   if (this->currentForgeLabel < 0)
      return nullptr;
   auto mp = ReachEditorState::get().multiplayerData();
   if (!mp)
      return nullptr;
   auto& list = mp->scriptContent.forgeLabels;
   if (this->currentForgeLabel >= list.size())
      return nullptr;
   return list[this->currentForgeLabel];
}
void ScriptEditorPageForgeLabels::updateListFromVariant(GameVariant* variant) {
   if (!variant) {
      variant = ReachEditorState::get().variant();
      if (!variant)
         return;
   }
   auto mp = variant->get_multiplayer_data();
   if (!mp)
      return;
   auto  container = this->ui.list;
   auto& list = mp->scriptContent.forgeLabels;
   container->clear();
   for (size_t i = 0; i < list.size(); i++) {
      auto& label = *list[i];
      if (label.name) {
         container->addItem(QString::fromUtf8(label.name->english().c_str()));
      } else {
         container->addItem(tr("<unnamed label %1>").arg(i));
      }
   }
}
void ScriptEditorPageForgeLabels::updateLabelFromVariant(GameVariant* variant) {
   if (!variant) {
      variant = ReachEditorState::get().variant();
      if (!variant)
         return;
   }
   auto mp = variant->get_multiplayer_data();
   if (!mp)
      return;
   auto& labels = mp->scriptContent.forgeLabels;
   if (this->currentForgeLabel >= labels.size())
      return;
   auto& label = *labels[this->currentForgeLabel].get();
   //
   const QSignalBlocker blocker1(this->ui.reqFlagNumber);
   const QSignalBlocker blocker2(this->ui.reqFlagObjectType);
   const QSignalBlocker blocker3(this->ui.reqFlagTeam);
   const QSignalBlocker blocker4(this->ui.reqNumber);
   const QSignalBlocker blocker5(this->ui.reqObjectType);
   const QSignalBlocker blocker6(this->ui.reqTeam);
   const QSignalBlocker blocker7(this->ui.minCount);
   this->ui.reqFlagNumber->setChecked(label.requires_number());
   this->ui.reqFlagObjectType->setChecked(label.requires_object_type());
   this->ui.reqFlagTeam->setChecked(label.requires_assigned_team());
   this->ui.reqNumber->setValue(label.requiredNumber);
   this->ui.reqObjectType->setValue(label.requiredObjectType);
   this->ui.reqTeam->setValue(label.requiredTeam);
   this->ui.minCount->setValue(label.mapMustHaveAtLeast);
   //
   this->ui.reqNumber->setDisabled(!label.requires_number());
   this->ui.reqObjectType->setDisabled(!label.requires_object_type());
   this->ui.reqTeam->setDisabled(!label.requires_assigned_team());
   //
   this->ui.name->setTarget(label.name);
}