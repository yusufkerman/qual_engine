#pragma once

/**
 * @file ICommand.h
 * @brief Geri alınabilir (Undo/Redo) editör eylemleri için temel komut arayüzünü tanımlar.
 */

#include "../Adapters/IEditableWorld.h"

namespace vesta {

    /**
     * @class ICommand
     * @brief Komut deseni (Command Pattern) için saf sanal (pure virtual) arayüz sınıfı.
     */
    class ICommand {
    public:
        virtual ~ICommand() = default;

        /**
         * @brief Komutun sahne üzerindeki mutasyonunu yürütür.
         * @param world Sahne üzerinde değişiklik yapmaya izin veren dünya adaptörü.
         */
        virtual void execute(IEditableWorld& world) = 0;

        /**
         * @brief Yürütülen komutun etkisini geri alır (tersine çevirir).
         * @param world Sahne üzerinde değişiklik yapmaya izin veren dünya adaptörü.
         */
        virtual void undo(IEditableWorld& world) = 0;
    };

} // namespace vesta