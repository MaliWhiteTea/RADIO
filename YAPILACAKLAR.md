# RADIO — Yapılacaklar / Notlar

**Son güncelleme:** 2026-09-24

## Sürüm noktaları

- **Son kayıtlı beta:** `3f94fec` — `radio beta 1` (2026-09-24)
- **Jingle öncesi yedek:** `efedf83` — `Snapshot before PCM DAC jingle experiment.` (2026-08-21)

Geri dönüş gerekirse önce mevcut değişiklikleri commit et veya yedekle. Ardından
istenen commit'i incelemek için `git switch --detach <commit>` kullanılabilir.

## Öncelikli düzeltmeler

- [ ] RDA5807M `REG05_BASE = 0x9080` değerini veri sayfası ve kullanılan modülle doğrula
  - Ayrılmış bit 12 şu anda `1`
  - `SEEKTH` alanı şu anda `0`; istasyon arama hassasiyetini etkileyebilir
- [ ] `REG04_VALUE = 0x0C00` değerindeki ayrılmış bit 10'u ve soft-mute tercihini doğrula
- [ ] Jingle öncesindeki mute durumunu sakla; jingle bitince aynı durumu geri yükle
- [ ] DAC örneklemesini ana döngü yerine timer veya I2S/DMA ile kararlı hale getir
- [ ] Favori kaydından önce `radioIsReady()` ve `frequencyValid` kontrolü yap
- [ ] Arduino IDE veya `arduino-cli` ile temiz ESP32 derlemesi doğrula

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

- [x] Proje README dosyası
- [ ] Favori tuşu (yıldız) — pin ata; kısa=git, uzun=kaydet (altyapı hazır)
- [ ] Favoriler için seri komutlar ekle (`fav save`, `fav 1` vb.)
- [ ] Altı favori yuvasının tamamını kullanıcı arayüzünden erişilebilir yap
- [ ] Geçici: tuş basılı tut = ses ayarı → ileride kaldır
- [ ] Geçici: LED başlangıçta açık → ileride değiştir
- [ ] `DEBUG_RADIO = false` (iş bitince)
- [ ] Ekran modülü (ileride)
- [ ] AUX girişi (ileride)
- [ ] Flash / son frekans-ses kaydı (ileride)
- [ ] Jingle kalitesi / `LOUD_MODE` gözden geçir (bozulma vs ses)
- [ ] Jingle çalarken gelen mute/seek/tune komutlarının davranışını netleştir
- [ ] Donanım üzerinde tune, seek, timeout ve I2C kopma senaryolarını test et

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
