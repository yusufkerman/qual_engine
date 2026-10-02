#pragma once

/**
 * @file engine_mode.h
 * @brief Motorun aktif çalışma modunu (Editör veya Oyun) tanımlar.
 */

 /**
  * @enum EngineMode
  * @brief Motor döngüsünün editör düzenleme durumunda mı yoksa oyun simülasyonunda mı olduğunu belirtir.
  */
enum class EngineMode
{
    EDITOR, ///< Sahne düzenleme, gizmo ve editör serbest uçuş kamerası aktif.
    PLAY    ///< Oyun mantığı, fizik simülasyonu ve oyun kamerası aktif.
};