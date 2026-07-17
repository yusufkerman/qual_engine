#pragma once
#include "../Adapters/IEditableWorld.h"

namespace vesta {

    class ICommand {
    public:
        virtual ~ICommand() = default;

        // Komutu çalıştır (L1 dünyasında mutasyon yap)
        virtual void execute(IEditableWorld& world) = 0;

        // Komutun yaptığı şeyi tam tersine çevir
        virtual void undo(IEditableWorld& world) = 0;
    };

} // namespace vesta