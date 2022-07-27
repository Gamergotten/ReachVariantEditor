#pragma once
#include <QString>
#include "./_base.h"

namespace halo::reach::megalo::AST {
   class alias : public item_base, public block_child {
      public:
         QString name;
         QString target; // TODO: PLACEHOLDER
   };
}