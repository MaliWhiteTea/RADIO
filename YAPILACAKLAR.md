# RADIO — Yapılacaklar / Notlar

**Son güncelleme:** 2026-08-21

**Yedek commit (jingle öncesi):** `efedf83`  
Geri dönüş: `git reset --hard efedf83`

## Donanım

- [ ] Transistör yükselteç (jingle / DAC GPIO25 → amp LIN)
  - NPN (BC547 / 2N2222)
  - GPIO25 --[10µF]--+-- Base
  - `[47k]-- 5V` / `[10k]-- GND`
  - 5V --[1k]-- Collector --[100µF]-- amp LIN
  - Emitter -- GND (ortak toprak)
  - Hoparlör sadece amp çıkışında
  - Sadece LIN (GPIO26 / RIN yok)
- [ ] Amp besleme / pot kontrolü (ses hâlâ kısıksa)
- [ ] RDA LOUT ile jingle hattının karışmaması (gerekirse anahtar / mute)

## Yazılım / özellik

- [ ] Favori tuşu (yıldız) — pin ata; kısa=git, uzun=kaydet (altyapı hazır)
- [ ] Geçici: tuş basılı tut = ses ayarı → ileride kaldır
- [ ] Geçici: LED başlangıçta açık → ileride değiştir
- [ ] `DEBUG_RADIO = false` (iş bitince)
- [ ] Ekran modülü (ileride)
- [ ] AUX girişi (ileride)
- [ ] Flash / son frekans-ses kaydı (ileride)
- [ ] Jingle kalitesi / `LOUD_MODE` gözden geçir (bozulma vs ses)

## Jingle (PCM + DAC)

- Dosya: `JingleData.h` (8 kHz, 8-bit, ~1.57 sn)
- Pin: GPIO25 (DAC1) → (transistör) → amp LIN
- Seri komut: `jingle`
- Ayarlar: `Data.h` → `AudioCfg` (GAIN, LOUD_MODE, PLAY_ON_BOOT)

## Bağlantı özeti (hedef)

| ESP32 / RDA | Nereye |
|-------------|--------|
| 3.3V | RDA VCC |
| GND | RDA GND, amp GND, transistör GND |
| GPIO21/22 | RDA SDA/SCL |
| GPIO25 | [transistör] → amp LIN |
| GPIO27 | LED |
| GPIO13 | seek up (kısa) / vol+ (basılı tut, geçici) |
| GPIO14 | seek down (kısa) / vol- (basılı tut, geçici) |
| RDA LOUT | amp (jingle ile çakışmayı çöz) |
| RDA ANT | tel anten |

## Notlar

- ESP32 DAC tek başına zayıf; transistör veya iyi amp şart.
- Commit ile yedek almadan büyük deneme yapma.
